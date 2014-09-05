/*
AmmarServer , HTTP Server Library

URLs: http://ammar.gr
Written by Ammar Qammaz a.k.a. AmmarkoV 2012

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>
#include <unistd.h>

#include "http_tools.h"
#include "logs.h"
#include "../server_configuration.h"
#include "../cache/file_caching.h"

#include "../stringscanners/applicationFiles.h"
#include "../stringscanners/archiveFiles.h"
#include "../stringscanners/imageFiles.h"
#include "../stringscanners/textFiles.h"
#include "../stringscanners/videoFiles.h"
#include "../stringscanners/audioFiles.h"




static char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789"
"+/";

unsigned int ServerThreads_DropRootUID()
{
   if (ENABLE_DROPPING_UID_ALWAYS ) { /* We fall through and change UID , as a mandatory step.. */} else
   if (ENABLE_DROPPING_ROOT_UID_IF_ROOT)
      { /*Check if we are root */
        if (getuid()>=1000) { /*Non root id , we can skip dropping our UID with this configuration..*/ return 0; }
                              //If we fell through it means we are root and dropping root when root is enabled so the code that follows will alter our uid..
      } else
      {
        if (getuid()<1000)
             {
              error("This version of AmmarServer is running with superuser privileges and is compiled not to drop them , this is a serious security liability");
             }
        return 0;
      }

   char command_to_get_uid[MAX_FILE_PATH]={0};
   snprintf(command_to_get_uid,MAX_FILE_PATH,"id -u %s",USERNAME_UID_FOR_DAEMON);


   FILE * fp  = popen(command_to_get_uid, "r");
   if (fp == 0 ) { fprintf(stderr,"Failed to get our user id ( trying to drop root UID ) \n"); return 0; }

   char output[POPEN_BUFFER_SIZE]={0};
   /* Read the output a line at a time - output it. */
     unsigned int i=0;
     while (fgets(output,POPEN_BUFFER_SIZE , fp) != 0) { ++i; /*fprintf(stderr,"\n\nline %u = %s \n",i,output);*/ break; }
    /* close */
     pclose(fp);

   int non_root_uid = atoi(output);
   if (non_root_uid<1000)
      {
        fprintf(stderr,"The user set in USERNAME_UID_FOR_DAEMON=\"%s\" is also root (his uid is %u)\n",USERNAME_UID_FOR_DAEMON,non_root_uid);
        if (CHANGE_TO_UID<1000)
            {
              error("This should never happen , Our CHANGE_TO_UID non-root value is also a super user UID , we are forced to set a bogus non-root value..\n");
              non_root_uid=NON_ROOT_UID_IF_USER_FAILS;
            } else
            {
              non_root_uid=CHANGE_TO_UID;
            }
      }

   fprintf(stderr,"setuid(%d);\n",non_root_uid);
   return setuid(non_root_uid); // Non Root UID :-P
}

char FileExistsAmmServ(const char * filename)
{
 FILE *fp = fopen(filename,"r");
 if( fp ) { /* exists */ fclose(fp); return 1; }
 fprintf(stderr,"FileExists(%s) returns 0\n",filename);
 return 0;
}

char DirectoryExistsAmmServ(const char* dirpath )
{
  if ( dirpath == 0) return 0;
  char bExists = 0;
  DIR *pDir = opendir (dirpath);

  if (pDir != 0 )
    {
      bExists = 1;
      closedir (pDir);
    }

   return bExists;
}



