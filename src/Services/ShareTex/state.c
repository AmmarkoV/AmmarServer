#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "state.h"
#include "../../InputParser/InputParser_C.h"

struct project projects[MAX_PROJECTS]={{{0}}};
unsigned int numberOfProjects=0;
unsigned int nextProjectUID=1;
struct hashMap * projectHashMap=0;

struct UserAccountDatabase * uadb=0;
struct AmmServer_Instance * default_server=0;


int isSafeRelativePath(const char * relativePath)
{
  if ( (relativePath==0) || (strlen(relativePath)==0) ) { return 0; }
  if (strlen(relativePath)>=MAX_STRING_SIZE) { return 0; }
  if (relativePath[0]=='/') { return 0; }
  if (strstr(relativePath,"..")!=0) { return 0; }

  unsigned int slashCount=0;
  unsigned int i=0;
  for (i=0; relativePath[i]!=0; i++)
  {
    char c=relativePath[i];
    if (c=='/') { ++slashCount; continue; }
    if ( isalnum((unsigned char)c) || (c=='_') || (c=='.') || (c=='-') ) { continue; }
    return 0; //Disallowed character
  }

  if (slashCount>3) { return 0; } //Keep nesting shallow , matches scanProjectFiles() depth cap

  return 1;
}


int isEditableTextFile(const char * relativePath)
{
  static const char * editableExtensions[] = { ".tex",".bib",".sty",".bst",".cls",".txt",".md",".sh",0 };

  const char * dot = strrchr(relativePath,'.');
  if (dot==0) { return 0; }

  unsigned int i=0;
  for (i=0; editableExtensions[i]!=0; i++)
  {
    if (strcasecmp(dot,editableExtensions[i])==0) { return 1; }
  }
  return 0;
}


int userCanAccessProject(struct project * p , const char * username)
{
  if ( (p==0) || (username==0) ) { return 0; }
  if (p->isPublic) { return 1; }
  if (strcmp(p->owner,username)==0) { return 1; }

  unsigned int i=0;
  for (i=0; i<p->numberOfCollaborators; i++)
  {
    if (strcmp(p->collaborators[i],username)==0) { return 1; }
  }
  return 0;
}


int findProjectFileIndex(struct project * p , const char * relativePath)
{
  if ( (p==0) || (relativePath==0) ) { return -1; }
  unsigned int i=0;
  for (i=0; i<p->numberOfFiles; i++)
  {
    if (strcmp(p->files[i].relativePath,relativePath)==0) { return (int) i; }
  }
  return -1;
}


struct project * findProject(const char * id)
{
  if (id==0) { return 0; }
  unsigned long slot=0;
  if (! hashMap_GetULongPayload(projectHashMap,id,&slot) ) { return 0; }
  if (slot>=numberOfProjects) { return 0; }
  return &projects[slot];
}


//Recursively walks data/projects/<id>/files/ populating p->files[] , depth-capped to keep this simple and bounded
static void scanProjectFilesRecursive(struct project * p , const char * baseDir , const char * relativePrefix , unsigned int depth)
{
  if (depth>4) { return; }
  if (p->numberOfFiles>=MAX_FILES_PER_PROJECT) { return; }

  DIR * dp = opendir(baseDir);
  if (dp==0) { return; }

  struct dirent * ep;
  while ( (ep=readdir(dp)) != 0 )
  {
    if (strcmp(ep->d_name,".")==0) { continue; }
    if (strcmp(ep->d_name,"..")==0) { continue; }
    if (p->numberOfFiles>=MAX_FILES_PER_PROJECT) { break; }

    char fullPath[MAX_STRING_SIZE*2]={0};
    snprintf(fullPath,sizeof(fullPath),"%s/%s",baseDir,ep->d_name);

    char relPath[MAX_STRING_SIZE]={0};
    if (strlen(relativePrefix)>0) { snprintf(relPath,sizeof(relPath),"%s/%s",relativePrefix,ep->d_name); }
    else                          { snprintf(relPath,sizeof(relPath),"%s",ep->d_name); }

    struct stat st={0};
    if (stat(fullPath,&st)!=0) { continue; }

    if (S_ISDIR(st.st_mode))
    {
      scanProjectFilesRecursive(p,fullPath,relPath,depth+1);
    } else
    if (S_ISREG(st.st_mode))
    {
      snprintf(p->files[p->numberOfFiles].relativePath,MAX_STRING_SIZE,"%s",relPath);
      p->files[p->numberOfFiles].version=1;
      ++p->numberOfFiles;
    }
  }

  closedir(dp);
}


int scanProjectFiles(struct project * p)
{
  if (p==0) { return 0; }
  p->numberOfFiles=0;

  char dirPath[MAX_STRING_SIZE*2]={0};
  snprintf(dirPath,sizeof(dirPath),"data/projects/%s/files",p->id);

  scanProjectFilesRecursive(p,dirPath,"",0);
  return 1;
}


