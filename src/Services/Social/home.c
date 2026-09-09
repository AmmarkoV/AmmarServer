#include "home.h"
#include "posts.h"
#include "media.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*Big enough for the MAX_POSTS_RENDERED posts a page ever draws , with their comments..*/
#define POSTS_RENDER_BUFFER_SIZE (512*1024)


/*The home feed and a user's wall are the same page with a different set of posts in it , so they share one
  renderer. `profileUser` is 0 for the feed ( every wall ) , or whose wall is being looked at..*/
static void renderPage(struct AmmServer_DynamicRequest * rqst,const char * viewer,const char * profileUser)
{
  char csrfToken[MAX_CSRF_TOKEN]={0};
  if ( ! AmmServer_GenerateCSRFToken(rqst,csrfToken,sizeof(csrfToken)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Could not issue a security token , please try again.</body></html>");
    rqst->contentSize=strlen(rqst->content);
    return;
  }

  char * postsBuffer = (char*) malloc(POSTS_RENDER_BUFFER_SIZE);
  if (postsBuffer==0) { return; }

  renderPosts(postsBuffer,POSTS_RENDER_BUFFER_SIZE,profileUser,viewer,csrfToken,profileUser);

  char escapedViewer[MAX_ESCAPED_USERNAME]={0};
  AmmServer_HTMLEscape(viewer,escapedViewer,sizeof(escapedViewer));

  //Both pages compose : the feed writes on your own wall , a profile writes on the wall being looked at. The
  //feed sends neither wall nor back - absent means "the author's own wall" and "back to the feed" , and an
  //empty multipart field would make the shared POST parser drop every field after it anyway..
  char body[3072]={0};
  if (profileUser==0)
  {
    snprintf(body,sizeof(body),
             "<form class=\"composer\" method=\"post\" enctype=\"multipart/form-data\" action=\"post.html\" onsubmit=\"return trimEmptyFields(this);\">"
              "<input type=\"hidden\" name=\"csrf\" value=\"%s\">"
              "<input type=\"text\" name=\"text\" maxlength=\"280\" placeholder=\"What is happening , %s ?\">"
              "<button type=\"submit\">Post</button>"
              "<input type=\"file\" name=\"media\" accept=\"image/*,audio/*\" title=\"Attach a picture or a sound\">"
             "</form>",
             csrfToken,escapedViewer);
  } else
  {
    char escapedProfileUser[MAX_ESCAPED_USERNAME]={0};
    AmmServer_HTMLEscape(profileUser,escapedProfileUser,sizeof(escapedProfileUser));

    //Somebody else's wall is where a private conversation with them starts from..
    char messageLink[256]={0};
    if (strcmp(profileUser,viewer)!=0)
      { snprintf(messageLink,sizeof(messageLink),"<a class=\"messagelink\" href=\"chat.html?u=%s\">Message %s</a>",escapedProfileUser,escapedProfileUser); }

    snprintf(body,sizeof(body),
             "<h2 class=\"profiletitle\">%s&apos;s wall%s</h2>"
             "<form class=\"composer\" method=\"post\" enctype=\"multipart/form-data\" action=\"post.html\" onsubmit=\"return trimEmptyFields(this);\">"
              "<input type=\"hidden\" name=\"csrf\" value=\"%s\">"
              "<input type=\"hidden\" name=\"wall\" value=\"%s\">"
              "<input type=\"hidden\" name=\"back\" value=\"%s\">"
              "<input type=\"text\" name=\"text\" maxlength=\"280\" placeholder=\"Write on %s&apos;s wall..\">"
              "<button type=\"submit\">Post</button>"
              "<input type=\"file\" name=\"media\" accept=\"image/*,audio/*\" title=\"Attach a picture or a sound\">"
             "</form>",
             escapedProfileUser,messageLink,csrfToken,escapedProfileUser,escapedProfileUser,escapedProfileUser);
  }

  char chat[512]={0};
  if (profileUser==0)
  {
    snprintf(chat,sizeof(chat),
             "<div class=\"chatcolumn\">"
              "<iframe class=\"chatwindow\" src=\"chat.html?room=lobby\" frameborder=\"0\" scrolling=\"no\" width=\"400\" height=\"500\"></iframe>"
             "</div>");
  }

  snprintf(rqst->content,rqst->MAXcontentSize,
           "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>The Social Gate</title>"
           "<link rel='stylesheet' type='text/css' href='social.css'>"
           "<script type=\"text/javascript\" src=\"social.js\"></script></head><body>"
           "<div class=\"topbar\">"
            "<img src=\"favicon.ico\" class=\"topbarlogo\"/>"
            "<div class=\"topbarlinks\">"
             "<a href=\"home.html\">Home</a>"
             "<a href=\"profile.html?u=%s\">%s</a>"
             "<form method=\"post\" enctype=\"multipart/form-data\" action=\"logout.html\">"
              "<input type=\"hidden\" name=\"csrf\" value=\"%s\">"
              "<button type=\"submit\">Log out</button>"
             "</form>"
            "</div>"
           "</div>"
           "<div class=\"page\"><div class=\"wall\">%s%s</div>%s</div>"
           "</body></html>",
           escapedViewer,escapedViewer,csrfToken,body,postsBuffer,chat);
  rqst->contentSize=strlen(rqst->content);

  free(postsBuffer);
}


void * home_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char viewer[MAX_USERNAME]={0};
  if ( ! AmmServer_CurrentUsername(rqst,viewer,sizeof(viewer)) ) { socialServeLoginRequired(rqst); return 0; }

  renderPage(rqst,viewer,0);
  return 0;
}


