#ifndef THUMBNAILER_H_INCLUDED
#define THUMBNAILER_H_INCLUDED

#define GENERATE_NEW_THUMBNAILS_LIVE 0

int createThumbnailDir(const char * outputDir);
char *  generateThumbnailOfVideo(int live,const char * videoDirectory,const char * videofile,const char * thumbDirectory);

#endif // THUMBNAILER_H_INCLUDED
