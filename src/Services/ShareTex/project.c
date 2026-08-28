#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "project.h"
#include "auth.h"

#define DASHBOARD_BUFFER_CAPACITY 65536

static void appendProjectRow(char * buffer , unsigned int bufferCapacity , struct project * p , const char * username , const char * sessionID)
{
  char escapedTitle[MAX_STRING_SIZE*6]={0};
  AmmServer_HTMLEscape(p->title,escapedTitle,sizeof(escapedTitle));

  char escapedOwner[64*6]={0};
  AmmServer_HTMLEscape(p->owner,escapedOwner,sizeof(escapedOwner));

  char badge[64]={0};
  if (p->isPublic) { snprintf(badge,sizeof(badge),"<span class=\"badge\">shared demo</span>"); } else
  if (strcmp(p->owner,username)==0) { snprintf(badge,sizeof(badge),"<span class=\"badge\">owner</span>"); } else
  { snprintf(badge,sizeof(badge),"<span class=\"badge\">collaborator</span>"); }

  char shareForm[600]={0};
  if ( (!p->isPublic) && (strcmp(p->owner,username)==0) )
  {
    snprintf(shareForm,sizeof(shareForm),
             "<form class=\"shareForm\" method=\"post\" enctype=\"multipart/form-data\" action=\"share.html\" style=\"display:inline\">"
             "<input type=\"hidden\" name=\"s\" value=\"%s\">"
             "<input type=\"hidden\" name=\"project\" value=\"%s\">"
             "<input type=\"text\" name=\"collaborator\" placeholder=\"username to add\" size=\"14\">"
             "<button type=\"submit\">Share</button></form>",
             sessionID,p->id);
  }

  char chunk[2048]={0};
  snprintf(chunk,sizeof(chunk),
           "<div class=\"projectRow\"><div><a class=\"open\" href=\"editor.html?s=%s&project=%s\">%s</a>%s"
           "<div style=\"font-size:12px;color:#888;\">owner: %s</div></div><div>%s</div></div>",
           sessionID,p->id,escapedTitle,badge,escapedOwner,shareForm);

  strncat(buffer,chunk,bufferCapacity - strlen(buffer) - 1);
}


void * dashboard_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64]={0};
  if ( ! getAuthenticatedUser(rqst,username,sizeof(username)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Session expired. <a href=\"index.html\">Log in</a></body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  char sessionID[64]={0};
  _GETcpy(rqst,"s",sessionID,sizeof(sessionID));

  char * buffer = (char*) malloc(DASHBOARD_BUFFER_CAPACITY);
  if (buffer==0) { return 0; }
  buffer[0]=0;

  unsigned int i=0;
  for (i=0; i<numberOfProjects; i++)
  {
    if ( userCanAccessProject(&projects[i],username) )
    {
      appendProjectRow(buffer,DASHBOARD_BUFFER_CAPACITY,&projects[i],username,sessionID);
    }
  }

  char escapedUsername[64*6]={0};
  AmmServer_HTMLEscape(username,escapedUsername,sizeof(escapedUsername));

  snprintf(rqst->content,rqst->MAXcontentSize,
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>ShareTex Dashboard</title>"
    "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\"></head><body>"
    "<div class=\"topbar\"><h1>ShareTex</h1><div class=\"right\">%s &nbsp; <a href=\"index.html\">Log out</a></div></div>"
    "<div class=\"container\">"
    "<div class=\"card\"><h3>New project</h3>"
    "<form class=\"newProjectForm\" method=\"post\" enctype=\"multipart/form-data\" action=\"createProject.html\">"
    "<input type=\"hidden\" name=\"s\" value=\"%s\">"
    "<input type=\"text\" name=\"title\" placeholder=\"Project title\" required>"
    "<button type=\"submit\">Create</button></form></div>"
    "<div class=\"card\"><h3>Your projects</h3>%s</div>"
    "</div></body></html>",
    escapedUsername,sessionID,buffer);
  rqst->contentSize=strlen(rqst->content);

  free(buffer);
  return 0;
}


void * createProject_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64]={0};
  if ( ! getAuthenticatedUser(rqst,username,sizeof(username)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Session expired. <a href=\"index.html\">Log in</a></body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  char sessionID[64]={0};
  _POSTcpy(rqst,"s",sessionID,sizeof(sessionID));

  char title[MAX_STRING_SIZE]={0};
  _POSTcpy(rqst,"title",title,sizeof(title));
  if (strlen(title)==0) { snprintf(title,sizeof(title),"Untitled Document"); }

  char newID[32]={0};
  if ( createProject(username,title,newID,sizeof(newID)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,
             "<html><head><meta http-equiv=\"refresh\" content=\"0; url=editor.html?s=%s&project=%s\"></head>"
             "<body>Creating project , redirecting..</body></html>",
             sessionID,newID);
  } else
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Could not create project. <a href=\"dashboard.html?s=%s\">Back</a></body></html>",sessionID);
  }
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


void * share_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64]={0};
  char sessionID[64]={0};
  _POSTcpy(rqst,"s",sessionID,sizeof(sessionID));

  if ( ! getAuthenticatedUser(rqst,username,sizeof(username)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Session expired. <a href=\"index.html\">Log in</a></body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  char projectID[32]={0};
  _POSTcpy(rqst,"project",projectID,sizeof(projectID));
  char collaborator[64]={0};
  _POSTcpy(rqst,"collaborator",collaborator,sizeof(collaborator));

  struct project * p = findProject(projectID);
  const char * message = "Shared successfully";

  if ( (p==0) || (strcmp(p->owner,username)!=0) )
  {
    message="Only the owner can share this project";
  } else
  if ( ! userAccountExists(collaborator) )
  {
    message="No such user account";
  } else
  if ( p->numberOfCollaborators>=MAX_COLLABORATORS )
  {
    message="This project already has the maximum number of collaborators";
  } else
  {
    unsigned int alreadyThere=0;
    unsigned int i=0;
    for (i=0; i<p->numberOfCollaborators; i++) { if (strcmp(p->collaborators[i],collaborator)==0) { alreadyThere=1; } }
    if (!alreadyThere)
    {
      snprintf(p->collaborators[p->numberOfCollaborators],sizeof(p->collaborators[0]),"%s",collaborator);
      ++p->numberOfCollaborators;
      saveProjectMeta(p);
    }
  }

  snprintf(rqst->content,rqst->MAXcontentSize,
           "<html><head><meta http-equiv=\"refresh\" content=\"1; url=dashboard.html?s=%s\"></head>"
           "<body>%s</body></html>",sessionID,message);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}
