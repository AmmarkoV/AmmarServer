#include "chat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CHAT_DIRECTORY "db/chat/"
#define DEFAULT_ROOM "lobby"
#define MAX_ROOM_NAME 32
#define MAX_CHAT_MESSAGE 512
/*A log only grows , so pages and polls serve its tail rather than however big it has become..*/
#define MAX_CHAT_LOG_SERVED 32768
#define MAX_CHAT_LIST 8192


/*A room and a private thread are the same thing pointed at a different file , so everything below works on
  one of these rather than on rooms and direct messages separately..*/
struct chatConversation
{
  char path[MAX_FILE_PATH];
  char key[8];                            /*"room" or "u" - how a client addresses this conversation back*/
  char value[MAX_USERNAME];               /*the room name , or the other participant*/
  char title[MAX_ESCAPED_USERNAME+64];
};


static void roomPath(const char * room,char * output,unsigned int outputSize)
{
  snprintf(output,outputSize,"%sroom.%s.chat",CHAT_DIRECTORY,room);
}


/*Both participants have to end up reading and writing the same file , so the two names always go in the same
  order whichever of the two asked. A username can never contain a dot , which is what keeps the one separating
  them unambiguous when the inbox reads these names back off the filename..*/
static void directMessagePath(const char * a,const char * b,char * output,unsigned int outputSize)
{
  if (strcmp(a,b)<=0) { snprintf(output,outputSize,"%sdm.%s.%s.chat",CHAT_DIRECTORY,a,b); }
  else                { snprintf(output,outputSize,"%sdm.%s.%s.chat",CHAT_DIRECTORY,b,a); }
}


int initializeChat()
{
  mkdir(CHAT_DIRECTORY,0755); //Already there is the normal case , not an error..

  char path[MAX_FILE_PATH]={0};
  roomPath(DEFAULT_ROOM,path,sizeof(path));

  FILE * fp=fopen(path,"a");
  if (fp==0) { AmmServer_Error("Could not create the default chat room at %s",path); return 0; }
  fclose(fp);

  return 1;
}


/*Works out which conversation a request is talking about , from either its GET or its POST fields. Returns 0
  when none was named ( the chat index ) or when what was named isn't something this visitor may open..*/
static int resolveConversation(struct AmmServer_DynamicRequest * rqst,const char * viewer,struct chatConversation * conversation)
{
  memset(conversation,0,sizeof(struct chatConversation));
  char escaped[MAX_ESCAPED_USERNAME]={0};

  char room[MAX_ROOM_NAME]={0};
  if ( (_GETcpy(rqst,"room",room,sizeof(room))) || (_POSTcpy(rqst,"room",room,sizeof(room))) )
  {
    //The account name rules ( 3-31 letters/digits/_/- ) are exactly the room name rules , and they are also
    //what stops a room name from walking out of db/chat/ into a path of its own choosing..
    if ( ! uadbWeb_isValidUsername(room) ) { return 0; }

    roomPath(room,conversation->path,sizeof(conversation->path));
    snprintf(conversation->key,sizeof(conversation->key),"room");
    snprintf(conversation->value,sizeof(conversation->value),"%s",room);
    AmmServer_HTMLEscape(room,escaped,sizeof(escaped));
    snprintf(conversation->title,sizeof(conversation->title),"Room %s",escaped);
    return 1;
  }

  char withUser[MAX_USERNAME]={0};
  if ( (_GETcpy(rqst,"u",withUser,sizeof(withUser))) || (_POSTcpy(rqst,"u",withUser,sizeof(withUser))) )
  {
    if ( ! uadbWeb_isValidUsername(withUser) )        { return 0; }
    if ( ! uadbWeb_userAccountExists(uadb,withUser) )  { return 0; }

    //A thread is addressed as "me and them" , so a visitor can only ever open one they are themselves in -
    //there is no way to name somebody else's private conversation..
    directMessagePath(viewer,withUser,conversation->path,sizeof(conversation->path));
    snprintf(conversation->key,sizeof(conversation->key),"u");
    snprintf(conversation->value,sizeof(conversation->value),"%s",withUser);
    AmmServer_HTMLEscape(withUser,escaped,sizeof(escaped));
    snprintf(conversation->title,sizeof(conversation->title),"Messages with %s",escaped);
    return 1;
  }

  return 0;
}


static int appendMessage(const char * path,const char * from,const char * message)
{
  FILE * fp=fopen(path,"a");
  if (fp==0) { return 0; }

  fprintf(fp,"%s:%s<br>\n",from,message);
  fclose(fp);
  return 1;
}


/*Reads back the last bufferSize-1 bytes of a chat log , trimmed forward to a line boundary so a cut never
  serves half a message..*/
