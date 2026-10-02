/*
AmmarServer , QuestTeleop relay : same-host link to the robot side through SharedMemoryVideoBuffers

Context QUEST_SHM_CONTEXT , two streams ( layouts shared with humanoid_controller/quest.py ) :

   quest_pose  written here , every pose the Quest POSTs :
               struct QuestPoseHeader , then `length` bytes of JSON
   quest_view  written by the robot side , raw pixels ( no JPEG on its side ) :
               struct QuestViewHeader , then `eyes` images of height x width x 3 RGB , one after the other ,
               which this relay JPEG-encodes side by side ( left | right ) when the Quest asks for /frame.jpg
*/
#ifndef QUESTTELEOP_SHM_LINK_H_INCLUDED
#define QUESTTELEOP_SHM_LINK_H_INCLUDED

#include <stdint.h>

#define QUEST_SHM_CONTEXT "quest_teleop.shm"
#define QUEST_POSE_MAGIC 0x31505451u   // "QTP1"
#define QUEST_VIEW_MAGIC 0x31565451u   // "QTV1"

struct QuestPoseHeader
{
  uint32_t magic;
  uint32_t length;   //bytes of JSON that follow
  uint64_t seq;      //increments with every pose
};

struct QuestViewHeader
{
  uint32_t magic;
  uint32_t width;    //of one eye
  uint32_t height;
  uint32_t eyes;     //1 = mono , 2 = stereo ( left image then right image )
  float    fovy;     //vertical field of view , degrees
  float    quat[4];  //x,y,z,w : the head orientation the view was rendered for
  uint32_t reserved[7];
};                   //64 bytes

//Creates the context and the quest_pose stream ; 0 if SharedMemoryVideoBuffers is unavailable
int shmLink_start(unsigned int maxPoseSize);
void shmLink_stop();

void shmLink_publishPose(const char * json,unsigned int length);

/*If the robot side published a view newer than the last one taken , JPEG-encode it ( with the "fovy=.. q=.. stereo=1"
  comment the APK reads ) into jpeg ( at most maxSize bytes ) and return its size , else 0 */
unsigned long shmLink_takeViewJPEG(char * jpeg,unsigned long maxSize,int quality);

#endif
