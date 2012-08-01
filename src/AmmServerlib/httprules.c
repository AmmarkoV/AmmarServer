#include "httprules.h"
#include "string.h"
enum FileType
{
    NO_FILETYPE=0,
    TEXT,
    PHOTO,
    MUSIC,
    EXECUTABLE
};



int GetExtentionType(char * theextension)
{

if (strcmp(theextension,"GIF")==0)  { return PHOTO; } else
if (strcmp(theextension,"PNG")==0)  { return PHOTO; } else
if (strcmp(theextension,"JPG")==0)  { return PHOTO; } else
if (strcmp(theextension,"JPEG")==0) { return PHOTO; } else
if (strcmp(theextension,"BMP")==0)  { return PHOTO; } else
if (strcmp(theextension,"TIFF")==0) { return PHOTO; } else
if (strcmp(theextension,"JPEG")==0) { return PHOTO; } else
if (strcmp(theextension,"BMP")==0)  { return PHOTO; } else
if (strcmp(theextension,"DIB")==0)  { return PHOTO; } else
if (strcmp(theextension,"RLE")==0)  { return PHOTO; } else
if (strcmp(theextension,"J2C")==0)  { return PHOTO; } else
if (strcmp(theextension,"ICO")==0)  { return PHOTO; } else
if ((strcmp(theextension,"EXE")==0)||(strcmp(theextension,"DLL")==0)||(strcmp(theextension,"CPL")==0)||(strcmp(theextension,"SCR")==0)) { return EXECUTABLE; } else
if (strcmp(theextension,"MP3")==0) { return MUSIC; } else
if (strcmp(theextension,"MP4")==0) { return MUSIC; } else
if (strcmp(theextension,"WAV")==0) { return MUSIC; } else
if (strcmp(theextension,"MID")==0) { return MUSIC; } else
if (strcmp(theextension,"DIB")==0) { return MUSIC; } else
if (strcmp(theextension,"OGG")==0) { return MUSIC; } else
if (strcmp(theextension,"VOC")==0) { return MUSIC; } else
if (strcmp(theextension,"AU")==0)  { return MUSIC; } else
if (strcmp(theextension,"TXT")==0) { return TEXT; } else
if (strcmp(theextension,"DOC")==0) { return TEXT; } else
if (strcmp(theextension,"RTF")==0) { return TEXT; }

 return 0;
}


int FilenameStripperOk(char * filename)
{
   unsigned int length=strlen(filename);
   unsigned int i=length-1;

   while (i>0)
     {
        if ( filename[i]<' ') { return 0; } else
        if ( filename[i]>'~') { return 0; } else
        if ( ( filename[i-1]=='.')&&( filename[i]=='.') ) { return 0; }

        --i;
     }


   return 1;
}
