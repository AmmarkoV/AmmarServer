#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

#include "../../AmmServerlib/AmmServerlib.h"
#include "../../Hashmap/hashmap.h"

#define MAX_POLLS 1000
#define MAX_OPTIONS 60
#define MAX_RESPONSES 300
#define MAX_STRING_SIZE 256
#define LINE_MAX_LENGTH 1024

enum voteValue { VOTE_BLANK=0, VOTE_YES=1, VOTE_MAYBE=2, VOTE_NO=3 };

struct pollOption
{
  unsigned int year,month,day;
  unsigned char hasStartTime; unsigned int startHour,startMinute;
  unsigned char hasEndTime;   unsigned int endHour,endMinute;
};

struct pollResponse
{
  char name[64];      //Free text , kept only in memory + responses/<n>_name.txt , never in poll.ini
  unsigned char hasEmail;
  char email[128];    //Free text , kept only in memory + responses/<n>_email.txt , never in poll.ini
  unsigned char votes[MAX_OPTIONS];
};

struct poll
{
  char id[32];                //"a%06u" , server generated only
  char title[MAX_STRING_SIZE];//Free text , kept only in memory + title.txt , never in poll.ini
  char ownerToken[40];
  unsigned char closed;
  int finalizedOptionIndex;   //-1 = not finalized

  unsigned int numberOfOptions;
  struct pollOption options[MAX_OPTIONS];

  unsigned int numberOfResponses;
  struct pollResponse responses[MAX_RESPONSES];
};

extern struct poll polls[MAX_POLLS];
extern unsigned int numberOfPolls;
extern unsigned int nextPollUID;
extern struct hashMap * pollHashMap;

extern struct AmmServer_Instance * default_server;

int loadAllPolls();
int unloadAllPolls();

int loadPoll(const char * id , struct poll * p);
int savePollMeta(struct poll * p);
int loadResponses(struct poll * p);

int createPoll(const char * title , struct pollOption * options , unsigned int numberOfOptions , char * outID , unsigned int outIDSize , char * outOwnerToken , unsigned int outOwnerTokenSize);

struct poll * findPoll(const char * id);
int isOwnerTokenValid(struct poll * p , const char * token);

int findResponseByName(struct poll * p , const char * name); //Read-only lookup , -1 if not found
int findOrCreateResponseSlot(struct poll * p , const char * name);
int saveResponse(struct poll * p , unsigned int responseIndex);

void generateRandomToken(char * out , unsigned int outSize);
void formatOptionHTML(struct pollOption * opt , char * buf , unsigned int bufSize);

#endif // STATE_H_INCLUDED