int loadProjectMeta(const char * id , struct project * p)
{
  if ( (id==0) || (p==0) ) { return 0; }
  memset(p,0,sizeof(struct project));
  snprintf(p->id,sizeof(p->id),"%s",id);

  char filename[MAX_STRING_SIZE*2]={0};
  snprintf(filename,sizeof(filename),"data/projects/%s/meta.ini",id);

  FILE * fp = fopen(filename,"r");
  if (fp==0) { fprintf(stderr,"Cannot open %s\n",filename); return 0; }

  struct InputParserC * ipc = InputParser_Create(LINE_MAX_LENGTH,5);
  if (ipc==0) { fclose(fp); return 0; }

  char line[LINE_MAX_LENGTH]={0};
  while (!feof(fp))
  {
    if (fgets(line,LINE_MAX_LENGTH,fp)!=0)
    {
      unsigned int words = InputParser_SeperateWords(ipc,line,0);
      if (words>0)
      {
        if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"TITLE")==1)
        {
          InputParser_GetWord(ipc,1,p->title,MAX_STRING_SIZE);
        } else
        if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"OWNER")==1)
        {
          InputParser_GetWord(ipc,1,p->owner,sizeof(p->owner));
        } else
        if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"PUBLIC")==1)
        {
          p->isPublic = InputParser_GetWordInt(ipc,1);
        } else
        if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"COLLABORATOR")==1)
        {
          if (p->numberOfCollaborators<MAX_COLLABORATORS)
          {
            InputParser_GetWord(ipc,1,p->collaborators[p->numberOfCollaborators],sizeof(p->collaborators[0]));
            ++p->numberOfCollaborators;
          }
        } else
        if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"PDFVERSION")==1)
        {
          p->pdfVersion = InputParser_GetWordInt(ipc,1);
        }
      }
    }
  }

  InputParser_Destroy(ipc);
  fclose(fp);

  scanProjectFiles(p);
  return 1;
}


int saveProjectMeta(struct project * p)
{
  if (p==0) { return 0; }

  char filename[MAX_STRING_SIZE*2]={0};
  snprintf(filename,sizeof(filename),"data/projects/%s/meta.ini",p->id);

  FILE * fp = fopen(filename,"w");
  if (fp==0) { fprintf(stderr,"Cannot open %s for writing\n",filename); return 0; }

  fprintf(fp,"title(%s)\n",p->title);
  fprintf(fp,"owner(%s)\n",p->owner);
  fprintf(fp,"public(%u)\n",p->isPublic);
  fprintf(fp,"pdfversion(%u)\n",p->pdfVersion);

  unsigned int i=0;
  for (i=0; i<p->numberOfCollaborators; i++)
  {
    fprintf(fp,"collaborator(%s)\n",p->collaborators[i]);
  }

  fclose(fp);
  return 1;
}


int loadAllProjects()
{
  projectHashMap = hashMap_Create(100,100,0,1);

  DIR * dp = opendir("data/projects");
  if (dp==0) { fprintf(stderr,"Cannot open data/projects directory\n"); return 0; }

  struct dirent * ep;
  unsigned int highestNumericID=0;

  while ( (ep=readdir(dp)) != 0 )
  {
    if (strcmp(ep->d_name,".")==0)  { continue; }
    if (strcmp(ep->d_name,"..")==0) { continue; }

    if (numberOfProjects>=MAX_PROJECTS) { break; }

    unsigned int slot = numberOfProjects;
    if ( loadProjectMeta(ep->d_name,&projects[slot]) )
    {
      hashMap_AddULong(projectHashMap,ep->d_name,slot);
      ++numberOfProjects;

      unsigned int numericPart = (unsigned int) atoi(ep->d_name+ ((ep->d_name[0]=='p') ? 1 : 0) );
      if (numericPart>highestNumericID) { highestNumericID=numericPart; }
    }
  }

  closedir(dp);

  nextProjectUID = highestNumericID+1;

  fprintf(stderr,"Loaded %u projects , nextProjectUID=%u\n",numberOfProjects,nextProjectUID);
  return 1;
}


int unloadAllProjects()
{
  if (projectHashMap!=0) { hashMap_Destroy(projectHashMap); projectHashMap=0; }
  return 1;
}


int createProject(const char * owner , const char * title , char * outID , unsigned int outIDSize)
{
  if ( (owner==0) || (title==0) ) { return 0; }
  if (numberOfProjects>=MAX_PROJECTS) { fprintf(stderr,"createProject : site is full\n"); return 0; }

  char id[32]={0};
  snprintf(id,sizeof(id),"p%06u",nextProjectUID);

  char dirPath[MAX_STRING_SIZE*2]={0};
  snprintf(dirPath,sizeof(dirPath),"data/projects/%s",id);
  if (mkdir(dirPath,0755)!=0) { fprintf(stderr,"createProject : cannot create %s\n",dirPath); return 0; }

  char filesPath[MAX_STRING_SIZE*2]={0};
  snprintf(filesPath,sizeof(filesPath),"data/projects/%s/files",id);
  if (mkdir(filesPath,0755)!=0) { fprintf(stderr,"createProject : cannot create %s\n",filesPath); return 0; }

  char mainTexPath[MAX_STRING_SIZE*2]={0};
  snprintf(mainTexPath,sizeof(mainTexPath),"%s/main.tex",filesPath);
  const char * blankTemplate =
    "\\documentclass{article}\n"
    "\\usepackage[utf8]{inputenc}\n"
    "\\title{Untitled Document}\n"
    "\\author{}\n"
    "\\date{}\n"
    "\\begin{document}\n"
    "\\maketitle\n"
    "\n"
    "Start writing here.\n"
    "\n"
    "\\end{document}\n";
  AmmServer_WriteFileFromMemory(mainTexPath,blankTemplate,strlen(blankTemplate));

  unsigned int slot = numberOfProjects;
  memset(&projects[slot],0,sizeof(struct project));
  snprintf(projects[slot].id,sizeof(projects[slot].id),"%s",id);
  snprintf(projects[slot].title,MAX_STRING_SIZE,"%s",title);
  snprintf(projects[slot].owner,sizeof(projects[slot].owner),"%s",owner);
  projects[slot].isPublic=0;
  scanProjectFiles(&projects[slot]);
  saveProjectMeta(&projects[slot]);

  hashMap_AddULong(projectHashMap,id,slot);
  ++numberOfProjects;
  ++nextProjectUID;

  if ( (outID!=0) && (outIDSize>0) ) { snprintf(outID,outIDSize,"%s",id); }
  return 1;
}
