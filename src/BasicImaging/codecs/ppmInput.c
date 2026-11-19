#include "codecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "ppmInput.h"

#define PRINT_COMMENTS 1
#define PPMREADBUFLEN 256


unsigned int simplePowPPM(unsigned int base,unsigned int exp)
{
    if (exp==0) return 1;
    unsigned int retres=base;
    unsigned int i=0;
    for (i=0; i<exp-1; i++)
    {
        retres*=base;
    }
    return retres;
}



int swapDepthEndianness(struct Image * img)
{
  if (img==0) { return 0; }
  if (img->pixels==0) { return 0; }
  if (img->bitsperpixel!=16) { fprintf(stderr,"Only 16bit PNM files need swapping ( we have a %u bit x %u channels file )..\n",img->bitsperpixel , img->channels); return 0; }

  unsigned char * traverser=(unsigned char * ) img->pixels;
  unsigned char * traverserSwap1;//=(unsigned char * ) img->pixels;
  unsigned char * traverserSwap2;//=(unsigned char * ) img->pixels;

  unsigned int bytesperpixel = (img->bitsperpixel/8);
  unsigned char * endOfMem = traverser + img->width * img->height * img->channels * bytesperpixel;

  while ( ( traverser < endOfMem)  )
  {
    traverserSwap1 = traverser;
    traverserSwap2 = traverser+1;

    unsigned char tmp = *traverserSwap1;
    *traverserSwap1 = *traverserSwap2;
    *traverserSwap2 = tmp;

    traverser += bytesperpixel;
  }

 return 1;
}


unsigned char * ReadPNM(unsigned char * buffer ,const char * filename,unsigned int *width,unsigned int *height,unsigned long * timestamp , unsigned int * bytesPerPixel , unsigned int * channels)
{
   * bytesPerPixel = 0;
   * channels = 0;

    //See http://en.wikipedia.org/wiki/Portable_anymap#File_format_description for this simple and useful format
    unsigned char * pixels=buffer;
    FILE *pf=0;
    pf = fopen(filename,"rb");

    if (pf!=0 )
    {
        *width=0; *height=0; *timestamp=0;

        char buf[PPMREADBUFLEN]={0};
        char *t;
        unsigned int w=0, h=0, d=0;
        int r=0 , z=0;

        t = fgets(buf, PPMREADBUFLEN, pf);
        if (t == 0) { fclose(pf); return buffer; }

        if ( strncmp(buf,"P6\n", 3) == 0 ) { *channels=3; } else
        if ( strncmp(buf,"P5\n", 3) == 0 ) { *channels=1; } else
                                           { fprintf(stderr,"Could not understand/Not supported file format\n"); fclose(pf); return buffer; }
        do
        { /* Px formats can have # comments after first line */
           #if PRINT_COMMENTS
             memset(buf,0,PPMREADBUFLEN);
           #endif
           t = fgets(buf, PPMREADBUFLEN, pf);
           if (strstr(buf,"TIMESTAMP")!=0)
              {
                char * timestampPayloadStr = buf + 10;
                *timestamp = atoi(timestampPayloadStr);
              }

           if ( t == 0 ) { fclose(pf); return buffer; }
        } while ( strncmp(buf, "#", 1) == 0 );
        z = sscanf(buf, "%u %u", &w, &h);
        if ( z < 2 ) { fclose(pf); fprintf(stderr,"Incoherent dimensions received %ux%u \n",w,h); return buffer; }
        // Exactly one whitespace byte separates maxval from the pixels. A "%u\n" format swallowed every following
        // byte that looked like whitespace ( pixel values 9-13 , 32 ) , which the old "border case" fseek hack only
        // half undid
        r = fscanf(pf, "%u", &d);
        if (r < 1) { fprintf(stderr,"Could not understand how many bytesPerPixel there are on this image\n"); fclose(pf); return buffer; }
        fgetc(pf);
        if (d==255) { *bytesPerPixel=1; }  else
        if (d==65535) { *bytesPerPixel=2; } else
                       { fprintf(stderr,"Incoherent payload received %u bits per pixel \n",d); fclose(pf); return buffer; }

        //The size comes from the file , so compute it in size_t and refuse what struct Image ( unsigned int image_size )
        //can't hold. It used to be an unsigned int product : 65536x21846 RGB wrapped to 128KB while fread() still
        //asked for every pixel , a heap overflow from a small crafted file
        size_t pixelCount = (size_t) w * (size_t) h;
        size_t pixelBytes = (size_t) (*bytesPerPixel) * (*channels);
        if ( (w==0) || (h==0) || (pixelCount > (size_t) UINT_MAX / pixelBytes) )
           { fprintf(stderr,"Refusing PNM dimensions %ux%u\n",w,h); *bytesPerPixel=0; *channels=0; fclose(pf); return buffer; }

        *width=w; *height=h;
        int allocatedHere = (pixels==0);
        if (pixels==0) {  pixels= (unsigned char*) malloc(pixelCount*pixelBytes); }

        if ( pixels != 0 )
        {
          size_t rd = fread(pixels,pixelBytes, pixelCount, pf);
          if (rd < pixelCount )
             {
               //A truncated file used to come back as a whole image with the rest of the buffer uninitialised
               fprintf(stderr,"Incomplete read while reading file %s (%zu instead of %zu pixels)\n",filename,rd,pixelCount);
               fclose(pf);
               if (allocatedHere) { free(pixels); }
               *width=0; *height=0;
               return (allocatedHere) ? 0 : buffer;
             }

          fclose(pf);

           #if PRINT_COMMENTS
             if ( (*channels==1) && (*bytesPerPixel==2) && (timestamp!=0) ) { printf("DEPTH %lu\n",*timestamp); } else
             if ( (*channels==3) && (*bytesPerPixel==1) && (timestamp!=0) ) { printf("COLOR %lu\n",*timestamp); }
           #endif

          return pixels;
        } else
        {
            fprintf(stderr,"Could not Allocate enough memory for file %s \n",filename);
        }
        fclose(pf);
    } else
    {
      fprintf(stderr,"File %s does not exist \n",filename);
    }
  return buffer;
}