int GetContentTypeForExtension(const char * theextension,char * content_type,unsigned int contentTypeLength)
{
  //fprintf(stderr,"Resolving Extension %s , length %u \n",theextension,theextensionLength );
  if (contentTypeLength<29)
  {
    error("GetContentTypeForExtension called with a very small buffer , we will return");
    fprintf(stderr,"GetContentTypeForExtension called with a very small buffer ( %u bytes )  \n",contentTypeLength );
    return 0;
  }


  unsigned int theextensionLength = strlen(theextension);

  //http://www.iana.org/assignments/media-types/text/index.html
  unsigned int ext=scanFor_textFiles(theextension,theextensionLength);
  switch (ext)
  {
    case TEXTFILES_HTML  :  strcpy(content_type,"text/html"); content_type[9]=0; return 1; break;
    case TEXTFILES_HTM   :  strcpy(content_type,"text/html"); content_type[9]=0; return 1; break;
    case TEXTFILES_CSS   :  strcpy(content_type,"text/css");  content_type[8]=0; return 1; break;
    case TEXTFILES_TXT   :  strcpy(content_type,"text/txt");  content_type[8]=0; return 1; break;
    case TEXTFILES_DOC   :  strcpy(content_type,"text/doc");  content_type[8]=0; return 1; break;
    case TEXTFILES_RTF   :  strcpy(content_type,"text/rtf");  content_type[8]=0; return 1; break;
    case TEXTFILES_ODF   :  strcpy(content_type,"text/odf");  content_type[8]=0; return 1; break;
    case TEXTFILES_ODT   :  strcpy(content_type,"text/odt");  content_type[8]=0; return 1; break;
  };

  //http://www.iana.org/assignments/media-types/image/index.html
  ext=scanFor_imageFiles(theextension,theextensionLength);
  switch (ext)
  {
   case IMAGEFILES_GIF  :  strcpy(content_type,"image/gif");  content_type[9]=0; return 1; break;
   case IMAGEFILES_PNG  :  strcpy(content_type,"image/png");  content_type[9]=0; return 1; break;
   case IMAGEFILES_JPG  :
   case IMAGEFILES_JPEG :  strcpy(content_type,"image/jpg");  content_type[9]=0; return 1; break;
   case IMAGEFILES_WEBP :  strcpy(content_type,"image/webp"); content_type[10]=0; return 1; break;
   case IMAGEFILES_BMP  :  strcpy(content_type,"image/bmp");  content_type[9]=0; return 1; break;
   case IMAGEFILES_TIFF :  strcpy(content_type,"image/tiff"); content_type[10]=0; return 1; break;
   case IMAGEFILES_DIB  :  strcpy(content_type,"image/dib");  content_type[9]=0; return 1; break;
   case IMAGEFILES_RLE  :  strcpy(content_type,"image/rle");  content_type[9]=0; return 1; break;
   case IMAGEFILES_J2C  :  strcpy(content_type,"image/j2c");  content_type[9]=0; return 1; break;
   case IMAGEFILES_ICO  :  strcpy(content_type,"image/ico");  content_type[9]=0; return 1; break;
   case IMAGEFILES_PPM  :  strcpy(content_type,"image/ppm");  content_type[9]=0; return 1; break;
   case IMAGEFILES_PNM  :  strcpy(content_type,"image/pnm");  content_type[9]=0; return 1; break;
   case IMAGEFILES_RAW  :  strcpy(content_type,"image/raw");  content_type[9]=0; return 1; break;
   case IMAGEFILES_SVG  :  strcpy(content_type,"image/svg+xml"); content_type[13]=0; return 1; break;
  };

//http://www.iana.org/assignments/media-types/video/index.html
  ext=scanFor_videoFiles(theextension,theextensionLength);
  switch (ext)
  {
   case VIDEOFILES_AVI      :  strcpy(content_type,"video/mp4");   content_type[9]=0; return 1; break;
   case VIDEOFILES_MPEG4    :  strcpy(content_type,"video/mp4");   content_type[9]=0; return 1; break;
   case VIDEOFILES_MPEG     :  strcpy(content_type,"video/mp4");   content_type[9]=0; return 1; break;
   case VIDEOFILES_MP4      :  strcpy(content_type,"video/mp4");   content_type[9]=0; return 1; break;
   case VIDEOFILES_WEBM     :  strcpy(content_type,"video/webm");  content_type[10]=0; return 1; break;
   case VIDEOFILES_MKV      :  strcpy(content_type,"video/mkv");   content_type[9]=0; return 1; break;
   case VIDEOFILES_3GP      :  strcpy(content_type,"video/3gp");   content_type[9]=0; return 1; break;
   case VIDEOFILES_H263     :  strcpy(content_type,"video/h263");  content_type[10]=0;return 1; break;
   case VIDEOFILES_H264     :  strcpy(content_type,"video/h264");  content_type[10]=0;return 1; break;
   case VIDEOFILES_FLV      :  strcpy(content_type,"video/x-flv"); content_type[11]=0; return 1; break;
  };

//http://www.iana.org/assignments/media-types/audio/index.html
  ext=scanFor_audioFiles(theextension,theextensionLength);
  switch (ext)
  {
   case AUDIOFILES_MP3   :  strcpy(content_type,"audio/mp3");   content_type[9]=0; return 1; break;
   case AUDIOFILES_WAV   :  strcpy(content_type,"audio/wav");   content_type[9]=0; return 1; break;
   case AUDIOFILES_MID   :  strcpy(content_type,"audio/mid");   content_type[9]=0; return 1; break;
   case AUDIOFILES_OGG   :  strcpy(content_type,"audio/ogg");   content_type[9]=0; return 1; break;
   case AUDIOFILES_VOC   :  strcpy(content_type,"audio/voc");   content_type[9]=0; return 1; break;
   case AUDIOFILES_AU    :  strcpy(content_type,"audio/au");    content_type[8]=0; return 1; break;
  };

//http://www.iana.org/assignments/media-types/application/index.html
  ext=scanFor_applicationFiles(theextension,theextensionLength);
  switch (ext)
  {
   case APPLICATIONFILES_EXE  :
   case APPLICATIONFILES_DLL  :
   case APPLICATIONFILES_SCR  :
   case APPLICATIONFILES_CPL  :  strcpy(content_type,"application/exe");                  content_type[15]=0; return 1; break;
   case APPLICATIONFILES_SWF  :  strcpy(content_type,"application/x-shockwave-flash");    content_type[29]=0; return 1; break;
   case APPLICATIONFILES_PDF  :  strcpy(content_type,"application/pdf");                  content_type[15]=0; return 1; break;
  };


  ext=scanFor_archiveFiles(theextension,theextensionLength);
  switch (ext)
  {
   case   ARCHIVEFILES_GZ        :  strcpy(content_type,"application/gzip");    content_type[16]=0; return 1; break;
   case   ARCHIVEFILES_TAR       :  strcpy(content_type,"application/x-tar");   content_type[17]=0; return 1; break;
   case   ARCHIVEFILES_TGZ       :  strcpy(content_type,"application/gnutar");  content_type[18]=0; return 1; break;
   case   ARCHIVEFILES_ZIP       :  strcpy(content_type,"application/zip");     content_type[15]=0; return 1; break;
   case   ARCHIVEFILES_JAR       :
   case   ARCHIVEFILES_7Z        :
   case   ARCHIVEFILES_AR        :
   case   ARCHIVEFILES_BZ2       :
   case   ARCHIVEFILES_CBZ       :
   case   ARCHIVEFILES_CPIO      :
   case   ARCHIVEFILES_ISO       :
   case   ARCHIVEFILES_LZMA      :
   case   ARCHIVEFILES_TAR_7Z    :
   case   ARCHIVEFILES_TAR_Z     :
   case   ARCHIVEFILES_TAR_GZ    :
   case   ARCHIVEFILES_TAR_BZ2   :
   case   ARCHIVEFILES_TAR_BZ    :
   case   ARCHIVEFILES_TAR_LZ    :
   case   ARCHIVEFILES_TAR_LZMA  :
   case   ARCHIVEFILES_TAR_XZ    :
   case   ARCHIVEFILES_XZ        :  strcpy(content_type,"application/octet-stream");      content_type[24]=0; return 1; break;
  };

 fprintf(stderr,"Could not find extension type for extension %s \n",theextension);
 return 0;
}



