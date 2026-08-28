#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

#include "../../AmmServerlib/AmmServerlib.h"
#include "../../Hashmap/hashmap.h"
#include "../../UserAccounts/userAccounts.h"

#define MAX_PROJECTS 1000
#define MAX_FILES_PER_PROJECT 200
#define MAX_COLLABORATORS 50
#define MAX_STRING_SIZE 256
#define LINE_MAX_LENGTH 1024

struct projectFile
{
  char relativePath[MAX_STRING_SIZE];
  unsigned int version;
};

struct project
{
  char id[32];
  char title[MAX_STRING_SIZE];
  char owner[64];
  char collaborators[MAX_COLLABORATORS][64];
  unsigned int numberOfCollaborators;
  unsigned char isPublic;

  unsigned int numberOfFiles;
  struct projectFile files[MAX_FILES_PER_PROJECT];

  unsigned int pdfVersion; //Bumped on each successful compile , used to build main_v<n>.pdf so a client never sees a stale cached PDF
};

extern struct project projects[MAX_PROJECTS];
extern unsigned int numberOfProjects;
extern unsigned int nextProjectUID;
extern struct hashMap * projectHashMap; //id string -> slot index in projects[] ( ULong payload , same pattern as HabChan's board/thread hashmaps )

extern struct UserAccountDatabase * uadb;

extern struct AmmServer_Instance * default_server;

int loadAllProjects();
int unloadAllProjects();

int loadProjectMeta(const char * id , struct project * p);
int saveProjectMeta(struct project * p);
int scanProjectFiles(struct project * p);

int createProject(const char * owner , const char * title , char * outID , unsigned int outIDSize);

int userCanAccessProject(struct project * p , const char * username);
int findProjectFileIndex(struct project * p , const char * relativePath);
struct project * findProject(const char * id);

int isSafeRelativePath(const char * relativePath);
int isEditableTextFile(const char * relativePath);

#endif // STATE_H_INCLUDED
