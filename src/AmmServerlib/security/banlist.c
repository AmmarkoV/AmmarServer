#include "banlist.h"
#include "../AmmServerlib.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/stat.h>

//One "IP BANNED_UNTIL_EPOCH LEVEL" line per IP , every process re-reads it when it changes , writers hold flock(LOCK_EX)
//If it can't be opened ( e.g. no db/ directory in the cwd ) bans still work , just only inside this process
#define BANLIST_SHARED_FILE "db/autoban"
#define BANLIST_STRIKES_TO_BAN 5
#define BANLIST_STRIKE_WINDOW_SECONDS (10*60)
#define BANLIST_FIRST_BAN_SECONDS (15*60)
#define BANLIST_MAX_BAN_SECONDS (24*60*60)
#define BANLIST_MAX_LEVEL 16 //Only bounds the shift below , BANLIST_MAX_BAN_SECONDS is reached long before that
#define BANLIST_FORGET_AFTER_SECONDS (7*24*60*60) //An expired ban keeps its escalation level this long
#define BANLIST_MAX_BANS 4096
#define BANLIST_MAX_STRIKERS 1024
#define BANLIST_RELOAD_ANYWAY_SECONDS 2 //mtime granularity is coarse , a same size rewrite by another process can look unchanged

struct BanEntry    { char ip[MAX_IP_STRING_SIZE]; long long until; unsigned int level; };
struct StrikeEntry { char ip[MAX_IP_STRING_SIZE]; long long windowStart; unsigned int strikes; };

static struct BanEntry bans[BANLIST_MAX_BANS];
static unsigned int banCount=0;
static struct StrikeEntry strikers[BANLIST_MAX_STRIKERS];
static pthread_mutex_t banMutex = PTHREAD_MUTEX_INITIALIZER;
static struct stat loadedFileState;
static int loadedFileStateValid=0;
static time_t lastReload=0;

int executeBanlist(const char * filename)
{
  AmmServer_Stub("banlist not implemented..");
  return 0;
}

static int isExempt(const char * ip)
{
  if ( (ip==0) || (ip[0]==0) ) { return 1; }
  if (strcmp(ip,"0.0.0.0")==0) { return 1; } //getSocketIPAddress() result when getpeername() fails
  if (strncmp(ip,"127.",4)==0) { return 1; }
  if (strcmp(ip,"::1")==0)     { return 1; }
  if (strncmp(ip,"::ffff:127.",11)==0) { return 1; }
  return 0;
}

static int sameFileState(const struct stat * a,const struct stat * b)
{
  return ( (a->st_ino==b->st_ino) && (a->st_size==b->st_size) &&
           (a->st_mtim.tv_sec==b->st_mtim.tv_sec) && (a->st_mtim.tv_nsec==b->st_mtim.tv_nsec) );
}

//Caller holds banMutex and a flock on fp
static void parseBanFile(FILE * fp)
{
  banCount=0;
  rewind(fp);
  struct BanEntry entry;
  while ( (banCount<BANLIST_MAX_BANS) && (fscanf(fp,"%47s %lld %u",entry.ip,&entry.until,&entry.level)==3) )
  {
    entry.ip[MAX_IP_STRING_SIZE-1]=0;
    bans[banCount++]=entry;
  }
}

//Caller holds banMutex
static void reloadIfChanged()
{
  struct stat st;
  if (stat(BANLIST_SHARED_FILE,&st)!=0) { return; }
  time_t now = time(0);
  if ( (loadedFileStateValid) && (sameFileState(&st,&loadedFileState)) && (now-lastReload<BANLIST_RELOAD_ANYWAY_SECONDS) ) { return; }
  lastReload = now;

  int fd = open(BANLIST_SHARED_FILE,O_RDONLY|O_CLOEXEC);
  if (fd<0) { return; }
  flock(fd,LOCK_SH);
  FILE * fp = fdopen(fd,"r");
  if (fp==0) { close(fd); return; }
  parseBanFile(fp);
  loadedFileStateValid = (fstat(fd,&loadedFileState)==0);
  fclose(fp); //also drops the flock
}

//Caller holds banMutex
static struct BanEntry * findBan(const char * ip)
{
  for (unsigned int i=0; i<banCount; i++)
   { if (strcmp(bans[i].ip,ip)==0) { return &bans[i]; } }
  return 0;
}