int GetExtentionType(const char * theextension)
{ //Crude and fast lookup
  if (theextension==0) { return NO_FILETYPE; }
  switch (theextension[0])
  {
   case 0   : return NO_FILETYPE; break;
   case 't' : return TEXT;      break;
   case 'i' : return IMAGE;     break;
   case 'v' : return VIDEO;     break;
   case 'a' :
              if (theextension[1]=='u') { return AUDIO; } else
              if (theextension[1]=='p') { return EXECUTABLE; }
              break;

  };
 //this is made to be used when generating a dynamic directory list to show the appropriate icons..!
 return NO_FILETYPE;
}


void convertToUpperCase(char *sPtr)
{
  while(*sPtr != 0 )
   {
     *sPtr = toupper((unsigned char)*sPtr);
     ++sPtr;
   }
}

int GetContentType(char * filename,char * contentType,unsigned int contentTypeLength)
{
   //fprintf(stderr,"GetContentType ( %s , %s , %u )\n",filename,contentType,contentTypeLength);
   unsigned int filenameLength=strlen(filename);
   if (filenameLength==0) { return 0; }
   unsigned int i=filenameLength-1;

   while ((i>0)&&(filename[i]!='.')) { --i; }
   if (i==0) { return 0; } //<- could not find the content type..
   if (i+1>=filenameLength) { return 0; } //<- found the dot at i BUT it is the last character so no extension is possible..!

   char * start_of_extension = &filename[i+1]; // do not include . ( dot )

   //fprintf(stderr,"START Extension ( %s ) ( last content type %s )\n",start_of_extension,contentType);
   int res=GetContentTypeForExtension((const char*) start_of_extension,contentType,contentTypeLength);
   //fprintf(stderr,"END Extension ( %s ) hints content type %s\n",start_of_extension,contentType);

  return res;
}


int GetExtensionImage(char * filename, char * theimagepath,unsigned int theimagepath_length)
{
   //fprintf(stderr,"GetExtensionImage for %s \n",filename);
   GetContentType(filename,theimagepath,theimagepath_length);
   //fprintf(stderr,"GetExtentionType for %s \n",filename);
   int res=GetExtentionType(theimagepath);
   //fprintf(stderr,"yields %u\n",res);
   switch (res)
   {
     case TEXT       :  snprintf(theimagepath,theimagepath_length,"doc.gif");    break;
     case IMAGE      :  snprintf(theimagepath,theimagepath_length,"img.gif");  break;
     case VIDEO      :  snprintf(theimagepath,theimagepath_length,"vid.gif");  break;
     case AUDIO      :  snprintf(theimagepath,theimagepath_length,"mus.gif");  break;
     case EXECUTABLE :  snprintf(theimagepath,theimagepath_length,"exe.gif");    break;
     default         :  snprintf(theimagepath,theimagepath_length,"dir.gif");  break;
   }
   if ( res == NO_FILETYPE ) { return 0; }
   return 1;
}

int ReducePathSlashes_Inplace(char * filename)
{
  //This piece of code removes excess slashes from resource strings in place
  //For example the string public_html/////test.html will become public_html/test.html
  //While extra slashes cause no side effects on fopen calls etc , they interfere with the
  //hashing functions because public_html//test.html yields a different hash than public_html/test.html ..
  //This in turn means cache misses happening without any reason and the cache growing larger for files already
  //contained in it , that is why it is worth it to reduce slashes for every incoming request even if it means
  //growing the incoming string processing surface ..

  //fprintf(stderr,"ReducePathSlashes_Inplace needs thorough testing %s.. :P\n",filename);
  unsigned int length=strlen(filename);
  unsigned int i=0,offset=0;
     while (i+offset<length)
     {
        if ( filename[i+offset]=='/')
         {
            unsigned int z=i+offset+1;
            while ((filename[z]=='/')&&(z<length))
            {
              ++offset;
              ++z;
            }

            if (z>i+offset+1)
            {
             //We increased the offset ..!
             ++i; // We dont want to hurt the first slash ..!
             //The next character though should be eliminated if offset changed
            }
         }

        if ( (offset>0)&&(i+offset<length) )
         {
            filename[i]=filename[i+offset];
         }
        ++i;
     }

     if (offset>0)
     {
       //We have reduced the total slashes..!
       //Append new null termination
       filename[length-offset]=0;
       //fprintf(stderr,"String shortened to %s.. :P\n",filename);
       return 1;
     }
 return 0;
}

