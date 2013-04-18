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
#include "../server_configuration.h"
#include "../cache/file_caching.h"




static char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789"
"+/";



/*
red=$(printf "\033[31m")
green=$(printf "\033[32m")
yellow=$(printf "\033[33m")
blue=$(printf "\033[34m")
magenta=$(printf "\033[35m")
cyan=$(printf "\033[36m")
white=$(printf "\033[37m")
normal=$(printf "\033[m")

normalChars=$(printf "\033[0m")
boldChars=$(printf "\033[1m")
underlinedChars=$(printf "\033[4m")
blinkingChars=$(printf "\033[5m")
*/


void error(char * msg)
{

 fprintf(stderr,"\033[31mERROR MESSAGE : %s\n\033[0m",msg);
 return;
}

char FileExistsAmmServ(char * filename)
{
 FILE *fp = fopen(filename,"r");
 if( fp ) { /* exists */ fclose(fp); return 1; }
 fprintf(stderr,"FileExists(%s) returns 0\n",filename);
 return 0;
}

char DirectoryExistsAmmServ( char* dirpath )
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



int GetContentTypeForExtension(char * theextension,char * content_type)
{
//TODO: This should be replaced with an extension hashmap with binary search going on to strcpy the correct content-type..!


//http://www.iana.org/assignments/media-types/image/index.html
if (strcmp(theextension,"GIF")==0)  { strcpy(content_type,"image/gif"); return 1; } else
if (strcmp(theextension,"PNG")==0)  { strcpy(content_type,"image/png"); return 1; } else
if (strcmp(theextension,"JPG")==0)  { strcpy(content_type,"image/jpg"); return 1; } else
if (strcmp(theextension,"JPEG")==0) { strcpy(content_type,"image/jpg"); return 1; } else
if (strcmp(theextension,"BMP")==0)  { strcpy(content_type,"image/bmp"); return 1; } else
if (strcmp(theextension,"TIFF")==0) { strcpy(content_type,"image/tiff"); return 1; } else
if (strcmp(theextension,"DIB")==0)  { strcpy(content_type,"image/dib"); return 1; } else
if (strcmp(theextension,"RLE")==0)  { strcpy(content_type,"image/rle"); return 1; } else
if (strcmp(theextension,"J2C")==0)  { strcpy(content_type,"image/j2c"); return 1; } else
if (strcmp(theextension,"ICO")==0)  { strcpy(content_type,"image/ico"); return 1; } else
if (strcmp(theextension,"SVG")==0)  { strcpy(content_type,"image/svg+xml"); return 1; } else

//http://www.iana.org/assignments/media-types/application/index.html
if ((strcmp(theextension,"EXE")==0)||(strcmp(theextension,"DLL")==0)||(strcmp(theextension,"CPL")==0)||(strcmp(theextension,"SCR")==0)) { strcpy(content_type,"application/exe"); return 1; } else
if (strcmp(theextension,"SWF")==0)  { strcpy(content_type,"application/x-shockwave-flash"); return 1; } else
if (strcmp(theextension,"PDF")==0) { strcpy(content_type,"application/pdf"); return 1; } else

//http://www.iana.org/assignments/media-types/video/index.html
if (strcmp(theextension,"AVI")==0) { strcpy(content_type,"video/mp4"); return 1; } else
if (strcmp(theextension,"MPEG4")==0) { strcpy(content_type,"video/mp4"); return 1; } else
if (strcmp(theextension,"MPEG")==0) { strcpy(content_type,"video/mp4"); return 1; } else
if (strcmp(theextension,"MP4")==0) { strcpy(content_type,"video/mp4"); return 1; } else
if (strcmp(theextension,"MKV")==0) { strcpy(content_type,"video/mkv"); return 1; } else
if (strcmp(theextension,"3GP")==0) { strcpy(content_type,"video/3gp"); return 1; } else
if (strcmp(theextension,"H263")==0) { strcpy(content_type,"video/h263"); return 1; } else
if (strcmp(theextension,"H264")==0) { strcpy(content_type,"video/h264"); return 1; } else
if (strcmp(theextension,"FLV")==0) { strcpy(content_type,"video/x-flv"); return 1; } else

//http://www.iana.org/assignments/media-types/audio/index.html
if (strcmp(theextension,"MP3")==0) { strcpy(content_type,"audio/mp3"); return 1; } else
if (strcmp(theextension,"WAV")==0) { strcpy(content_type,"audio/wav"); return 1; } else
if (strcmp(theextension,"MID")==0) { strcpy(content_type,"audio/wav"); return 1; } else
if (strcmp(theextension,"OGG")==0) { strcpy(content_type,"audio/ogg"); return 1; } else
if (strcmp(theextension,"VOC")==0) { strcpy(content_type,"audio/voc"); return 1; } else
if (strcmp(theextension,"AU")==0)  { strcpy(content_type,"audio/au"); return 1; } else

//http://www.iana.org/assignments/media-types/text/index.html
if (strcmp(theextension,"HTML")==0) { strcpy(content_type,"text/html"); return 1; } else
if (strcmp(theextension,"HTM")==0) { strcpy(content_type,"text/html"); return 1; } else
if (strcmp(theextension,"CSS")==0) { strcpy(content_type,"text/css"); return 1; } else
if (strcmp(theextension,"TXT")==0) { strcpy(content_type,"text/txt"); return 1; } else
if (strcmp(theextension,"DOC")==0) { strcpy(content_type,"text/doc"); return 1; } else
if (strcmp(theextension,"RTF")==0) { strcpy(content_type,"text/rtf"); return 1; } else
if (strcmp(theextension,"ODF")==0) { strcpy(content_type,"text/odf"); return 1; } else
if (strcmp(theextension,"ODT")==0) { strcpy(content_type,"text/odt"); return 1; }

fprintf(stderr,"Could not find extension type for extension %s \n",theextension);

 return 0;
}



