#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "editor.h"
#include "auth.h"

#define FILETREE_BUFFER_CAPACITY 16384

static void appendFileTreeEntry(char * buffer , unsigned int bufferCapacity , const char * sessionID , const char * projectID , const char * relativePath , const char * activeFile)
{
  char escapedPath[MAX_STRING_SIZE*6]={0};
  AmmServer_HTMLEscape(relativePath,escapedPath,sizeof(escapedPath));

  const char * activeClass = (strcmp(relativePath,activeFile)==0) ? " class=\"active\"" : "";

  char chunk[MAX_STRING_SIZE*8]={0};
  if (isEditableTextFile(relativePath))
  {
    snprintf(chunk,sizeof(chunk),"<a%s href=\"editor.html?s=%s&project=%s&file=%s\">%s</a>",
             activeClass,sessionID,projectID,escapedPath,escapedPath);
  } else
  {
    snprintf(chunk,sizeof(chunk),"<span style=\"display:block;padding:6px 8px;opacity:0.5;font-size:12px;\">%s</span>",escapedPath);
  }
  strncat(buffer,chunk,bufferCapacity - strlen(buffer) - 1);
}


void * editorPage_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64]={0};
  char sessionID[64]={0};
  _GETcpy(rqst,"s",sessionID,sizeof(sessionID));

  if ( ! getAuthenticatedUser(rqst,username,sizeof(username)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Session expired. <a href=\"index.html\">Log in</a></body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  char projectID[32]={0};
  _GETcpy(rqst,"project",projectID,sizeof(projectID));

  struct project * p = findProject(projectID);
  if ( (p==0) || (! userCanAccessProject(p,username)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Project not found or access denied. <a href=\"dashboard.html?s=%s\">Back</a></body></html>",sessionID);
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  char activeFile[MAX_STRING_SIZE]={0};
  if ( ! _GETcpy(rqst,"file",activeFile,sizeof(activeFile)) ) { snprintf(activeFile,sizeof(activeFile),"main.tex"); }
  if ( ! isSafeRelativePath(activeFile) ) { snprintf(activeFile,sizeof(activeFile),"main.tex"); }

  char * fileTreeHTML = (char*) malloc(FILETREE_BUFFER_CAPACITY);
  if (fileTreeHTML==0) { return 0; }
  fileTreeHTML[0]=0;

  unsigned int i=0;
  for (i=0; i<p->numberOfFiles; i++)
  {
    appendFileTreeEntry(fileTreeHTML,FILETREE_BUFFER_CAPACITY,sessionID,projectID,p->files[i].relativePath,activeFile);
  }

  char newFileForm[700]={0};
  snprintf(newFileForm,sizeof(newFileForm),
           "<form id=\"newFileForm\" method=\"post\" enctype=\"multipart/form-data\" action=\"newFile.html\" style=\"margin-top:10px;\">"
           "<input type=\"hidden\" name=\"s\" value=\"%s\"><input type=\"hidden\" name=\"project\" value=\"%s\">"
           "<input type=\"text\" name=\"relativePath\" placeholder=\"sec/new.tex\" style=\"width:120px;font-size:11px;\">"
           "<button type=\"submit\" style=\"font-size:11px;\">+ file</button></form>",
           sessionID,projectID);
  strncat(fileTreeHTML,newFileForm,FILETREE_BUFFER_CAPACITY - strlen(fileTreeHTML) - 1);

  int fileIndex = findProjectFileIndex(p,activeFile);
  unsigned int initialVersion = (fileIndex>=0) ? p->files[fileIndex].version : 1;

  //Switching files is a full page reload , so without this the PDF pane would forget the last successful
  //compile every time you click a different file in the tree , forcing an expensive recompile just to look
  //at it again. If a compiled PDF already exists on disk for this project , point the iframe straight at it.
  char pdfSrcAttribute[MAX_STRING_SIZE*2+32]={0};
  char compileLogInitial[128]="Not compiled yet.";
  if (p->pdfVersion>0)
  {
    char pdfFilePath[MAX_STRING_SIZE*2]={0};
    snprintf(pdfFilePath,sizeof(pdfFilePath),"data/projects/%s/files/main_v%u.pdf",p->id,p->pdfVersion);
    if ( AmmServer_FileExists(pdfFilePath) )
    {
      snprintf(pdfSrcAttribute,sizeof(pdfSrcAttribute)," src=\"projects/%s/files/main_v%u.pdf\"",p->id,p->pdfVersion);
      snprintf(compileLogInitial,sizeof(compileLogInitial),"Showing the last successful compile (v%u). Click Compile to refresh it.",p->pdfVersion);
    }
  }

  char filePath[MAX_STRING_SIZE*2]={0};
  snprintf(filePath,sizeof(filePath),"data/projects/%s/files/%s",p->id,activeFile);
  unsigned int contentLength=0;
  char * fileContent = AmmServer_ReadFileToMemory(filePath,&contentLength);

  char escapedTitle[MAX_STRING_SIZE*6]={0};
  AmmServer_HTMLEscape(p->title,escapedTitle,sizeof(escapedTitle));

  char escapedActiveFile[MAX_STRING_SIZE*6]={0};
  AmmServer_HTMLEscape(activeFile,escapedActiveFile,sizeof(escapedActiveFile));

  //The textarea content is escaped for HTML but must NOT be re-escaped on save/round-trip , the browser decodes
  //entities back to raw text when reading a textarea's value , so this is safe and standard.
  unsigned int escapedContentCapacity = (contentLength+1)*6 + 64;
  char * escapedContent = (char*) malloc(escapedContentCapacity);
  if (escapedContent!=0)
  {
    escapedContent[0]=0;
    if (fileContent!=0) { AmmServer_HTMLEscape(fileContent,escapedContent,escapedContentCapacity); }
  }

  unsigned int responseCapacity = rqst->MAXcontentSize;
  unsigned int needed = strlen(fileTreeHTML) + (escapedContent!=0?strlen(escapedContent):0) + strlen(escapedTitle) + strlen(escapedActiveFile) + 8192;

  if (needed>=responseCapacity)
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>This file is too large to open in the editor.</body></html>");
    rqst->contentSize=strlen(rqst->content);
  } else
  {
    snprintf(rqst->content,rqst->MAXcontentSize,
      "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>%s - ShareTex</title>"
      "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\"></head><body>"
      "<div class=\"topbar\"><h1>ShareTex</h1><div class=\"right\">%s &nbsp; <a href=\"dashboard.html?s=%s\">Projects</a></div></div>"
      "<div class=\"editorWrap\">"
      "<div class=\"fileTree\">%s</div>"
      "<div class=\"editorPane\">"
      "<div class=\"editorToolbar\">"
      "<b>%s</b>"
      "<button id=\"compileBtn\">Compile</button>"
      "<span class=\"status\" id=\"saveStatus\"></span>"
      "<span class=\"status\" id=\"cursorStatus\"></span>"
      "</div>"
      "<div class=\"editorBody\">"
      "<pre id=\"editorMirror\"></pre>"
      "<textarea id=\"editorTextarea\" spellcheck=\"false\">%s</textarea>"
      "</div>"
      "<div class=\"compileLog\" id=\"compileLog\">%s</div>"
      "</div>"
      "<div class=\"pdfPane\"><iframe id=\"pdfFrame\"%s></iframe></div>"
      "</div>"
      "<input type=\"hidden\" id=\"sessionID\" value=\"%s\">"
      "<input type=\"hidden\" id=\"projectID\" value=\"%s\">"
      "<input type=\"hidden\" id=\"activeFile\" value=\"%s\">"
      "<input type=\"hidden\" id=\"initialVersion\" value=\"%u\">"
      "<input type=\"hidden\" id=\"username\" value=\"%s\">"
      "<script src=\"editor.js\"></script>"
      "</body></html>",
      escapedTitle,
      username,sessionID,
      fileTreeHTML,
      escapedActiveFile,
      (escapedContent!=0)?escapedContent:"",
      compileLogInitial,
      pdfSrcAttribute,
      sessionID,projectID,escapedActiveFile,initialVersion,username);
    rqst->contentSize=strlen(rqst->content);
  }

  if (fileContent!=0) { free(fileContent); }
  if (escapedContent!=0) { free(escapedContent); }
  free(fileTreeHTML);
  return 0;
}


void * getFileContent_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64]={0};
  if ( ! getAuthenticatedUser(rqst,username,sizeof(username)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"###VERSION 0###\n");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  char projectID[32]={0};
  _GETcpy(rqst,"project",projectID,sizeof(projectID));
  char relativePath[MAX_STRING_SIZE]={0};
  _GETcpy(rqst,"file",relativePath,sizeof(relativePath));

  struct project * p = findProject(projectID);
  if ( (p==0) || (! userCanAccessProject(p,username)) || (! isSafeRelativePath(relativePath)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"###VERSION 0###\n");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  int fileIndex = findProjectFileIndex(p,relativePath);
  unsigned int version = (fileIndex>=0) ? p->files[fileIndex].version : 0;

  char header[64]={0};
  snprintf(header,sizeof(header),"###VERSION %u###\n",version);
  unsigned int headerLen = strlen(header);

  char filePath[MAX_STRING_SIZE*2]={0};
  snprintf(filePath,sizeof(filePath),"data/projects/%s/files/%s",p->id,relativePath);
  unsigned int contentLength=0;
  char * fileContent = AmmServer_ReadFileToMemory(filePath,&contentLength);

  if ( (headerLen+contentLength+1) >= rqst->MAXcontentSize )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"###VERSION %u###\n",version); //Too big to echo back whole ; client keeps its local copy
    rqst->contentSize=strlen(rqst->content);
  } else
  {
    memcpy(rqst->content,header,headerLen);
    if (fileContent!=0) { memcpy(rqst->content+headerLen,fileContent,contentLength); }
    rqst->contentSize=headerLen+contentLength;
  }

  if (fileContent!=0) { free(fileContent); }
  return 0;
}