int StripGETRequestQueryAndFragment(char * filename , char * query , unsigned int max_query_length)
{
   //fprintf(stderr,"StripGETRequestQueryAndFragment is not thoroughly tested yet\n");
   unsigned int length=strlen(filename);
   if (length==0) { return 0; }
   unsigned int i=0,fragment_pos=length;

   while ( (i<length) &&  (filename[i]!='?') /*Query start character*/)
    {
        if (filename[i]=='#' /*Fragment start character*/ ) { fragment_pos=i; }
        ++i;
    }

   //fprintf(stderr,"Searching for Query : Request has a %u size , fragment at pos %u/%u \n",length,fragment_pos,length);
   if (fragment_pos<length) { filename[fragment_pos]=0; /*Disregard any fragments , they are for the client only.. */ }
   if (i==length) { return 0; } //<- could not find a ..
   if (i+1>=length) { return 0; } //<- found the question mark at i BUT it is the last character so no extension is possible..!

   char * start_of_query = &filename[i+1]; // do not include ? ( question mark )
   filename[i]=0;

   if (strlen(start_of_query)>=max_query_length) { fprintf(stderr,"Not enough space for GET Request query\n");  return 0; }
   snprintf(query,max_query_length,"%s",start_of_query);

   //fprintf(stderr,"GET Request ( %s ) has a query inlined ( %s ) \n",filename,query);
  return 1;
}

int StripVariableFromGETorPOSTString(char * input,char * var_id, char * var_val , unsigned int var_val_length)
{
  if (var_val==0) { fprintf(stderr,"StripVariableFromGETorPOSTString called with a null output buf\n"); return 0; }
  if (var_val_length==0) { fprintf(stderr,"StripVariableFromGETorPOSTString called with a null output buf size\n");  return 0; }
  var_val[0]=0;

  unsigned int var_id_length = strlen(var_id);

  //fprintf(stderr,"StripVariableFromGETorPOSTString is slopilly implemented \n");
  #warning "StripVariableFromGETorPOSTString is slopilly implemented"
  /**
   @bug StripVariableFromGETorPOSTString does not have a high quality implementation
  */

  /*! TODO : A decent implementation here..! , input is like "idname=idvalue&idname2=idvalue2&idname3=idvalue3" , var_id is the value we are looking for
             var_val is the payload which has space allocated as declared in var_val_length  */

  unsigned int input_length = strlen(input);

  //This part of the code adds a ? or a & prefix to var_id and checks for any instances of this substring
  char * FullVarID = (char*) malloc( ( var_id_length+2 ) * sizeof(char) );
  if (FullVarID==0) { error("Could not allocate FullVarID"); return 0; }
  FullVarID[0]='&';   FullVarID[1]=0; //Initialize first character
  strcat(FullVarID,var_id);
  FullVarID[var_id_length+1]=0;
  //fprintf(stderr,"input=`%s` , VarID=`%s` , FullVarID=`%s`\n",input,var_id,FullVarID);
  char * id_instance = strstr (input,FullVarID);
  free(FullVarID);
  FullVarID=0;
  //---------------------

  if (id_instance==0)
   {
     if (strncmp(input,var_id,var_id_length) ==0 )
     {
       //It is the first variable!
       if (input[var_id_length]=='=')
       {
        fprintf(stderr,"It is the first variable\n"); //,input,var_id,FullVarID
        id_instance=input;
       }
     }
   }


  if (id_instance!=0)
   {
     fprintf(stderr,"Found var_id %s in GET/POST String \n",var_id);
     unsigned int start_of_var_val=id_instance-input;
     unsigned int total_chars_to_copy=0;
     start_of_var_val+=strlen(var_id); // We go right at the = char!
     while ( (input[start_of_var_val]!='=') && (start_of_var_val<input_length) ) {  ++start_of_var_val; } //We seek the = sign

     if ( input[start_of_var_val]=='=') { ++start_of_var_val; } else
     if ( input[start_of_var_val]!='=') { fprintf(stderr,"Error Parsing GET/POST var string\n"); }
     unsigned int i=start_of_var_val;
     while ( i < input_length )
       {
          if (input[i]==0)    {  total_chars_to_copy = i-start_of_var_val; break; } else
          if (input[i]=='&')  {  total_chars_to_copy = i-start_of_var_val; break; }
          ++i;
       }
     if (i>=input_length) { fprintf(stderr,"This is the last arg ? , we finished the string! \n");
                             total_chars_to_copy = input_length-start_of_var_val; }


     if (total_chars_to_copy==0) { fprintf(stderr,"VAR `%s` was empty\n",var_id); return 0; } else
     if (total_chars_to_copy < var_val_length-1) //We want to include a null terminator
                                 {
                                  char * val_start_on_input = input + start_of_var_val;
                                  strncpy(var_val,val_start_on_input,total_chars_to_copy);
                                  var_val[total_chars_to_copy]=0;

                                  fprintf(stderr,"Found VAR `%s` value `%s` \n",var_id,var_val);
                                  return 1;
                                 } else
                                 {
                                  fprintf(stderr,"There was not sufficient space to copy back the value of VAR `%s` \n",var_id);
                                  fprintf(stderr,"The VAR `%s` had a size of %u bytes , we had %u bytes to accomodate it \n",var_id,total_chars_to_copy,var_val_length);
                                  return 0;
                                 }

   }
  //If we are here this means we couldnt find an instance of our var in the input string..!
  //This spams output a lot , so it is disabled -> fprintf(stderr,"Could not find VAR %s \n",var_id);
  return 0;
}