int GetExtentionType(char * theextension)
{

 //Crude and fast lookup
 if (theextension[0]=='t') { return TEXT; } else
 if (theextension[0]=='i') { return IMAGE; } else
 if (theextension[0]=='v') { return VIDEO; } else
 if ((theextension[0]=='a')&&(theextension[1]=='u')) { return AUDIO; } else
 if ((theextension[0]=='a')&&(theextension[1]=='p')) { return EXECUTABLE; }
 //this is made to be used when generating a dynamic directory list to show the appropriate icons..!


 //fprintf(stderr,"GetExtentionType(%s)\n",theextension);
 //fprintf(stderr,"No file type\n",theextension);
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

int GetContentType(char * filename,char * content_type)
{
   unsigned int length=strlen(filename);
   if (length==0) { return 0; }
   unsigned int i=length-1;

   while ((i>0)&&(filename[i]!='.')) { --i; }
   if (i==0) { return 0; } //<- could not find the content type..
   if (i+1>=length) { return 0; } //<- found the dot at i BUT it is the last character so no extension is possible..!

   char extension[MAX_FILE_PATH_EXTENSION_SIZE]={0};
   char * start_of_extension = &filename[i+1]; // do not include . ( dot )

   //fprintf(stderr,"Extension for filename %s comes after character %u ( it is %s ) \n",filename,i,start_of_extension);

   sprintf(extension,"%s",start_of_extension);
   convertToUpperCase(extension);
   int res=GetContentTypeForExtension(extension,content_type);
   fprintf(stderr,"Extension ( %s ) hints content type %s\n",extension,content_type);

  return res;
}


int GetExtensionImage(char * filename, char * theimagepath,unsigned int theimagepath_length)
{
  // fprintf(stderr,"GetExtensionImage for %s ",filename);
   int res=GetContentType(filename,theimagepath);
        res=GetExtentionType(theimagepath);
  // fprintf(stderr,"yields %u\n",res);

   if (res==TEXT)       { strncpy(theimagepath,"fdoc.gif",theimagepath_length);   } else
   if (res==IMAGE)      { strncpy(theimagepath,"fpaint.gif",theimagepath_length); } else
   if (res==VIDEO)      { strncpy(theimagepath,"fvideo.gif",theimagepath_length); } else
   if (res==AUDIO)      { strncpy(theimagepath,"fmusic.gif",theimagepath_length); } else
   if (res==EXECUTABLE) { strncpy(theimagepath,"fexe.gif",theimagepath_length);   } else
                        { strncpy(theimagepath,"folder.gif",theimagepath_length); }

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
   sprintf(query,"%s",start_of_query);

   //fprintf(stderr,"GET Request ( %s ) has a query inlined ( %s ) \n",filename,query);
  return 1;
}

int StripVariableFromGETorPOSTString(char * input,char * var_id, char * var_val , unsigned int var_val_length)
{
  if (var_val==0) { fprintf(stderr,"StripVariableFromGETorPOSTString called with a null output buf\n"); return 0; }
  if (var_val_length==0) { fprintf(stderr,"StripVariableFromGETorPOSTString called with a null output buf size\n");  return 0; }
  var_val[0]=0;

  fprintf(stderr,"StripVariableFromGETorPOSTString is slopilly implemented \n");
  /*! TODO : A decent implementation here..! , input is like "idname=idvalue&idname2=idvalue2&idname3=idvalue3" , var_id is the value we are looking for
             var_val is the payload which has space allocated as declared in var_val_length  */

  unsigned int input_length = strlen(input);
  char * id_instance = strstr (input,var_id);
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
  fprintf(stderr,"Could not find VAR %s \n",var_id);
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
  /*1*/   if (length==1) { if (filename[0]=='%') { filename[0]=(char) "_"; } }
  /*2*/   if (length==2) { if (filename[0]=='%') { filename[0]=(char) "_"; } if (filename[1]=='%') { filename[1]= (char) "_"; }  }

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
        //We have imposed a limit on slash number..!
        if ( filename[i]=='/')  { ++back_slashes_number; }
        if (back_slashes_number>MAX_RESOURCE_SLASHES)  { return 0; }

        //We only accept english strings
        if ( filename[i]<' ')   { return 0; } else
        if ( filename[i]=='\\')   { return 0; } else //Front slashes (escape characters ) denied completely.!
        if ( filename[i]>'~')   { return 0; } else

        //We dont like /../ ... etc in paths..!
        if ( ( filename[i-1]=='.')&&( filename[i]=='.') ) { return 0; }

        --i;
     }

   return 1;
}


