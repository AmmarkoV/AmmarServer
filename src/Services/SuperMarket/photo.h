#ifndef PHOTO_H_INCLUDED
#define PHOTO_H_INCLUDED

#include "state.h"

//POST a=img : sniff the upload's magic bytes ( jpg/png only ) and resize+reencode it via BasicImaging ,
//mirroring go.php's convert pipeline ( shrink-only to 1000px , quality 82 , EXIF auto-orient )
//Writes the resulting cart JSON ( with an "error" field on failure ) straight into rqst->content
void * photoUpload_callback(struct AmmServer_DynamicRequest * rqst,const char * token);

//GET ?img=ID : stream a previously uploaded photo straight off disk with the correct Content-Type
void * photoStream_callback(struct AmmServer_DynamicRequest * rqst,const char * token,const char * rawImgID);

#endif // PHOTO_H_INCLUDED