int StripHTMLCharacters_Inplace(char * filename,int enable_security)
{
  //This piece of code converts characters like %20 to their ASCII equivalent " " ( for example )
  //It is not a full mapping of HTML characters to ASCII characters since ( for security reasons )
  //We dont want a full mapping.. :P , We just want to convert characters like spaces plus symbols etc
  //that are common in the internet to try and maintain some level of compatibility with such weird filenames
  //Of course I don't know why anyone would want to name his file "index of-website+.html" but.. :P

  unsigned int length=strlen(filename);
  unsigned int offset=0;

  //The following 2 lines of checks are kind of redundant but if we want all % symbols stripped they have to be here..
  /*1*/   if (length==1) { if (filename[0]=='%') { filename[0]=(char) '_'; } }
  /*2*/   if (length==2) { if (filename[0]=='%') { filename[0]=(char) '_'; } if (filename[1]=='%') { filename[1]= (char) '_'; }  }

  //This line will return if the input is very small
  if (length<=2) { return 0; }

  unsigned int i=0;
  while (i<length)
     {
        if ( (filename[i+offset]=='%') && (i+offset<length-2) )
         {
           unsigned char ascii_val=0,sec_byte=0;
           unsigned char sub_valNumb=0,sub_valHex=0;

           unsigned int A=i+offset+1; // The first byte (A) of hex %AB
           if ((filename[A]>='0')&&(filename[A]<='9'))  { sub_valNumb=filename[A]-'0'; ascii_val+=sub_valNumb*16; }  else
           if ((filename[A]>='a')&&(filename[A]<='f'))  { sub_valHex=filename[A]-'a';  ascii_val+=(10+sub_valHex)*16; }   else
           if ((filename[A]>='A')&&(filename[A]<='F'))  { sub_valHex=filename[A]-'A';  ascii_val+=(10+sub_valHex)*16; }   else
                                                        { sec_byte=1; } //This byte is out of 0-F range so we trigger an alarm


           unsigned int B=i+offset+2; // The second byte (B) of hex %AB
           if ((filename[B]>='0')&&(filename[B]<='9'))  { sub_valNumb=filename[B]-'0'; ascii_val+=sub_valNumb*1; }  else
           if ((filename[B]>='a')&&(filename[B]<='f'))  { sub_valHex=filename[B]-'a';  ascii_val+=(10+sub_valHex)*1; }   else
           if ((filename[B]>='A')&&(filename[B]<='F'))  { sub_valHex=filename[B]-'A';  ascii_val+=(10+sub_valHex)*1; }   else
                                                        { sec_byte=1; } //This byte is out of 0-F range so we trigger an alarm

          if (enable_security)
          { //Security to filter out possibly unwanted bytes..!! , bytes larger than 255 are always filtered since the sec_byte
            //can be also triggered by %ZZ or any ascii value out of 0-F for ( see code above ) ..!
           if ( ascii_val<' ')   { sec_byte=1; } else /* See IGNORED Control characters..! */
           if ( ascii_val>'~')   { sec_byte=1; }
          }

           if (sec_byte)
            {
              fprintf(stderr,"BAD Hex URI Char %% %c %c attempted overflow ( %u ) \n",filename[i+1],filename[i+2],ascii_val);
              //We mute the % character that caused the problem..!
              filename[i]='_';
            } else
            {
              //fprintf(stderr,"Hex URI Char %%%c%c = ascii %u = `%c`\n",filename[i+1],filename[i+2],ascii_val,ascii_val);
              offset+=2; //We keep 1 of the 3 charcters so offset is 2!
              filename[i]=ascii_val;
            }
         }
        ++i;
        if (offset!=0) {  filename[i]=filename[i+offset]; }
     }

  if (offset!=0) {  filename[i]=0; /*Append null termination..!*/ }
  //fprintf(stderr,"\nStripHTMLCharacters_Inplace produced %s ..\n",filename);


  return 0;
}

/* IGNORED Control characters by nStripHTMLCharacters_Inplace ..!
NUL 	null character 	%00
SOH 	start of header 	%01
STX 	start of text 	%02
ETX 	end of text 	%03
EOT 	end of transmission 	%04
ENQ 	enquiry 	%05
ACK 	acknowledge 	%06
BEL 	bell (ring) 	%07
BS 	backspace 	%08
HT 	horizontal tab 	%09
LF 	line feed 	%0A
VT 	vertical tab 	%0B
FF 	form feed 	%0C
CR 	carriage return 	%0D
SO 	shift out 	%0E
SI 	shift in 	%0F
DLE 	data link escape 	%10
DC1 	device control 1 	%11
DC2 	device control 2 	%12
DC3 	device control 3 	%13
DC4 	device control 4 	%14
NAK 	negative acknowledge 	%15
SYN 	synchronize 	%16
ETB 	end transmission block 	%17
CAN 	cancel 	%18
EM 	end of medium 	%19
SUB 	substitute 	%1A
ESC 	escape 	%1B
FS 	file separator 	%1C
GS 	group separator 	%1D
RS 	record separator 	%1E
US 	unit separator 	%1F
*/