int stristr2Caps(char * str1,unsigned int str1_length,char * str2CAPS,unsigned int str2_length,unsigned int * pos_found)
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
     unsigned long int sr;
     unsigned byte;

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


int CheckHTTPHeaderCategory(char * line,unsigned int line_length,char * potential_strCAPS,unsigned int * payload_start)
{
  return stristr2Caps(line,line_length,potential_strCAPS,strlen(potential_strCAPS),payload_start);
}

int FindIndexFile(struct AmmServer_Instance * instance,char * webserver_root,char * directory,char * indexfile)
{
  unsigned int unused=0;
  //TODO : This code can become much better and avoid re making all the strings again and again and again..
  strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"index.html"); ReducePathSlashes_Inplace(indexfile);
  if ((Find_CacheItem(instance,indexfile,&unused))||(FileExistsAmmServ(indexfile))) { return 1; }
  strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"index.htm");  ReducePathSlashes_Inplace(indexfile);// <- TODO : notice that i can just change the extension to reduce copying around
  if ((Find_CacheItem(instance,indexfile,&unused))||(FileExistsAmmServ(indexfile))) { return 1; }
  strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"home.htm");   ReducePathSlashes_Inplace(indexfile); // <- TODO : notice that i can just change the extension to reduce copying around
  if ((Find_CacheItem(instance,indexfile,&unused))||(FileExistsAmmServ(indexfile))) { return 1; }
  strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"home.html");  ReducePathSlashes_Inplace(indexfile);// <- TODO : notice that i can just change the extension to reduce copying around
  if ((Find_CacheItem(instance,indexfile,&unused))||(FileExistsAmmServ(indexfile))) { return 1; }
  strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"index.php");  ReducePathSlashes_Inplace(indexfile);// <- TODO : notice that i can just change the extension to reduce copying around
  if ((Find_CacheItem(instance,indexfile,&unused))||(FileExistsAmmServ(indexfile))) { return 1; }

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
    printf("Server-Using %s:%d...\n",hostname,port);
    their_addr.sin_port = htons(port);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    // zero the rest of the struct
    memset(&(their_addr.sin_zero), '\0', 8);

   if(connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) { error("Could not connect the created socket \n");  return 0; } else
                                                                                         { printf("Starting Request for filename %s \n",filename); }



    struct timeval timeout;
    timeout.tv_sec = (unsigned int) 1/1000; timeout.tv_usec = 0;
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0) { fprintf(stderr,"Warning : Could not set socket Receive timeout \n"); }

    timeout.tv_sec = (unsigned int) 1/1000; timeout.tv_usec = 0;
    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0) { fprintf(stderr,"Warning : Could not set socket Send timeout \n"); }


    char buffer[1024]={0};
    sprintf(buffer,"GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n",filename,hostname);

    int opres =  send(sockfd,buffer,strlen(buffer),MSG_WAITALL|MSG_NOSIGNAL);  //Send filesize as soon as we've got it
    if (opres<=0) { fprintf(stderr,"Error Sending Request data\n"); } else
    {
      buffer[0]=0;
      opres = recv(sockfd,buffer,1024,MSG_WAITALL|MSG_NOSIGNAL);
    }
    // Todo add here some header sensing , malloc a good sized buffer , put the file in there ,
    close(sockfd);
  return 0;
}