//Caller holds banMutex
static int addBan(const char * ip,int malicious)
{
  long long now = (long long) time(0);

  //Re-read under an exclusive lock so bans other processes added since our last look are kept
  FILE * fp = 0;
  int fd = open(BANLIST_SHARED_FILE,O_RDWR|O_CREAT|O_CLOEXEC,0600);
  if (fd>=0)
  {
    flock(fd,LOCK_EX);
    fp = fdopen(fd,"r+");
    if (fp==0) { close(fd); } else { parseBanFile(fp); }
  }

  //Forget bans that expired long ago
  unsigned int kept=0;
  for (unsigned int i=0; i<banCount; i++)
   { if (bans[i].until+BANLIST_FORGET_AFTER_SECONDS>=now) { bans[kept++]=bans[i]; } }
  banCount=kept;

  struct BanEntry * entry = findBan(ip);
  unsigned int level = 0;
  if (entry!=0) { level = (entry->level<BANLIST_MAX_LEVEL) ? entry->level+1 : BANLIST_MAX_LEVEL; } else
  {
    if (banCount<BANLIST_MAX_BANS) { entry=&bans[banCount++]; } else
    {
      //Full , reuse the entry whose ban ends first
      entry=&bans[0];
      for (unsigned int i=1; i<banCount; i++) { if (bans[i].until<entry->until) { entry=&bans[i]; } }
    }
    snprintf(entry->ip,MAX_IP_STRING_SIZE,"%s",ip);
    entry->until=0;
  }

  long long duration = (long long) BANLIST_FIRST_BAN_SECONDS << level;
  if ( (malicious) || (duration>BANLIST_MAX_BAN_SECONDS) ) { duration=BANLIST_MAX_BAN_SECONDS; }
  if (entry->until<now+duration) { entry->until=now+duration; } //Never shorten a longer running ban
  entry->level=level;

  AmmServer_Warning("Banning %s for %lld minutes ( %s , ban level %u )\n",ip,(entry->until-now)/60,(malicious) ? "malicious" : "too many strikes",level);

  if (fp!=0)
  {
    rewind(fp);
    for (unsigned int i=0; i<banCount; i++) { fprintf(fp,"%s %lld %u\n",bans[i].ip,bans[i].until,bans[i].level); }
    fflush(fp);
    if (ftruncate(fd,ftell(fp))!=0) { AmmServer_Warning("Could not truncate %s\n",BANLIST_SHARED_FILE); }
    loadedFileStateValid = (fstat(fd,&loadedFileState)==0);
    fclose(fp); //also drops the flock
  }
  return 1;
}

//A path segment equal to one of these , or one of these followed by a '.' ( .env.local , .env.bak .. ) , is a scanner probe
static const char * scannerProbes[] = { "wp-login.php" , ".env" , 0 };

int banlist_isScannerProbe(const char * resource)
{
  if (resource==0) { return 0; }
  const char * segment = resource;
  while (*segment!=0)
  {
    while (*segment=='/') { segment++; }
    unsigned int length = (unsigned int) strcspn(segment,"/?#");
    for (unsigned int i=0; scannerProbes[i]!=0; i++)
    {
      unsigned int probeLength = (unsigned int) strlen(scannerProbes[i]);
      if ( (length>=probeLength) && (strncmp(segment,scannerProbes[i],probeLength)==0) &&
           ( (length==probeLength) || (segment[probeLength]=='.') ) ) { return 1; }
    }
    if ( (segment[length]=='?') || (segment[length]=='#') ) { return 0; }
    segment+=length;
  }
  return 0;
}

int banlist_isIPBanned(const char * ip)
{
  if (isExempt(ip)) { return 0; }
  pthread_mutex_lock(&banMutex);
  reloadIfChanged();
  struct BanEntry * entry = findBan(ip);
  int banned = ( (entry!=0) && (entry->until>(long long) time(0)) );
  pthread_mutex_unlock(&banMutex);
  return banned;
}

int banlist_signalBadBehaviour(const char * ip)
{
  if (isExempt(ip)) { return 0; }
  if (banlist_isIPBanned(ip)) { return 1; }

  long long now = (long long) time(0);
  int banned = 0;
  pthread_mutex_lock(&banMutex);
  struct StrikeEntry * striker = 0;
  struct StrikeEntry * oldest  = &strikers[0];
  for (unsigned int i=0; i<BANLIST_MAX_STRIKERS; i++)
  {
    if (strcmp(strikers[i].ip,ip)==0) { striker=&strikers[i]; break; }
    if (strikers[i].windowStart<oldest->windowStart) { oldest=&strikers[i]; }
  }
  if (striker==0)
  {
    striker=oldest; //Empty slots have windowStart 0 so they are picked first
    snprintf(striker->ip,MAX_IP_STRING_SIZE,"%s",ip);
    striker->strikes=0;
    striker->windowStart=now;
  }
  if (striker->windowStart+BANLIST_STRIKE_WINDOW_SECONDS<now) { striker->strikes=0; striker->windowStart=now; }
  striker->strikes++;
  AmmServer_Warning("Strike %u/%u for %s\n",striker->strikes,BANLIST_STRIKES_TO_BAN,ip);

  if (striker->strikes>=BANLIST_STRIKES_TO_BAN)
  {
    memset(striker,0,sizeof(struct StrikeEntry));
    banned = addBan(ip,0);
  }
  pthread_mutex_unlock(&banMutex);
  return banned;
}

int banlist_signalMalicious(const char * ip)
{
  if (isExempt(ip)) { return 0; }
  pthread_mutex_lock(&banMutex);
  int banned = addBan(ip,1);
  pthread_mutex_unlock(&banMutex);
  return banned;
}