int FilenameStripperOk(char * filename)
{
   if (filename==0) { return 0; }

   //This function checks for paths to ensure that fopen will be handed a safe file path..!
   unsigned int length=strlen(filename);
   if (length==0) { return 0; }

   unsigned int i=length-1;

   unsigned int back_slashes_number=0;

   while (i>0)
     {
        //We only accept english strings
        if ( ( filename[i]<' ') || ( filename[i]>'~') )
             { return 0; }

        //We have imposed a limit on slash number..!
        switch (filename[i])
        {
          case '/' :  ++back_slashes_number;
                      if (back_slashes_number>MAX_RESOURCE_SLASHES)  { return 0; }
                      break;

          case '\\' : return 0; break;
          //We dont like /../ ... etc in paths..!
          case '.' :   if ( filename[i-1]=='.') {return 0; } break;
        };

        --i;
     }

   return 1;
}


int strToUpcase(char * strTarget , char * strSource , unsigned int strLength)
{
 unsigned int i=0;
 while (i < strLength)
 {
   strTarget[i] = toupper( strSource[i] );
   ++i;
 }
 return 1;
}


inline int stristr(char * str1CAPS,unsigned int str1_length,char * str2CAPS,unsigned int str2_length,unsigned int * pos_found)
{
  if (str1_length<str2_length) { return 0; }
  unsigned int str1_i=0,str2_i=0;
  while (str1_i<str1_length)
  {
    if (str1CAPS[str1_i]==str2CAPS[str2_i]) { ++str2_i; } else
                                            { str2_i=0; }

    if (str2_i==str2_length)  { *pos_found=str1_i; return 1; }
    ++str1_i;
  }
  return 0;
}


inline int stristr2Caps(char * str1,unsigned int str1_length,char * str2CAPS,unsigned int str2_length,unsigned int * pos_found)
{
  if (str1_length<str2_length) { return 0; }
  unsigned int str1_i=0,str2_i=0;
  while (str1_i<str1_length)
  {
    if (toupper(str1[str1_i])==str2CAPS[str2_i]) { ++str2_i; } else
                                                 { str2_i=0; }

    if (str2_i==str2_length)  { *pos_found=str1_i; return 1; }
    ++str1_i;
  }
  return 0;
}

int trim_last_empty_chars(char * input,unsigned int input_length)
{
   unsigned int i=input_length;
   while (i>0)
    {
         --i; // <-- before the next line for a reason :P
         if (input[i]<'!') { input[i]=0; }
    }
   return 1;
}


int seek_non_blank_char(char * input,char * input_end)
{
   char * ptr=input;
   while (ptr<input_end)
    {
         ++ptr; // <-- before the next line for a reason :P
         if (*ptr>='!') { return ptr-input; }
    }
   return 0;
}

int seek_blank_char(char * input,char * input_end)
{
   char * ptr=input;
   while (ptr<input_end)
    {
         ++ptr; // <-- before the next line for a reason :P
         if (*ptr<'!') { return ptr-input; }
    }
   return 0;
}

unsigned int GetIntFromHTTPHeaderFieldPayload(char * request,unsigned int request_length)
{
   /*                                                             char * request should initally point here ( at the `:` )
                                                                              ||
                                                                              \/
           The line we are trying to analyze looks like this -> Content-Length: NUMBERHERE   <cr><lf> <-*/

        //STEP 1 : We are going to make a null teriminated string called "payload" inside the request string by getting the first blank or <cr> or <lf> character after the NUMBERHERE and making it null
        //STEP 2 : We will use payload like a regular nullterminated string and call atoi on it
        //STEP 3 : After processing we will turn it back to its former value in order to preserve the header line intact..
        //It is kind of confusing but definately the fastest way to do without meaningless string copying around..

        char * payload = request;
        char * payload_end = request+request_length;

        unsigned int blank_offset = seek_non_blank_char(payload,payload_end);
        if (blank_offset>0)
         {
           fprintf(stderr,"Got an offset of %u chars while seeking non_blank characters\n",blank_offset);
           payload+=blank_offset;
           blank_offset = seek_blank_char(payload,payload_end);
             if (blank_offset>0)
              {
                fprintf(stderr,"Got an offset of %u chars while seeking for a blank character\n",blank_offset);
                char * formerly_blank_char = payload+blank_offset;
                char   formerly_blank_char_val = *formerly_blank_char;

                *formerly_blank_char = 0 ; //It became a null terminated string now , efficiency ftw :P
                fprintf(stderr,"Payload is %s (string)\n",payload);
                unsigned int payload_in_uint_form = atoi(payload);
                fprintf(stderr,"Payload is %u (int)\n",payload_in_uint_form);
                *formerly_blank_char = formerly_blank_char_val; //It came back to normal..
                return payload_in_uint_form;
              }
         }
    return 0;
}

