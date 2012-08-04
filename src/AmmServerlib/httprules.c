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

#include "httprules.h"
#include "configuration.h"


enum FileType
{
    NO_FILETYPE=0,
    TEXT,
    IMAGE,
    AUDIO,
    VIDEO,
    EXECUTABLE
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
 char content_type[MAX_CONTENT_TYPE]={0};
 GetContentTypeForExtension(theextension,content_type);

 //Crude and fast lookup
 if (content_type[0]=='t') { return TEXT; } else
 if (content_type[0]=='i') { return IMAGE; } else
 if (content_type[0]=='v') { return VIDEO; } else
 if ((content_type[0]=='a')&&(content_type[0]=='u')) { return AUDIO; } else
 if ((content_type[0]=='a')&&(content_type[0]=='p')) { return EXECUTABLE; }
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


int FilenameStripperOk(char * filename)
{
   //This function checks for paths to ensure that fopen will be handed a safe file path..!
   unsigned int length=strlen(filename);
   unsigned int i=length-1;

   while (i>0)
     {
        //We only accept english strings
        if ( filename[i]<' ') { return 0; } else
        if ( filename[i]>'~') { return 0; } else
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