void * profile_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char viewer[MAX_USERNAME]={0};
  if ( ! AmmServer_CurrentUsername(rqst,viewer,sizeof(viewer)) ) { socialServeLoginRequired(rqst); return 0; }

  char profileUser[MAX_USERNAME]={0};
  if ( ! _GETcpy(rqst,"u",profileUser,sizeof(profileUser)) ) { snprintf(profileUser,sizeof(profileUser),"%s",viewer); }

  if ( (! uadbWeb_isValidUsername(profileUser)) || (! uadbWeb_userAccountExists(uadb,profileUser)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,
             "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><link rel='stylesheet' type='text/css' href='social.css'></head>"
             "<body><div class=\"notice\">No such user. <a href=\"home.html\">Home</a></div></body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  renderPage(rqst,viewer,profileUser);
  return 0;
}


void * post_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char author[MAX_USERNAME]={0};
  if ( ! AmmServer_CurrentUsername(rqst,author,sizeof(author)) ) { socialServeLoginRequired(rqst); return 0; }

  char backUser[MAX_USERNAME]={0};
  _POSTcpy(rqst,"back",backUser,sizeof(backUser));

  if ( ! socialPostedCSRFIsValid(rqst) ) { socialRedirectBack(rqst,backUser); return 0; }

  //A post lands on its author's own wall unless it names an account that actually has one..
  char wall[MAX_USERNAME]={0};
  _POSTcpy(rqst,"wall",wall,sizeof(wall));
  if ( ( ! uadbWeb_isValidUsername(wall) ) || ( ! uadbWeb_userAccountExists(uadb,wall) ) ) { wall[0]=0; }

  char media[MAX_MEDIA_NAME]={0};
  mediaStoreFromRequest(rqst,"media",media,sizeof(media));

  char text[MAX_POST_TEXT]={0};
  _POSTcpy(rqst,"text",text,sizeof(text));
  addPost(author,wall,text,media); //Refuses on its own if there is neither text nor an attachment

  socialRedirectBack(rqst,backUser);
  return 0;
}


void * comment_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char author[MAX_USERNAME]={0};
  if ( ! AmmServer_CurrentUsername(rqst,author,sizeof(author)) ) { socialServeLoginRequired(rqst); return 0; }

  char backUser[MAX_USERNAME]={0};
  _POSTcpy(rqst,"back",backUser,sizeof(backUser));

  if ( ! socialPostedCSRFIsValid(rqst) ) { socialRedirectBack(rqst,backUser); return 0; }

  char text[MAX_POST_TEXT]={0};
  unsigned int postID=_POSTuint(rqst,"post");
  if ( (postID!=0) && (_POSTcpy(rqst,"text",text,sizeof(text))) ) { addComment(postID,author,text); }

  socialRedirectBack(rqst,backUser);
  return 0;
}


void * like_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char username[MAX_USERNAME]={0};
  if ( ! AmmServer_CurrentUsername(rqst,username,sizeof(username)) ) { socialServeLoginRequired(rqst); return 0; }

  char backUser[MAX_USERNAME]={0};
  _POSTcpy(rqst,"back",backUser,sizeof(backUser));

  if ( ! socialPostedCSRFIsValid(rqst) ) { socialRedirectBack(rqst,backUser); return 0; }

  unsigned int postID=_POSTuint(rqst,"post");
  if (postID!=0) { toggleLike(postID,username); }

  socialRedirectBack(rqst,backUser);
  return 0;
}
