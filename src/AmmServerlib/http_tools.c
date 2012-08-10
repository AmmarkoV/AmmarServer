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

#include "http_tools.h"
#include "configuration.h"
#include <dirent.h>



enum FileType
{
    NO_FILETYPE=0,
    TEXT,
    IMAGE,
    AUDIO,
    VIDEO,
    EXECUTABLE,
    FOLDER
};


void error(char * msg)
{
 fprintf(stderr,"ERROR MESSAGE : %s\n",msg);
 return;
}

char FileExists(char * filename)
{
 FILE *fp = fopen(filename,"r");
 if( fp ) { /* exists */ fclose(fp); return 1; }
 return 0;
}

char DirectoryExists( char* dirpath )
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


int StripHTMLCharacters_Inplace(char * filename)
{
  //This piece of code converts characters like %20 to their ASCII equivalent " " ( for example )
  //It is not a full mapping of HTML characters to ASCII characters since ( for security reasons )
  //We dont want a full mapping.. :P , We just want to convert characters like spaces plus symbols etc
  //that are common in the internet to try and maintain some level of compatibility with such weird filenames
  //Of course I don't know why anyone would want to name his file "index of-website+.html" but.. :P

  unsigned int length=strlen(filename);
  unsigned int offset=0;
  if (length<=2) { return 0; }
  unsigned int i=0;
  while (i<length)
     {

        if ( (filename[i]=='%') && (i<length-2) )
         {
           unsigned char ascii_val=0,sec_byte=0;
           unsigned char sub_valNumb=0,sub_valHex=0;

           if ((filename[i+1]>='0')&&(filename[i+1]<='9'))  { sub_valNumb=filename[i+1]-'0'; ascii_val+=sub_valNumb*16; }  else
           if ((filename[i+1]>='A')&&(filename[i+1]<='F'))  { sub_valHex=filename[i+1]-'A';  ascii_val+=(10+sub_valHex)*16; }   else
                                                            { sec_byte=1; }


           if ((filename[i+2]>='0')&&(filename[i+2]<='9'))  { sub_valNumb=filename[i+2]-'0'; ascii_val+=sub_valNumb*1; }  else
           if ((filename[i+2]>='A')&&(filename[i+2]<='F'))  { sub_valHex=filename[i+2]-'A';  ascii_val+=(10+sub_valHex)*1; }   else
                                                            { sec_byte=1; }

           if ( ascii_val<' ')   { sec_byte=1; } else /* See IGNORED Control characters..! */
           if ( ascii_val>'~')   { sec_byte=1; }

           if (sec_byte)
            {
              fprintf(stderr,"BAD Hex URI Char %%%c%c attempted overflow ( %u ) \n",filename[i+1],filename[i+2],ascii_val);
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
   //This function checks for paths to ensure that fopen will be handed a safe file path..!
   unsigned int length=strlen(filename);
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



int FindIndexFile(char * webserver_root,char * directory,char * indexfile)
{
  strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"index.html");
  if ((FilenameStripperOk(indexfile))&&(FileExists(indexfile))) { return 1; }
  strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"index.htm"); // <- TODO : notice that i can just change the extension to reduce copying around
  if ((FilenameStripperOk(indexfile))&&(FileExists(indexfile))) { return 1; }
  strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"home.htm"); // <- TODO : notice that i can just change the extension to reduce copying around
  if ((FilenameStripperOk(indexfile))&&(FileExists(indexfile))) { return 1; }
  strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"home.html"); // <- TODO : notice that i can just change the extension to reduce copying around
  if ((FilenameStripperOk(indexfile))&&(FileExists(indexfile))) { return 1; }
  strcpy(indexfile,webserver_root); strcat(indexfile,directory); strcat(indexfile,"index.php"); // <- TODO : notice that i can just change the extension to reduce copying around
  if ((FilenameStripperOk(indexfile))&&(FileExists(indexfile))) { return 1; }

  indexfile[0]=0;
  return 0;
}

