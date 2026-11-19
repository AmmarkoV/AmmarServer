#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codecs.h"
//Note : USE_PZP_FILES is defined ( with an #ifndef guard ) in codecs.h , included above ; it must NOT
//be redefined here unconditionally ( the upstream file did , before this line even had a chance to see
//it ) , or it would silently force PZP support back on even on a build where BasicImaging's
//CMakeLists.txt detected libzstd/liblz4 are missing and passed USE_PZP_FILES=0 in.

#if USE_PZP_FILES

#include "pzpInput.h"
#include "pzp.h"

/* pzp.h checks the INTERNAL layout ( channelsInternal x bitsperpixelInternal ) against the decoded bytes , but the
 * image is sized from the EXTERNAL fields , which also come from the file. A file claiming e.g. 4 external
 * channels over 1 internal one used to be handed on as 4 times bigger than its buffer. Accept only layouts with
 * the same bytes per pixel , in the 8/16 bit , 1-4 channel range this library handles. */
static int pzpLayoutMatches(unsigned int bitsperpixelExternal,unsigned int channelsExternal,
                            unsigned int bitsperpixelInternal,unsigned int channelsInternal)
{
  if ( (bitsperpixelExternal!=8) && (bitsperpixelExternal!=16) ) { return 0; }
  if ( (channelsExternal==0) || (channelsExternal>4) )           { return 0; }
  if ( (bitsperpixelInternal==0) || (bitsperpixelInternal%8!=0) ) { return 0; }
  return ( (bitsperpixelExternal/8)*channelsExternal == (bitsperpixelInternal/8)*channelsInternal );
}

int ReadPZP(const char *filename, struct Image *pic)
{
    //fprintf(stderr, "Decompressing %s\n", filename);
    unsigned int width = 0, height = 0;
    unsigned int bitsperpixelExternal = 0, channelsExternal = 3;
    unsigned int bitsperpixelInternal = 24, channelsInternal = 3;
    unsigned int configuration = 0;

    pic->pixels = pzp_decompress_combined(filename, &width, &height, &bitsperpixelExternal, &channelsExternal, &bitsperpixelInternal, &channelsInternal, &configuration);
    if (pic->pixels == 0)
    {
        //Upstream doesn't check this itself : a corrupt/truncated file makes pzp_decompress_combined()
        //return 0 , and without this check we'd report success with a NULL pixel buffer.
        return 0;
    }
    if (!pzpLayoutMatches(bitsperpixelExternal,channelsExternal,bitsperpixelInternal,channelsInternal))
    {
        fprintf(stderr,"PZP %s : external layout %u ch @ %u bit does not match the decoded %u ch @ %u bit\n",filename,channelsExternal,bitsperpixelExternal,channelsInternal,bitsperpixelInternal);
        free(pic->pixels); pic->pixels=0;
        return 0;
    }

    pic->width = width;
    pic->height = height;
    pic->channels = channelsExternal;
    pic->bitsperpixel = bitsperpixelExternal * channelsExternal;
    pic->image_size = width * height * (bitsperpixelExternal / 8) * channelsExternal;

    return 1;
}


int ReadPZPMemory(const char *mem, unsigned int memSize, struct Image *pic)
{
    //fprintf(stderr, "Decompressing %s\n", filename);
    unsigned int width = 0, height = 0;
    unsigned int bitsperpixelExternal = 0, channelsExternal = 3;
    unsigned int bitsperpixelInternal = 24, channelsInternal = 3;
    unsigned int configuration = 0;

    pic->pixels = pzp_decompress_combined_from_memory((const void *) mem, (size_t) memSize, &width, &height, &bitsperpixelExternal, &channelsExternal, &bitsperpixelInternal, &channelsInternal, &configuration);
    if (pic->pixels == 0)
    {
        return 0; //see the same note in ReadPZP() above
    }
    if (!pzpLayoutMatches(bitsperpixelExternal,channelsExternal,bitsperpixelInternal,channelsInternal))
    {
        free(pic->pixels); pic->pixels=0; //see the same check in ReadPZP() above
        return 0;
    }

    pic->width = width;
    pic->height = height;
    pic->channels = channelsExternal;
    pic->bitsperpixel = bitsperpixelExternal * channelsExternal;
    pic->image_size = width * height * (bitsperpixelExternal / 8) * channelsExternal;

    return 1;
}




int WritePZP(const char *filename, struct Image *pic)
{
    //fprintf(stderr, "Compressing %s\n", filename);
    unsigned int bitsperpixelInternal = pic->bitsperpixel;
    unsigned int channelsInternal = pic->channels;
    unsigned int configuration = USE_COMPRESSION | USE_RLE;

    if (pic->bitsperpixel == 16)
    {
        bitsperpixelInternal = 8;
        channelsInternal *= 2;
    }

    unsigned char **buffers = malloc(channelsInternal * sizeof(unsigned char *));
    if (buffers == NULL)
    {
        return 0;
    }

    for (unsigned int ch = 0; ch < channelsInternal; ch++)
    {
        buffers[ch] = malloc((size_t) pic->width * pic->height * sizeof(unsigned char));
        if (buffers[ch] == NULL) //Was unchecked , pzp_split_channels() then wrote through NULL
        {
            for (unsigned int done = 0; done < ch; done++) { free(buffers[done]); }
            free(buffers);
            return 0;
        }
    }

    pzp_split_channels(pic->pixels, buffers, channelsInternal, pic->width, pic->height);

    //No pzp_RLE_filter() here any more : since the channel group format , pzp_compress_combined() applies the
    //USE_RLE ( left ) predictor itself through its default group , filtering here as well would apply it twice.
    //pzp_compress_combined() returns 1 on success , 0 on failure ( bad output path, allocation
    //failure, a frame that failed to compress ) - it never exits/longjmps , see the comment on
    //fail() in pzp.h. groups=NULL/0 asks for the default single group , like pzp_lib.c does for plain images.
    int ok = pzp_compress_combined(buffers, pic->width, pic->height, pic->bitsperpixel, pic->channels, bitsperpixelInternal, channelsInternal, configuration, NULL, 0, filename);

    for (unsigned int ch = 0; ch < channelsInternal; ch++)
    {
        free(buffers[ch]);
    }
    free(buffers);
    return ok;
}

#endif // USE_PZP_FILES