void * saveFileContent_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64]={0};
  unsigned int authenticated = getAuthenticatedUser(rqst,username,sizeof(username));

  char projectID[32]={0};
  _POSTcpy(rqst,"project",projectID,sizeof(projectID));
  char relativePath[MAX_STRING_SIZE]={0};
  _POSTcpy(rqst,"file",relativePath,sizeof(relativePath));

  struct project * p = authenticated ? findProject(projectID) : 0;
  unsigned int allowed = (p!=0) && userCanAccessProject(p,username) && isSafeRelativePath(relativePath);

  unsigned int newVersion=0;
  unsigned int conflict=0;

  if (allowed)
  {
    int fileIndex = findProjectFileIndex(p,relativePath);
    if (fileIndex<0)
    {
      //Saving to a name that isn't in the current listing yet ( e.g. right after newFile.html ) is fine ,
      //just refresh the listing so we can track its version too.
      scanProjectFiles(p);
      fileIndex = findProjectFileIndex(p,relativePath);
    }

    if (fileIndex>=0)
    {
      char baseVersionStr[16]={0};
      _POSTcpy(rqst,"baseVersion",baseVersionStr,sizeof(baseVersionStr));
      unsigned int baseVersion = (unsigned int) atoi(baseVersionStr);
      if ( (baseVersion!=0) && (baseVersion!=p->files[fileIndex].version) ) { conflict=1; }

      unsigned int contentLength=0;
      const char * content = _POST(rqst,"content",&contentLength);

      char filePath[MAX_STRING_SIZE*2]={0};
      snprintf(filePath,sizeof(filePath),"data/projects/%s/files/%s",p->id,relativePath);

      if ( AmmServer_WriteFileFromMemory(filePath,(content!=0)?content:"",contentLength) )
      {
        ++p->files[fileIndex].version;
        newVersion = p->files[fileIndex].version;
      }
    }
  }

  snprintf(rqst->content,rqst->MAXcontentSize,"%u %u",newVersion,conflict);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