char * GetNewStringFromHTTPHeaderFieldPayload(char * request,unsigned int request_length)
{
    int PRINT_HELP_HERE=0;
    if (PRINT_HELP_HERE) fprintf(stderr,"Testing Version of GetNewStringFromHTTPHeaderFieldPayload(%s,%u) called\n",request,request_length);
    /*THIS IS NOT CORRECT , IT NEEDS TO BE FIXED..!*/

   /*                                                             char * request should initally point here ( at the `:` )
                                                                              ||
                                                                              \/
           The line we are trying to analyze looks like this -> Content-Length: STRINGMPLAMLPAMLPAHERE   <cr><lf> <-*/

        //STEP 1 : We are going to make a null teriminated string called "payload" inside the request string by getting the first blank or <cr> or <lf> character after the STRINGMPLAMLPAMLPAHERE and making it null
        //STEP 2 : We will use payload like a regular nullterminated string , malloc a new string , strncpy payload in it
        //STEP 3 : After processing we will turn it back to its former value in order to preserve the header line intact..
        //It is kind of confusing but definately the fastest way to do without meaningless string copying around..

        char * payload = request;
        char * payload_end = request+request_length;

        unsigned int blank_offset = seek_non_blank_char(payload,payload_end);
        if (blank_offset>0)
         {
           if (PRINT_HELP_HERE) fprintf(stderr,"Got an offset of %u chars while seeking non_blank characters\n",blank_offset);
           payload+=blank_offset;
           blank_offset = seek_blank_char(payload,payload_end);
             if (blank_offset>0)
              {
                if (PRINT_HELP_HERE) fprintf(stderr,"Got an offset of %u chars while seeking for a blank character\n",blank_offset);
                char * formerly_blank_char = payload+blank_offset;
                char   formerly_blank_char_val = *formerly_blank_char;

                *formerly_blank_char = 0 ; //It became a null terminated string now , efficiency ftw :P
                if (PRINT_HELP_HERE) fprintf(stderr,"Payload is %s (string)\n",payload);

                                                          //also add some space for the null termination..!
                unsigned int payload_length = strlen(payload);
                char * new_allocation = (char *) malloc ((payload_length+1 /*Null Termination*/)*sizeof(char));
                if (new_allocation!=0)
                  {
                    strncpy(new_allocation,payload,payload_length);
                    new_allocation[payload_length]=0; //<- Null termination should automatically (via strncpy ) be there BUT I am trying to debug some weird behaviour..!
                  }


                *formerly_blank_char = formerly_blank_char_val; //It came back to normal..

                return new_allocation;
              }
         }
    return 0;
}


/*
** ENCODE RAW into BASE64
*/

/* Found here : http://www.velocityreviews.com/forums/t516606-sample-for-base64-encoding-in-c-language.html
   Encode source from raw data into Base64 encoded string */
int encodeToBase64(char *src,unsigned s_len,char *dst,unsigned d_len)
{
  //This function doesnt seem to append a proper null termination ..
  //So lets preemtively clear everything..!
  memset(dst,0,d_len);

  unsigned triad;
   for (triad = 0; triad < s_len; triad += 3)
   {

     unsigned long int sr=0; //Den itan = 0
     unsigned byte=0;         //Den itan = 0

     for (byte = 0; (byte<3)&&(triad+byte<s_len); ++byte)
        {
          sr <<= 8;
          sr |= (*(src+triad+byte) & 0xff);
        }

     sr <<= (6-((8*byte)%6))%6; /*shift left to next 6bit alignment*/

     if (d_len < 4) return 0; /* error - dest too short */

     *(dst+0) = *(dst+1) = *(dst+2) = *(dst+3) = '=';

     switch(byte)
        {
           case 3: *(dst+3) = base64[sr&0x3f];
                    sr >>= 6;
           case 2: *(dst+2) = base64[sr&0x3f];
                    sr >>= 6;
           case 1: *(dst+1) = base64[sr&0x3f];
                    sr >>= 6;

           *(dst+0) = base64[sr&0x3f];
         }
     dst += 4; d_len -= 4;
  }

return 1;
}

int CheckHTTPHeaderCategoryAllCaps(char * lineCAPS,unsigned int line_length,char * potential_strCAPS,unsigned int * payload_start)
{
  return stristr(lineCAPS,line_length,potential_strCAPS,strlen(potential_strCAPS),payload_start);
}

int CheckHTTPHeaderCategory(char * line,unsigned int line_length,char * potential_strCAPS,unsigned int * payload_start)
{
  return stristr2Caps(line,line_length,potential_strCAPS,strlen(potential_strCAPS),payload_start);
}

