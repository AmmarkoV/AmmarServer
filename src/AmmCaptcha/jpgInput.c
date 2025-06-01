#include "jpgInput.h"

#define USE_JPG_FILES 1

#if USE_JPG_FILES


#include <stdio.h>
#include <string.h>
#include <jpeglib.h>
#include <stdlib.h>
//#include "codecs.h"


/* we will be using this uninitialized pointer later to store raw, uncompressd image */
//unsigned char *raw_image = NULL;

/* dimensions of the image we want to write */
//int JPEGwidth = 1600;
//int JPEGheight = 1200;
//int JPEGbytes_per_pixel = 3;   /* or 1 for GRACYSCALE images */

/**
 * read_jpeg_file Reads from a jpeg file on disk specified by filename and saves into the
 * raw_image buffer in an uncompressed format.
 *
 * \returns positive integer if successful, -1 otherwise
 * \param *filename char string specifying the file name to read from
 *
 */

/* setup the buffer but we did that in the main function */
void init_buffer(struct jpeg_compress_struct* cinfo) { return ; }

/* what to do when the buffer is full; this should almost never
 * happen since we allocated our buffer to be big to start with
 */
int empty_buffer(struct jpeg_compress_struct* cinfo) { return 1; }

/* finalize the buffer and do any cleanup stuff */
void term_buffer(struct jpeg_compress_struct* cinfo) { return ; }



int fastJPGHeaderCheck(FILE * file)
{
  unsigned char a = fgetc (file);
  unsigned char b = fgetc (file);
  rewind (file);
  if ( (a==0xFF) && (b==0xD8) ) { return 1; }
  //if ( (a==0x89) && (b==0x50) ) { return 1; }
  return 0;
}



static void jpeg_mem_init_source(j_decompress_ptr cinfo) {
    // No initialization needed
}

static boolean jpeg_mem_fill_input_buffer(j_decompress_ptr cinfo) {
    // Insert a fake EOI (End of Image) marker
    static const JOCTET fake_eoi[] = { 0xFF, JPEG_EOI };
    cinfo->src->next_input_byte = fake_eoi;
    cinfo->src->bytes_in_buffer = sizeof(fake_eoi);
    return TRUE;
}

static void jpeg_mem_skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
    if (num_bytes > 0) {
        if (num_bytes > (long)cinfo->src->bytes_in_buffer) {
            jpeg_mem_fill_input_buffer(cinfo);
        } else {
            cinfo->src->next_input_byte += num_bytes;
            cinfo->src->bytes_in_buffer -= num_bytes;
        }
    }
}

static void jpeg_mem_term_source(j_decompress_ptr cinfo) {
    // Nothing to do here
}

void jpeg_memory_src(j_decompress_ptr cinfo, const unsigned char *buffer, size_t bufsize)
{
    if (cinfo->src == NULL) {
        cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)(
            (j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_source_mgr));
    }

    struct jpeg_source_mgr *src = cinfo->src;
    src->init_source = jpeg_mem_init_source;
    src->fill_input_buffer = jpeg_mem_fill_input_buffer;
    src->skip_input_data = jpeg_mem_skip_input_data;
    src->resync_to_restart = jpeg_resync_to_restart; // Use default
    src->term_source = jpeg_mem_term_source;
    src->bytes_in_buffer = bufsize;
    src->next_input_byte = buffer;
}

