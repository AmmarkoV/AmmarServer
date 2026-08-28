/** @file basicImaging.h
* @brief BasicImaging - the one place every AmmarServer Service should go through to decode, resize
*        and re-encode images ( thumbnailing , photo uploads , etc ). Backed by a trimmed-down vendored
*        copy of Ammar Qammaz's codecs library ( see codecs/ ) for JPEG/PNG/PZP , plus its own resize.
*
*        Safety contract : nothing in this header ever crashes on bad/missing codecs. If libjpeg ,
*        libpng , or libzstd+liblz4 ( PZP needs both ) weren't found when this library was built ,
*        BasicImaging_HasJPEG()/HasPNG()/HasPZP() report that , and the corresponding Save calls just
*        return 0 ( logging why ) instead of doing anything unsafe - this includes an internal pzp
*        encode error, which upstream would otherwise exit() the whole process. BasicImaging_ThumbnailFile()
*        goes one step further : on ANY failure ( bad input file , missing codec , out of memory ) it
*        falls back to copying the original file verbatim to the destination , so callers never end up
*        with a missing or half-written file.
*
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef BASICIMAGING_H_INCLUDED
#define BASICIMAGING_H_INCLUDED

#include "codecs/image.h" //struct Image

#ifdef __cplusplus
extern "C"
{
#endif

/**
* @brief Was this build compiled with JPEG support ( i.e. was libjpeg/jpeglib.h found at build time )
* @retval 1=Yes,0=No - if 0 , BasicImaging_SaveJPEG() / the JPEG branch of BasicImaging_Save() will always fail safely and return 0
*/
int BasicImaging_HasJPEG();

/**
* @brief Was this build compiled with PNG support ( i.e. was libpng/png.h found at build time )
* @retval 1=Yes,0=No - if 0 , BasicImaging_SavePNG() / the PNG branch of BasicImaging_Save() will always fail safely and return 0
*/
int BasicImaging_HasPNG();

/**
* @brief Was this build compiled with PZP support ( i.e. were both libzstd and liblz4 found at build time )
* @retval 1=Yes,0=No - if 0 , BasicImaging_SavePZP() / the .pzp branch of BasicImaging_Save() will always fail safely and return 0
*/
int BasicImaging_HasPZP();


/**
* @brief Decode a .jpg/.jpeg/.png file ( or anything else the underlying codecs library recognises by
*        extension/header sniffing ) into a freshly allocated struct Image
* @param Path to the image file to load
* @retval A newly allocated struct Image the caller owns and must release with BasicImaging_Free() , or 0 on failure ( missing file , unsupported/undetectable format , decode error , disabled codec )
*/
struct Image * BasicImaging_Load(const char * filename);


/**
* @brief Allocate a blank ( zeroed ) image , 8 bits per channel
* @param Width in pixels , Height in pixels , Number of channels ( 1=grayscale , 3=RGB , 4=RGBA )
* @retval A newly allocated struct Image the caller owns and must release with BasicImaging_Free() , or 0 on failure ( bad arguments / out of memory )
*/
struct Image * BasicImaging_New(unsigned int width,unsigned int height,unsigned int channels);

/**
* @brief Release an image allocated by any BasicImaging_* call , and NULL the caller's pointer so it can't be used ( or freed ) again by mistake
* @param Address of the struct Image* to release - *img is set to 0 afterwards
*/
void BasicImaging_Free(struct Image ** img);


/**
* @brief Save an image as JPEG. Safe no-op ( returns 0 ) if this build has no JPEG support - see BasicImaging_HasJPEG()
* @param Image to encode , Destination path , JPEG quality 1-100 ( clamped into range , ignored/meaningless for other formats )
* @retval 1=Success,0=Failure ( bad arguments , codec unavailable , write error - never crashes )
*/
int BasicImaging_SaveJPEG(struct Image * img,const char * filename,int quality);

/**
* @brief Save an image as PNG ( lossless ). Safe no-op ( returns 0 ) if this build has no PNG support - see BasicImaging_HasPNG()
* @param Image to encode , Destination path
* @retval 1=Success,0=Failure ( bad arguments , codec unavailable , write error - never crashes )
*/
int BasicImaging_SavePNG(struct Image * img,const char * filename);

/**
* @brief Save an image as PZP ( Portable Zipped PNM - lossless , zstd/lz4-backed ). Safe no-op ( returns 0 ) if this build has no PZP support - see BasicImaging_HasPZP()
* @param Image to encode , Destination path
* @retval 1=Success,0=Failure ( bad arguments , codec unavailable , write error - never crashes , not even on an internal pzp error that would otherwise exit() the whole process )
*/
int BasicImaging_SavePZP(struct Image * img,const char * filename);

/**
* @brief Save an image , picking JPEG vs PNG vs PZP from filename's extension ( .jpg/.jpeg , .png , .pzp , case-insensitive )
* @param Image to encode , Destination path ( its extension picks the codec ) , JPEG quality 1-100 ( ignored for PNG/PZP )
* @retval 1=Success,0=Failure ( unrecognised extension counts as failure - this never guesses/falls back to a different format )
*/
int BasicImaging_Save(struct Image * img,const char * filename,int jpegQuality);


/**
* @brief Resize an image with a separable bilinear filter ( see resize.c for the algorithm notes ). Does not modify the input.
* @param Source image ( left untouched ) , target width in pixels , target height in pixels
* @retval A newly allocated struct Image the caller owns and must release with BasicImaging_Free() , or 0 on failure ( bad arguments / out of memory )
*/
struct Image * BasicImaging_Resize(const struct Image * img,unsigned int newWidth,unsigned int newHeight);

/**
* @brief Shrink an image so neither side exceeds maxDimension , preserving aspect ratio. Never upscales :
*        an image that already fits is returned as a plain copy , so the result can always be freed the
*        same way regardless of whether resizing actually happened. Does not modify the input.
* @param Source image ( left untouched ) , the longest allowed side in pixels
* @retval A newly allocated struct Image the caller owns and must release with BasicImaging_Free() , or 0 on failure ( bad arguments / out of memory )
*/
struct Image * BasicImaging_Thumbnail(const struct Image * img,unsigned int maxDimension);


/**
* @brief The one-call convenience most Services want : load inputFilename , shrink it to fit
*        maxDimension , save it to outputFilename ( codec picked from outputFilename's extension ,
*        jpegQuality used only if that's JPEG ). If anything at all goes wrong along the way - the
*        input can't be decoded , the output codec isn't compiled in , allocation fails - this falls
*        back to copying inputFilename's bytes verbatim to outputFilename instead of leaving nothing
*        there, so the caller always ends up with SOME usable file at outputFilename.
* @param Source image path , Destination path ( extension picks the output codec ) , longest allowed side in pixels , JPEG quality 1-100 ( ignored for PNG output )
* @retval 1 = a real resized/recompressed thumbnail was written , 0 = fell back to a verbatim copy ( or even that failed , e.g. inputFilename doesn't exist / isn't readable )
*/
int BasicImaging_ThumbnailFile(const char * inputFilename,const char * outputFilename,unsigned int maxDimension,int jpegQuality);

#ifdef __cplusplus
}
#endif

#endif // BASICIMAGING_H_INCLUDED