int FindIndexFile(struct AmmServer_Instance * instance,char * webserver_root,char * directory,char * indexfile,unsigned int indexFileLength)
{
  unsigned int unused=0;
  //TODO : This code can become much better and avoid re making all the strings again and again and again..
  snprintf(indexfile,indexFileLength,"%s%sindex.html",webserver_root,directory); ReducePathSlashes_Inplace(indexfile);
  //strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"index.html"); ReducePathSlashes_Inplace(indexfile);
  if ((cache_FindResource(instance,indexfile,&unused))||(FileExistsAmmServ(indexfile))) { return 1; }

  snprintf(indexfile,indexFileLength,"%s%sindex.htm",webserver_root,directory); ReducePathSlashes_Inplace(indexfile);
  //strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"index.htm");  ReducePathSlashes_Inplace(indexfile);// <- TODO : notice that i can just change the extension to reduce copying around
  if ((cache_FindResource(instance,indexfile,&unused))||(FileExistsAmmServ(indexfile))) { return 1; }

  //strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"home.htm");   ReducePathSlashes_Inplace(indexfile); // <- TODO : notice that i can just change the extension to reduce copying around
  //if ((cache_FindResource(instance,indexfile,&unused))||(FileExistsAmmServ(indexfile))) { return 1; }
  //strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"home.html");  ReducePathSlashes_Inplace(indexfile);// <- TODO : notice that i can just change the extension to reduce copying around
  //if ((cache_FindResource(instance,indexfile,&unused))||(FileExistsAmmServ(indexfile))) { return 1; }
  //strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"index.php");  ReducePathSlashes_Inplace(indexfile);// <- TODO : notice that i can just change the extension to reduce copying around
  //if ((cache_FindResource(instance,indexfile,&unused))||(FileExistsAmmServ(indexfile))) { return 1; }

  indexfile[0]=0;
  return 0;
}



char * RequestHTTPWebPage(char * hostname,unsigned int port,char * filename,unsigned int max_content)
{
  int sockfd;
  struct hostent *he=0;
  struct sockaddr_in their_addr;

  if ((he=gethostbyname(hostname)) == 0) { fprintf(stderr,"Error getting host (%s) by name \n",hostname); return 0; } else
                                        { printf("Client-The remote host is: %s\n",hostname); }

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { error("Could not create the socket for the connection\n"); return 0; } else
                                                        { printf("Client socket ok\n"); }

    // host byte order
    their_addr.sin_family = AF_INET;
    // short, network byte order
    printf("Server-Using %s:%u...\n",hostname,port);
    their_addr.sin_port = htons(port);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    // zero the rest of the struct
    memset(&(their_addr.sin_zero), 0 , 8); // '\0'

   if(connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) { error("Could not connect the created socket \n");  return 0; } else
                                                                                         { printf("Starting Request for filename %s \n",filename); }


    struct timeval timeout;
    timeout.tv_sec = (unsigned int) 1/1000; timeout.tv_usec = 0;
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0) { fprintf(stderr,"Warning : Could not set socket Receive timeout \n"); }

    timeout.tv_sec = (unsigned int) 1/1000; timeout.tv_usec = 0;
    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0) { fprintf(stderr,"Warning : Could not set socket Send timeout \n"); }



    char * buffer = (char*) malloc( (max_content+1) * sizeof(char) );
    if (buffer!=0)
    {
      snprintf(buffer,max_content,"GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n",filename,hostname);
      int opres =  send(sockfd,buffer,strlen(buffer),MSG_WAITALL|MSG_NOSIGNAL);  //Send filesize as soon as we've got it

      if (opres<=0) { fprintf(stderr,"Error Sending Request data\n"); } else
      {
        buffer[0]=0;
        opres = recv(sockfd,buffer,max_content,MSG_WAITALL|MSG_NOSIGNAL);
        if (opres<=0) { fprintf(stderr,"Error Receiving Request data\n"); }
      }

      free(buffer);
    }

    close(sockfd);

  /** @bug : Check for success or failure on RequestHTTPWebPage and return an appropriate return value */
  return 0;
}



int freeString(char ** str)
{
   if (*str!=0)
      {
         free(*str);
         *str=0;
         return 1;
      }

   return 0;
}



int setSocketTimeouts(int clientSock)
{
 int errorSettingTimeouts = 1;
 struct timeval timeout; //We dont need to initialize here , since we initialize on the next step
 timeout.tv_sec = (unsigned int) varSocketTimeoutREAD_seconds; timeout.tv_usec = 0;
 if (setsockopt (clientSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0) { warning("Could not set socket Receive timeout \n"); errorSettingTimeouts=0; }

 timeout.tv_sec = (unsigned int) varSocketTimeoutWRITE_seconds; timeout.tv_usec = 0;
 if (setsockopt (clientSock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0) { warning("Could not set socket Send timeout \n"); errorSettingTimeouts=0; }

 return errorSettingTimeouts;
}



clientID findOutClientIDOfPeer(struct AmmServer_Instance * instance , int clientSock)
{
  //Lets find out who we are talking to , and if we want to deny him service or not..!
  socklen_t len=0;
  struct sockaddr_storage addr;
  char ipstr[INET6_ADDRSTRLEN]; ipstr[0]=0;
  int port=0;

  len = sizeof addr;
  if ( getpeername(clientSock, (struct sockaddr*)&addr, &len) == 0 )
 {
  // deal with both IPv4 and IPv6:
  if (addr.ss_family == AF_INET)
  {
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
    port = ntohs(s->sin_port);
    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
  } else
  { // AF_INET6
    struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
    port = ntohs(s->sin6_port);
    inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
  }
  fprintf(stderr,"Peer IP address: %s , port %d \n", ipstr,port);
 } else
 {
   warning("Could not get peer name..!"); //This could be a reason to drop this connection!
   return 0;
 }

 return  clientList_GetClientId(instance->clientList,"0.0.0.0"); // <- TODO add IPv4 , IPv6 IP here
}