int ReadPPM(const char * filename,struct Image * pic,char read_only_header)
{
  pic->pixels = ReadPNM(pic->pixels , filename, &pic->width, &pic->height, &pic->timestamp ,&pic->bitsperpixel , &pic->channels );
  pic->bitsperpixel = pic->bitsperpixel * 8; // ( we go from bytes to bits )

  return (pic->pixels!=0);
}



int ReadSwappedPPM(const char * filename,struct Image * pic,char read_only_header)
{
  ReadPPM(filename,pic,read_only_header);
  swapDepthEndianness(pic);
  return (pic->pixels!=0);
}


int WritePPMOld(const char * filename,struct Image * pic)
{
    //fprintf(stderr,"saveRawImageToFile(%s) called\n",filename);
    if (pic==0) { return 0; }
    if ( (pic->width==0) || (pic->height==0) || (pic->channels==0) || (pic->bitsperpixel==0) )
        {
          fprintf(stderr,"saveRawImageToFile(%s) called with zero dimensions ( %ux%u %u channels %u bpp\n",filename,pic->width , pic->height,pic->channels,pic->bitsperpixel);
          return 0;
        }
    if(pic->pixels==0) { fprintf(stderr,"saveRawImageToFile(%s) called for an unallocated (empty) frame , will not write any file output\n",filename); return 0; }
    if (pic->bitsperpixel/pic->channels>16) { fprintf(stderr,"PNM does not support more than 2 bytes per pixel..!\n"); return 0; }

    FILE *fd=0;
    fd = fopen(filename,"wb");

    if (fd!=0)
    {
        unsigned int n;
        if (pic->channels==3) fprintf(fd, "P6\n");
        else if (pic->channels==1) fprintf(fd, "P5\n");
        else
        {
            fprintf(stderr,"Invalid channels arg (%u) for SaveRawImageToFile\n",pic->channels);
            fclose(fd);
            return 1;
        }

        fprintf(fd, "%u %u\n%u\n", pic->width, pic->height , simplePowPPM(2 ,pic->bitsperpixel)-1);

        float tmp_n = (float) pic->bitsperpixel/ 8;
        tmp_n = tmp_n *  pic->width * pic->height * pic->channels ;
        n = (unsigned int) tmp_n;

        fwrite(pic->pixels, 1 , n , fd);
        fflush(fd);
        fclose(fd);
        return 1;
    }
    else
    {
        fprintf(stderr,"SaveRawImageToFile could not open output file %s\n",filename);
        return 0;
    }
    return 0;
}


int WritePPM16bitMultiChannel(const char * filename, struct Image * pic)
{
    if (pic == 0) { return 0; }
    if ((pic->width == 0) || (pic->height == 0) || (pic->channels == 0) || (pic->bitsperpixel == 0))
    {
        fprintf(stderr, "saveRawImageToFile(%s) called with zero dimensions ( %ux%u %u channels %u bpp\n", filename, pic->width, pic->height, pic->channels, pic->bitsperpixel);
        return 0;
    }
    if (pic->pixels == 0)
    {
        fprintf(stderr, "saveRawImageToFile(%s) called for an unallocated (empty) frame, will not write any file output\n", filename);
        return 0;
    }
    if (pic->bitsperpixel / pic->channels > 16)
    {
        fprintf(stderr, "PNM does not support more than 2 bytes per pixel..!\n");
        return 0;
    }

    FILE *fd = fopen(filename, "wb");
    if (fd != 0)
        {
         if (pic->channels == 3) { fprintf(fd, "P6\n"); } else
         if (pic->channels == 1) { fprintf(fd, "P5\n"); } else
         {
            fprintf(stderr, "Invalid channels arg (%u) for SaveRawImageToFile\n", pic->channels);
            fclose(fd);
            return 1;
         }

         unsigned int bitsperchannelpixel = pic->bitsperpixel / pic->channels;
         fprintf(fd, "%u %u\n%u\n", pic->width, pic->height, simplePowPPM(2, bitsperchannelpixel) - 1);

         unsigned int n = pic->width * pic->height * pic->channels * (bitsperchannelpixel / 8);

         fwrite(pic->pixels, 1, n, fd);
         fflush(fd);
         fclose(fd);
         return 1;
    } else
    {
        fprintf(stderr, "SaveRawImageToFile could not open output file %s\n", filename);
        return 0;
    }
    return 0;
}



int WritePPM(const char * filename,struct Image * pic)
{
   return WritePPM16bitMultiChannel(filename,pic);
       /*
    if ( (pic->channels==3) && (pic->bitsperpixel==48) )
    {
       fprintf(stderr,"WritePPM16bitMultiChannel %s \n",filename);
       return WritePPM16bitMultiChannel(filename,pic);
    } else
    {
       return WritePPMOld(filename,pic);
    }
  return 0;*/
}

int WriteSwappedPPM(const char * filename,struct Image * pic)
{
   swapDepthEndianness(pic);
   return WritePPM(filename,pic);
}