// Memory-based JPEG reader
int ReadJPEGMem(unsigned char *buffer, unsigned int bufferSize, struct Image *pic, char read_only_header)
{
    if (buffer == NULL || bufferSize == 0)
    {
        fprintf(stderr, "Cannot load from null or empty buffer\n");
        return 0;
    }
    if (pic == NULL)
    {
        fprintf(stderr, "Invalid picture structure\n");
        return 0;
    }

    struct jpeg_decompress_struct cinfo = {0};
    struct jpeg_error_mgr jerr = {0};
    JSAMPROW row_pointer[1] = {0};
    unsigned long location = 0;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_memory_src(&cinfo, buffer, bufferSize);
    jpeg_read_header(&cinfo, TRUE);

    pic->width = cinfo.image_width;
    pic->height = cinfo.image_height;

    if (read_only_header || pic->width == 0 || pic->height == 0)
    {
        jpeg_destroy_decompress(&cinfo);
        return 1;
    }

    jpeg_start_decompress(&cinfo);

    unsigned long img_size = cinfo.output_width * cinfo.output_height * cinfo.num_components;
    pic->imageSize = img_size;
    unsigned char *raw_image = (unsigned char *)malloc(img_size);

    if (raw_image != NULL) {
        memset(raw_image, 0, img_size);

        float get_channels = (float)img_size / (pic->width * pic->height);
        pic->depth = cinfo.out_color_components;
        //pic->bitsperpixel = pic->depth * 8;

        if (get_channels < (float) pic->depth)
        {
            fprintf(stderr, "ReadJPEGMem: Image buffer size mismatch for channels (got %0.2f expected %u) \n",get_channels,pic->depth);
        }

        row_pointer[0] = (unsigned char *)malloc(cinfo.output_width * cinfo.num_components);
        if (row_pointer[0] != NULL)
        {
            while (cinfo.output_scanline < cinfo.image_height)
            {
                jpeg_read_scanlines(&cinfo, row_pointer, 1);
                memcpy(raw_image + location, row_pointer[0], cinfo.image_width * cinfo.num_components);
                location += cinfo.image_width * cinfo.num_components;
            }
            free(row_pointer[0]);
        }

#if READ_CREATES_A_NEW_PIXEL_BUFFER
        if (pic->pixels != 0) {
            fprintf(stderr, "Erasing previously allocated buffer\n");
            free(pic->pixels);
        }
        pic->pixels = raw_image;
#else
        memcpy(pic->pixels, raw_image, img_size);
        free(raw_image);
        raw_image = NULL;
#endif
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return 1;
}


int ReadJPEG(const char *filename,struct Image * pic,char read_only_header)
{
    if (filename==0) { fprintf(stderr,"Cannot load Null filename\n"); return 0; }
    if (pic==0) { fprintf(stderr,"Damaged picture structure , cannot load %s\n",filename); return 0; }

    //fprintf(stderr,"%s\n",filename);
	/* these are standard libjpeg structures for reading(decompression) */
	struct jpeg_decompress_struct cinfo={0};
	struct jpeg_error_mgr jerr={0};
	/* libjpeg data structure for storing one row, that is, scanline of an image */
	JSAMPROW row_pointer[1] = {0};


	FILE *infile = fopen( filename, "rb" );
	if ( infile==0 )
	{
		fprintf(stderr,"Error opening jpeg file %s\n!", filename );
		return 0;
	}

	if (!fastJPGHeaderCheck(infile))
    {
		fclose(infile);
		return 0;
	}

	unsigned long location = 0;

	/* here we set up the standard libjpeg error handler */
	cinfo.err = jpeg_std_error( &jerr );
	/* setup decompression process and source, then read JPEG header */
	jpeg_create_decompress( &cinfo );
	/* this makes the library read from infile */
	jpeg_stdio_src( &cinfo, infile );
	/* reading the image header which contains image information */
	jpeg_read_header( &cinfo, TRUE );
	/* Uncomment the following to output image information, if needed. */

	pic->width =cinfo.image_width;
	pic->height=cinfo.image_height;

	if ( (read_only_header) || ( (pic->width==0) || (pic->height==0)  ) )
	  {
	    //we dont want to load the body , just return here
        jpeg_destroy_decompress( &cinfo );
        fclose( infile );
	    return 1;
	  }

	/*--
	printf( "JPEG File Information: \n" );
	printf( "Image width and height: %d pixels and %d pixels.\n", cinfo.image_width, cinfo.image_height );
	printf( "Color components per pixel: %d.\n", cinfo.num_components );
	printf( "Color space: %d.\n", cinfo.jpeg_color_space );
	--*/
	/* Start decompression jpeg here */
	jpeg_start_decompress( &cinfo );

	/* allocate memory to hold the uncompressed image */
	unsigned long img_size = cinfo.output_width*cinfo.output_height*cinfo.num_components;
	pic->imageSize = img_size;
    unsigned char * raw_image = (unsigned char*) malloc( img_size );

    if (raw_image!=0)
    {
     memset(raw_image,0,img_size);

     float get_channels = (float) cinfo.output_width*cinfo.output_height*cinfo.num_components / cinfo.image_width * cinfo.image_height;
	 pic->depth=cinfo.out_color_components;
	 //pic->bitsperpixel=pic->depth*8;

     if (get_channels < pic->depth)
          { fprintf(stderr,"Picture %s has incorrect filesize allocated for its channels ( %u vs %0.2f) ..!\n",filename , pic->depth , get_channels); }

	 /* now actually read the jpeg into the raw buffer */
	 row_pointer[0] = (unsigned char *)malloc( cinfo.output_width*cinfo.num_components );
	 if (row_pointer[0]!=0)
     {
	  /* read one scan line at a time */
	  while( cinfo.output_scanline < cinfo.image_height )
	   {
		jpeg_read_scanlines( &cinfo, row_pointer, 1 );
		for(int i=0; i<cinfo.image_width*cinfo.num_components; i++)
          {
			raw_image[location++] = row_pointer[0][i];
          }
	   }
      free(row_pointer[0]);
     }
	/* wrap up decompression, destroy objects, free pointers and close open files */

    #if READ_CREATES_A_NEW_PIXEL_BUFFER
      if ( pic->pixels!=0)
      {
         fprintf(stderr,"Erasing previously allocated buffer\n");
         free(pic->pixels);
         pic->pixels=0;
      }
	  pic->pixels = raw_image;
    #else
     fprintf(stderr,"please #define READ_CREATES_A_NEW_PIXEL_BUFFER=1\n");
     exit(2);
	 memcpy(pic->pixels,(unsigned char *) raw_image ,  cinfo.output_width*cinfo.output_height*cinfo.num_components);
	 free(raw_image);
	 raw_image=0;
	#endif
    }

	jpeg_finish_decompress( &cinfo );
	jpeg_destroy_decompress( &cinfo );
	fclose(infile);
	/* yup, we succeeded! */
	return 1;
}

/**
 * write_jpeg_file Writes the raw image data stored in the raw_image buffer
 * to a jpeg image with default compression and smoothing options in the file
 * specified by *filename.
 *
 * \returns positive integer if successful, -1 otherwise
 * \param *filename char string specifying the file name to save to
 *
 */
int WriteJPEGInternal(const char *filename,struct Image * pic,char *mem,unsigned long * mem_size,int quality)
{
    //debug where things get loaded using next line..
    //fprintf(stderr,"WriteJPEG(%s,%p,%p,%p); called \n",filename,pic,mem,mem_size);
    if (filename==0)    { return 0; }
    if (pic==0)         { fprintf(stderr,"WriteJPEG called with an incorrect image structure \n "); return 0; }
	if (pic->pixels==0) { fprintf(stderr,"WriteJPEG called with a problematic raw image..\n ");     return 0; }
	if ( (pic->depth!=1) && (pic->depth!=3) )
    {
      fprintf(stderr,"WriteJPEG called with a problematic raw image (START)..\n ");
      fprintf(stderr,"However we were given a JPEG file with %u channels.. \n",pic->depth);
      return 0;
    }

	unsigned char * raw_image = (unsigned char * ) pic->pixels;
	struct jpeg_compress_struct cinfo ={0}; // memset(&cinfo,0,sizeof(struct jpeg_compress_struct));;
	struct jpeg_error_mgr jerr={0};         // memset(&jerr,0,sizeof(struct jpeg_error_mgr));;
    struct jpeg_destination_mgr dmgr={0};   // memset(&dmgr,0,sizeof(struct jpeg_destination_mgr));;
    unsigned long initial_mem_size = 0;     //*mem_size; can crash with a zero mem_size because it tries for the value of a zero pointer..

    FILE *outfile =0;

	/* this is a pointer to one row of image data */
	JSAMPROW row_pointer[1]={0};

	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_compress(&cinfo);

	/* Setting the parameters of the output file here */
    if ( (mem!=0) && (mem_size!=0) )
	 {
	   //We want destination to be our buffer..!
       dmgr.init_destination    = init_buffer;
	   dmgr.empty_output_buffer = empty_buffer;
	   dmgr.term_destination    = term_buffer;
	   dmgr.next_output_byte    = (JOCTET*) mem;
	   dmgr.free_in_buffer      = *mem_size;
       initial_mem_size         = *mem_size;

	   cinfo.dest = &dmgr;
	 } else
	 {
	    outfile = fopen( filename, "wb" );
        if ( !outfile )
	     {
	    	printf("Error opening output jpeg file %s\n!", filename );
		    return 0;
	     }

	   jpeg_stdio_dest(&cinfo, outfile);
	 }

    int JPEGcolor_space = 0;
	cinfo.image_width  = pic->width;
	cinfo.image_height = pic->height;

	if (pic->depth==3)
    {
	 cinfo.input_components = 3;
     JPEGcolor_space = JCS_RGB;
    } else
    if (pic->depth==1)
    {
     cinfo.input_components   = 1;
     JPEGcolor_space  = JCS_GRAYSCALE;
    } else
    {
     fprintf(stderr,"Asked to encode %s as JPEG\n",filename);
     fprintf(stderr,"However we were given a JPEG file with %u channels.. \n",pic->depth);
     fprintf(stderr,"Can only handle RGB (3) / or Monochrome (1) channel images.. \n");
     fclose(outfile);
     return 0;
    }


	cinfo.in_color_space = (J_COLOR_SPACE) JPEGcolor_space;
    /* default compression parameters, we shouldn't be worried about these */
	jpeg_set_defaults( &cinfo );
	jpeg_set_quality (&cinfo, quality ,1/*TRUE*/);
	/* Now do the compression .. */
	jpeg_start_compress( &cinfo, 1/*TRUE*/ );
	/* like reading a file, this time write one row at a time */

     while( cinfo.next_scanline < cinfo.image_height )
	   {
		row_pointer[0] = &raw_image[ cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
		jpeg_write_scanlines( &cinfo, row_pointer, 1 );
       }

    jpeg_finish_compress( &cinfo );
    jpeg_destroy_compress( &cinfo );

	 if ( (mem!=0) && (mem_size!=0) )
	 {
	   //Write back the file size of the compressed image
       *mem_size = initial_mem_size-cinfo.dest->free_in_buffer;
	 } else
	 {
	   /* similar to read file, clean up after we're done compressing */
	   fclose( outfile );
	 }
	/* success code is 1! */
	return 1;
}

int WriteJPEGFile(struct Image * pic,const char *filename)
{
    int quality = 75; // 1 - 100 range
    return WriteJPEGInternal(filename,pic,0,0,quality);
}


int WriteJPEGMemory(struct Image * pic,char *mem,unsigned long * mem_size) //,int quality
{
    int quality = 90;
    return WriteJPEGInternal(0,pic,mem,mem_size,quality);
}


int jpegtest()
{
	char *infilename = (char*) "test.jpg", *outfilename = (char*) "test_out.jpg";

	/* Try opening a jpeg*/
	if( ReadJPEG( infilename , 0  , 0 ) > 0 )
	{
		/* then copy it to another file */
		if( WriteJPEGFile( 0 , outfilename ) < 0 ) return -1;
	}
	else return -1;
	return 0;
}

#endif

