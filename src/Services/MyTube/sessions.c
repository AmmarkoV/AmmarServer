#include "sessions.h"


unsigned int getAUserIDForSession(struct videoCollection * db , const char * sessionQuery , const char * sessionToken , int * foundSession)
{
  //Sessions not implemented
  *foundSession=0;
  /*
  AmmServer_Warning("Searching for session `%s` among %u sessions \n\n",query,db->numberOfLoadedVideos);
  unsigned int i=0;

  for (i=0; i<db->numberOfLoadedVideos; i++)
  {
      if (strstr(db->video[i].filename , query)!=0)
      {
        AmmServer_Success("Found it @ %u \n\n",i);
        *foundSession=1;
        return i;
      }
  }
  AmmServer_Error("Could not Find it %s \n\n",query);
  */
  return 0;
}