void * newFile_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64]={0};
  unsigned int authenticated = getAuthenticatedUser(rqst,username,sizeof(username));

  char sessionID[64]={0};
  _POSTcpy(rqst,"s",sessionID,sizeof(sessionID));
  char projectID[32]={0};
  _POSTcpy(rqst,"project",projectID,sizeof(projectID));
  char relativePath[MAX_STRING_SIZE]={0};
  _POSTcpy(rqst,"relativePath",relativePath,sizeof(relativePath));

  struct project * p = authenticated ? findProject(projectID) : 0;

  if ( (p!=0) && userCanAccessProject(p,username) && isSafeRelativePath(relativePath) && isEditableTextFile(relativePath) )
  {
    char filePath[MAX_STRING_SIZE*2]={0};
    snprintf(filePath,sizeof(filePath),"data/projects/%s/files/%s",p->id,relativePath);

    //mkdir any single subdirectory component the relative path implies ( isSafeRelativePath caps nesting to 3 levels )
    char dirPath[MAX_STRING_SIZE*2]={0};
    snprintf(dirPath,sizeof(dirPath),"%s",filePath);
    char * lastSlash = strrchr(dirPath,'/');
    if (lastSlash!=0)
    {
      *lastSlash=0;
      char mkdirCmd[MAX_STRING_SIZE*2+32]={0};
      snprintf(mkdirCmd,sizeof(mkdirCmd),"mkdir -p '%s'",dirPath);
      char scratch[16]={0};
      AmmServer_ExecuteCommandLine(mkdirCmd,scratch,sizeof(scratch));
    }

    if (! AmmServer_FileExists(filePath))
    {
      AmmServer_WriteFileFromMemory(filePath,"",0);
    }
    scanProjectFiles(p);
  }

  snprintf(rqst->content,rqst->MAXcontentSize,
           "<html><head><meta http-equiv=\"refresh\" content=\"0; url=editor.html?s=%s&project=%s&file=%s\"></head><body></body></html>",
           sessionID,projectID,relativePath);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}