static unsigned int readChatLogTail(const char * path,char * buffer,unsigned int bufferSize)
{
  buffer[0]=0;

  FILE * fp=fopen(path,"r");
  if (fp==0) { return 0; }

  fseek(fp,0,SEEK_END);
  long fileSize=ftell(fp);
  long readSize=(long) bufferSize-1;
  long offset=0;
  if (fileSize>readSize) { offset=fileSize-readSize; } else { readSize=fileSize; }

  fseek(fp,offset,SEEK_SET);
  size_t bytesRead=fread(buffer,1,(size_t) readSize,fp);
  buffer[bytesRead]=0;
  fclose(fp);

  if (offset>0)
  {
    char * firstNewline=strchr(buffer,'\n');
    if (firstNewline!=0) { memmove(buffer,firstNewline+1,strlen(firstNewline+1)+1); }
  }

  return strlen(buffer);
}


/*The chat index : every room there is , and the private threads this visitor is part of..*/
static void renderConversationIndex(struct AmmServer_DynamicRequest * rqst,const char * viewer,const char * csrfToken)
{
  char rooms[MAX_CHAT_LIST]={0};
  char threads[MAX_CHAT_LIST]={0};
  unsigned int roomsPosition=0,threadsPosition=0;
  char entry[512]={0};
  char escaped[MAX_ESCAPED_USERNAME]={0};

  //One directory of logs is the whole listing - creating a room is creating its file , with no register of
  //rooms to keep in step with what is actually on disk..
  DIR * directory=opendir(CHAT_DIRECTORY);
  if (directory!=0)
  {
    struct dirent * directoryEntry=0;
    while ( ( directoryEntry=readdir(directory) ) != 0 )
    {
      char name[256]={0};
      snprintf(name,sizeof(name),"%s",directoryEntry->d_name);

      unsigned int nameLength=strlen(name);
      if (nameLength<=5)                        { continue; }
      if (strcmp(name+nameLength-5,".chat")!=0) { continue; }
      name[nameLength-5]=0; //Leaving room.<name> or dm.<a>.<b>

      if (strncmp(name,"room.",5)==0)
      {
        AmmServer_HTMLEscape(name+5,escaped,sizeof(escaped));
        snprintf(entry,sizeof(entry),"<li><a href=\"chat.html?room=%s\">%s</a></li>",escaped,escaped);
        socialAppendChunk(rooms,sizeof(rooms),&roomsPosition,entry);
      } else
      if (strncmp(name,"dm.",3)==0)
      {
        char * first=name+3;
        char * separator=strchr(first,'.');
        if (separator==0) { continue; }
        *separator=0;
        char * second=separator+1;

        //Only the two people in a thread ever see it listed..
        const char * other=0;
        if      (strcmp(first,viewer)==0)  { other=second; }
        else if (strcmp(second,viewer)==0) { other=first;  }
        if (other==0) { continue; }

        AmmServer_HTMLEscape(other,escaped,sizeof(escaped));
        snprintf(entry,sizeof(entry),"<li><a href=\"chat.html?u=%s\">%s</a></li>",escaped,escaped);
        socialAppendChunk(threads,sizeof(threads),&threadsPosition,entry);
      }
    }
    closedir(directory);
  }

  if (roomsPosition==0)   { socialAppendChunk(rooms,sizeof(rooms),&roomsPosition,"<li>No rooms yet</li>"); }
  if (threadsPosition==0) { socialAppendChunk(threads,sizeof(threads),&threadsPosition,"<li>Nobody has messaged you yet</li>"); }

  snprintf(rqst->content,rqst->MAXcontentSize,
           "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Chats</title>"
           "<link rel='stylesheet' type='text/css' href='chat.css'></head>"
           "<body><div class=\"chatbox\"><div class=\"chatname\"><h3>Chats</h3></div>"
           "<div class=\"chatindex\">"
            "<h4>Rooms</h4><ul>%s</ul>"
            "<form method=\"post\" enctype=\"multipart/form-data\" action=\"createRoom.html\">"
             "<input type=\"hidden\" name=\"csrf\" value=\"%s\">"
             "<input type=\"text\" name=\"name\" maxlength=\"31\" placeholder=\"New room name\">"
             "<button type=\"submit\">Create</button>"
            "</form>"
            "<h4>Direct messages</h4><ul>%s</ul>"
           "</div></div></body></html>",
           rooms,csrfToken,threads);
  rqst->contentSize=strlen(rqst->content);
}


static void renderConversationPage(struct AmmServer_DynamicRequest * rqst,struct chatConversation * conversation,const char * csrfToken)
{
  char * messages=(char*) malloc(MAX_CHAT_LOG_SERVED);
  if (messages==0) { return; }
  readChatLogTail(conversation->path,messages,MAX_CHAT_LOG_SERVED);

  snprintf(rqst->content,rqst->MAXcontentSize,
           "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>%s</title>"
           "<link rel='stylesheet' type='text/css' href='chat.css'>"
           "<script type=\"text/javascript\" src=\"chat.js\"></script></head>"
           "<body onload=\"goToEndOfMessages();\">"
           "<div class=\"chatbox\">"
            "<div class=\"chatname\"><h3>%s</h3><a href=\"chat.html\">all chats</a></div>"
            "<div class=\"chatmessages\" id=\"chatmessages\">%s</div>"
            "<div class=\"chatsend\">"
             "<form id='chat' onsubmit='sendNewMessage(); return false;'>"
              "<input type='hidden' id='csrf' value='%s'>"
              "<input type='hidden' id='ckey' value='%s'>"
              "<input type='hidden' id='cvalue' value='%s'>"
              "<input type='text' id='text' placeholder='Type and push enter!'>"
              "<input type='submit' value='&#9786;'>"
             "</form>"
            "</div>"
           "</div>"
           "</body></html>",
           conversation->title,conversation->title,messages,csrfToken,conversation->key,conversation->value);
  rqst->contentSize=strlen(rqst->content);

  free(messages);
}


