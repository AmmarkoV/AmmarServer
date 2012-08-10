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

  fprintf(stderr,"ReducePathSlashes_Inplace needs thorough testing %s.. :P\n",filename);
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
       fprintf(stderr,"String shortened to %s.. :P\n",filename);
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
  fprintf(stderr,"StripHTMLCharacters_Inplace(%s) not implemented yet..\n",filename);
  return 0;
  unsigned int length=strlen(filename);
  unsigned int offset=0;
  if (length<=2) { return 0; }
  unsigned int i=0;
  while (i<length)
     {

        if ( (filename[i]=='%') && (i<length-2) )
         {
           unsigned char ascii_val=0,sec_byte=0;
           unsigned int sub_valNumb=filename[i+1]-'0',sub_valHex=filename[i+1]-'A';
           if ((sub_valNumb>9) || (sub_valHex>5)) { sec_byte=1; /*Security check for overflow hex values*/ } else
           if (sub_valNumb<=9) { ascii_val+=sub_valNumb*16; } else
           if (sub_valHex<=5)  { ascii_val+=(10+sub_valHex)*16; }

           sub_valNumb=filename[i+2]-'0'; sub_valHex=filename[i+2]-'A';
           if ((sub_valNumb>9) || (sub_valHex>5)) { sec_byte=1; /*Security check for overflow hex values*/ } else
           if (sub_valNumb<=9) { ascii_val+=sub_valNumb; } else
           if (sub_valHex<=5)  { ascii_val+=10+sub_valHex; }

           if (ascii_val<32) { sec_byte=1; /* See IGNORED Control characters..! */ }
           //TODO : remove some more useless characters like † 	%86 etc.. :P

           if (sec_byte)
            {
              fprintf(stderr,"Hex URI Char %%%c%c attempted overflow\n",filename[i+1],filename[i+2]);
            } else
            {
              fprintf(stderr,"Hex URI Char %%%c%c = ascii %u = `%c`\n",filename[i+1],filename[i+2],ascii_val,ascii_val);
              offset+=2;
              filename[i]=ascii_val;
            }
         }
        ++i;
        if (offset!=0) {  filename[i]=filename[i+offset]; }
     }
  if (offset!=0) {  filename[i]=0; /*Append null termination..!*/ }


  /*
space 	%20
! 	%21
" 	%22
# 	%23
$ 	%24
% 	%25
& 	%26
' 	%27
( 	%28
) 	%29
* 	%2A
+ 	%2B
, 	%2C
- 	%2D
. 	%2E
/ 	%2F
0 	%30
1 	%31
2 	%32
3 	%33
4 	%34
5 	%35
6 	%36
7 	%37
8 	%38
9 	%39
: 	%3A
; 	%3B
< 	%3C
= 	%3D
> 	%3E
? 	%3F
@ 	%40
A 	%41
B 	%42
C 	%43
D 	%44
E 	%45
F 	%46
G 	%47
H 	%48
I 	%49
J 	%4A
K 	%4B
L 	%4C
M 	%4D
N 	%4E
O 	%4F
P 	%50
Q 	%51
R 	%52
S 	%53
T 	%54
U 	%55
V 	%56
W 	%57
X 	%58
Y 	%59
Z 	%5A
[ 	%5B
\ 	%5C
] 	%5D
^ 	%5E
_ 	%5F
` 	%60
a 	%61
b 	%62
c 	%63
d 	%64
e 	%65
f 	%66
g 	%67
h 	%68
i 	%69
j 	%6A
k 	%6B
l 	%6C
m 	%6D
n 	%6E
o 	%6F
p 	%70
q 	%71
r 	%72
s 	%73
t 	%74
u 	%75
v 	%76
w 	%77
x 	%78
y 	%79
z 	%7A
{ 	%7B
| 	%7C
} 	%7D
~ 	%7E
  	%7F
` 	%80
 	%81
‚ 	%82
ƒ 	%83
„ 	%84
… 	%85
† 	%86
‡ 	%87
ˆ 	%88
‰ 	%89
Š 	%8A
‹ 	%8B
Œ 	%8C
 	%8D
Ž 	%8E
 	%8F
 	%90
‘ 	%91
’ 	%92
“ 	%93
” 	%94
• 	%95
– 	%96
— 	%97
˜ 	%98
™ 	%99
š 	%9A
› 	%9B
œ 	%9C
 	%9D
ž 	%9E
Ÿ 	%9F
  	%A0
¡ 	%A1
¢ 	%A2
£ 	%A3
¤ 	%A4
¥ 	%A5
¦ 	%A6
§ 	%A7
¨ 	%A8
© 	%A9
ª 	%AA
« 	%AB
¬ 	%AC
­ 	%AD
® 	%AE
¯ 	%AF
° 	%B0
± 	%B1
² 	%B2
³ 	%B3
´ 	%B4
µ 	%B5
¶ 	%B6
· 	%B7
¸ 	%B8
¹ 	%B9
º 	%BA
» 	%BB
¼ 	%BC
½ 	%BD
¾ 	%BE
¿ 	%BF
À 	%C0
Á 	%C1
Â 	%C2
Ã 	%C3
Ä 	%C4
Å 	%C5
Æ 	%C6
Ç 	%C7
È 	%C8
É 	%C9
Ê 	%CA
Ë 	%CB
Ì 	%CC
Í 	%CD
Î 	%CE
Ï 	%CF
Ð 	%D0
Ñ 	%D1
Ò 	%D2
Ó 	%D3
Ô 	%D4
Õ 	%D5
Ö 	%D6
× 	%D7
Ø 	%D8
Ù 	%D9
Ú 	%DA
Û 	%DB
Ü 	%DC
Ý 	%DD
Þ 	%DE
ß 	%DF
à 	%E0
á 	%E1
â 	%E2
ã 	%E3
ä 	%E4
å 	%E5
æ 	%E6
ç 	%E7
è 	%E8
é 	%E9
ê 	%EA
ë 	%EB
ì 	%EC
í 	%ED
î 	%EE
ï 	%EF
ð 	%F0
ñ 	%F1
ò 	%F2
ó 	%F3
ô 	%F4
õ 	%F5
ö 	%F6
÷ 	%F7
ø 	%F8
ù 	%F9
ú 	%FA
û 	%FB
ü 	%FC
ý 	%FD
þ 	%FE
ÿ 	%FF
  */
/* IGNORED Control characters..!
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
  return 0;
}

int FilenameStripperOk(char * filename)
{
   //This function checks for paths to ensure that fopen will be handed a safe file path..!
   unsigned int length=strlen(filename);
   unsigned int i=length-1;

   unsigned int back_slashes_number=0;
   unsigned int slashes_number=0;

   while (i>0)
     {
        //We have imposed a limit on slash number..!
        if ( filename[i]=='\\') { ++slashes_number; } else
        if ( filename[i]=='/')  { ++back_slashes_number; }
        if ( (back_slashes_number>MAX_RESOURCE_SLASHES) || (slashes_number>MAX_RESOURCE_SLASHES)  ) { return 0; }

        //We only accept english strings
        if ( filename[i]<' ')   { return 0; } else
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

