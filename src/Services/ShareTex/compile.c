#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compile.h"
#include "auth.h"

void * compile_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64]={0};
  unsigned int authenticated = getAuthenticatedUser(rqst,username,sizeof(username));

  char projectID[32]={0};
  _POSTcpy(rqst,"project",projectID,sizeof(projectID));

  struct project * p = authenticated ? findProject(projectID) : 0;

  if ( (p==0) || (! userCanAccessProject(p,username)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"###FAIL###\nAccess denied.");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  char filesDir[MAX_STRING_SIZE*2]={0};
  snprintf(filesDir,sizeof(filesDir),"data/projects/%s/files",p->id);

  char logPath[MAX_STRING_SIZE*2]={0};
  snprintf(logPath,sizeof(logPath),"data/projects/%s/compile.log",p->id);

  //Fixed pipeline , fixed filename ( "main.tex" is never taken from user input ) , no -shell-escape : the project
  //content ( arbitrary , multi-tenant , collaboratively edited LaTeX ) can influence what pdflatex DOES , but not
  //what command gets run.
  char command[MAX_STRING_SIZE*4]={0};
  snprintf(command,sizeof(command),
           "cd '%s' && "
           "timeout 60 pdflatex -interaction=nonstopmode -no-shell-escape main.tex > '../compile.log' 2>&1 && "
           "( timeout 30 bibtex main >> '../compile.log' 2>&1 ; true ) && "
           "timeout 60 pdflatex -interaction=nonstopmode -no-shell-escape main.tex >> '../compile.log' 2>&1 && "
           "timeout 60 pdflatex -interaction=nonstopmode -no-shell-escape main.tex >> '../compile.log' 2>&1",
           filesDir);

  char scratch[16]={0};
  AmmServer_ExecuteCommandLine(command,scratch,sizeof(scratch));

  char pdfPath[MAX_STRING_SIZE*2]={0};
  snprintf(pdfPath,sizeof(pdfPath),"%s/main.pdf",filesDir);

  unsigned int logLength=0;
  char * logContent = AmmServer_ReadFileToMemory(logPath,&logLength);

  #define LOG_TAIL_SIZE 4000
  const char * logTail = "";
  if (logContent!=0)
  {
    logTail = (logLength>LOG_TAIL_SIZE) ? (logContent+logLength-LOG_TAIL_SIZE) : logContent;
  }

  if ( AmmServer_FileExists(pdfPath) )
  {
    ++p->pdfVersion;
    saveProjectMeta(p);

    char versionedName[64]={0};
    snprintf(versionedName,sizeof(versionedName),"main_v%u.pdf",p->pdfVersion);
    char versionedPath[MAX_STRING_SIZE*2]={0};
    snprintf(versionedPath,sizeof(versionedPath),"%s/%s",filesDir,versionedName);

    char copyCommand[MAX_STRING_SIZE*4]={0};
    snprintf(copyCommand,sizeof(copyCommand),"cp '%s' '%s'",pdfPath,versionedPath);
    AmmServer_ExecuteCommandLine(copyCommand,scratch,sizeof(scratch));

    snprintf(rqst->content,rqst->MAXcontentSize,"###OK %s###\n%s",versionedName,logTail);
  } else
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"###FAIL###\n%s",logTail);
  }
  rqst->contentSize=strlen(rqst->content);

  if (logContent!=0) { free(logContent); }
  return 0;
}