void * chatPage_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char username[MAX_USERNAME]={0};
  if ( ! AmmServer_CurrentUsername(rqst,username,sizeof(username)) ) { socialServeLoginRequired(rqst); return 0; }

  char csrfToken[MAX_CSRF_TOKEN]={0};
  if ( ! AmmServer_GenerateCSRFToken(rqst,csrfToken,sizeof(csrfToken)) )
    { snprintf(rqst->content,rqst->MAXcontentSize,"Could not issue a security token"); rqst->contentSize=strlen(rqst->content); return 0; }

  struct chatConversation conversation;
  if ( resolveConversation(rqst,username,&conversation) ) { renderConversationPage(rqst,&conversation,csrfToken); }
  else                                                    { renderConversationIndex(rqst,username,csrfToken); }

  return 0;
}


void * chatMessages_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char username[MAX_USERNAME]={0};
  if ( ! AmmServer_CurrentUsername(rqst,username,sizeof(username)) )
    { snprintf(rqst->content,rqst->MAXcontentSize,"Not logged in"); rqst->contentSize=strlen(rqst->content); return 0; }

  struct chatConversation conversation;
  if ( ! resolveConversation(rqst,username,&conversation) )
    { snprintf(rqst->content,rqst->MAXcontentSize,"No such conversation"); rqst->contentSize=strlen(rqst->content); return 0; }

  rqst->contentSize=readChatLogTail(conversation.path,rqst->content,MAX_CHAT_LOG_SERVED);
  return 0;
}


void * chatSpeak_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char username[MAX_USERNAME]={0};
  if ( ! AmmServer_CurrentUsername(rqst,username,sizeof(username)) )
    { snprintf(rqst->content,rqst->MAXcontentSize,"Not logged in"); rqst->contentSize=strlen(rqst->content); return 0; }

  struct chatConversation conversation;
  char message[MAX_CHAT_MESSAGE]={0};

  if ( ( socialPostedCSRFIsValid(rqst) ) &&
       ( resolveConversation(rqst,username,&conversation) ) &&
       ( _POSTcpy(rqst,"text",message,sizeof(message)) ) && (message[0]!=0) )
  {
    //A log is stored as the HTML that gets served back , so messages are escaped once here on the way in..
    char escapedMessage[MAX_CHAT_MESSAGE*6]={0};
    char escapedUsername[MAX_ESCAPED_USERNAME]={0};
    AmmServer_HTMLEscape(message,escapedMessage,sizeof(escapedMessage));
    AmmServer_HTMLEscape(username,escapedUsername,sizeof(escapedUsername));

    appendMessage(conversation.path,escapedUsername,escapedMessage);
    snprintf(rqst->content,rqst->MAXcontentSize,"Ok");
  } else
  {
    AmmServer_Warning("Discarding chat message from %s",username);
    snprintf(rqst->content,rqst->MAXcontentSize,"Failed");
  }

  rqst->contentSize=strlen(rqst->content);
  return 0;
}


void * createRoom_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char username[MAX_USERNAME]={0};
  if ( ! AmmServer_CurrentUsername(rqst,username,sizeof(username)) ) { socialServeLoginRequired(rqst); return 0; }

  char room[MAX_ROOM_NAME]={0};
  if ( ( socialPostedCSRFIsValid(rqst) ) &&
       ( _POSTcpy(rqst,"name",room,sizeof(room)) ) &&
       ( uadbWeb_isValidUsername(room) ) )
  {
    //Creating a room is creating its log , and opening one that already exists just joins it..
    char path[MAX_FILE_PATH]={0};
    roomPath(room,path,sizeof(path));

    FILE * fp=fopen(path,"a");
    if (fp!=0) { fclose(fp); }

    snprintf(rqst->content,rqst->MAXcontentSize,
             "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
             "<meta http-equiv=\"refresh\" content=\"0; url=chat.html?room=%s\"></head><body></body></html>",room);
  } else
  {
    snprintf(rqst->content,rqst->MAXcontentSize,
             "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
             "<meta http-equiv=\"refresh\" content=\"0; url=chat.html\"></head><body></body></html>");
  }

  rqst->contentSize=strlen(rqst->content);
  return 0;
}


void * chatPicture_callback(struct AmmServer_DynamicRequest  * rqst)
{
  AmmServer_Success("chatPicture_callback done");
  snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Ok</body></html>");
  rqst->contentSize=strlen(rqst->content);
  return 0;
}
