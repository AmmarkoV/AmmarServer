#ifndef MEDIA_H_INCLUDED
#define MEDIA_H_INCLUDED

#include "library.h"

#define MAX_SUBTITLE_TRACKS 64

enum SubtitleKind
{
  SUBTITLE_SIDECAR=0, //A .srt/.vtt file next to the video , key = its file name
  SUBTITLE_EMBEDDED   //A text subtitle stream inside the video , key = ffmpeg stream index
};

struct SubtitleTrack
{
  enum SubtitleKind kind;
  char key[256];
  char label[256];
};

//cacheDirectory gets thumbs/ and subs/ sub directories
int media_init(const char * cacheDirectory);

//Finds or generates ( ffmpeg ) a thumbnail , 1=outPath holds a jpg to serve
int media_thumbnail(const struct Video * video,char * outPath,unsigned int outPathSize);

//Sidecar files first , then embedded text streams ( probed with ffprobe once , cached )
unsigned int media_listSubtitles(const struct Video * video,struct SubtitleTrack * tracks,unsigned int maxTracks);

//Resolves a subtitle track to a file on disk , extracting embedded streams ( ffmpeg , cached ) if needed. 1=Success
int media_sidecarSubtitlePath(const struct Video * video,const char * name,char * outPath,unsigned int outPathSize);
int media_embeddedSubtitlePath(const struct Video * video,unsigned int streamIndex,char * outPath,unsigned int outPathSize);

#endif // MEDIA_H_INCLUDED
