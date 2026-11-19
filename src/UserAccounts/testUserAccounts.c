//Regression tests for the UserAccounts library , everything happens in a temporary directory ( db/users.db is never touched )
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "userAccounts.h"

int uadb_saveUserAccountDatabase(struct UserAccountDatabase * uadb);
char * uadb_getBackRandomFileDigits(unsigned int numberOfDigits);

static unsigned int failures=0;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr,"FAIL %s:%d : %s\n",__FILE__,__LINE__,#cond); ++failures; } } while(0)

static char tempDir[64];
static char * path(const char * name) { static char p[2][128]; static int n=0; n^=1; snprintf(p[n],sizeof(p[n]),"%s/%s",tempDir,name); return p[n]; }
static void writeFile(const char * name,const char * content) { FILE * fp=fopen(path(name),"w"); fputs(content,fp); fclose(fp); }
static int login(struct UserAccountDatabase * db,const char * user,const char * pass) { struct UserAccountAuthenticationToken t={0}; return uadb_authenticateUser(db,user,pass,&t); }

//Usernames with whitespace shifted every later field on reload , and 32+ characters were stored truncated
static void test_usernames()
{
  struct UserAccountDatabase * db = uadb_initializeUserAccountDatabase(path("names.db"));
  CHECK(db!=0);
  CHECK(!uadb_addUser(db,"bob smith","password1","0","x"));
  CHECK(!uadb_addUser(db,"","password1","0","x"));
  CHECK(!uadb_addUser(db,"tab\there","password1","0","x"));
  CHECK(!uadb_addUser(db,"a_very_long_username_of_forty_characters","password1","0","x"));
  CHECK(!uadb_addUser(db,0,"password1","0","x"));
  CHECK(!uadb_addUser(db,"nopass",0,"0","x"));
  CHECK(uadb_addUser(db,"carol","password2","0","x"));
  CHECK(uadb_addUser(db,"exactly_31_characters_long_name","password3","0","x"));
  CHECK(!uadb_addUser(db,"carol","other","0","x")); //Taken
  CHECK(db->userListSize==2);
  CHECK(uadb_closeUserAccountDatabase(&db));
  CHECK(db==0);

  db = uadb_initializeUserAccountDatabase(path("names.db"));
  CHECK( (db!=0) && (db->userListSize==2) );
  CHECK(login(db,"carol","password2"));
  CHECK(login(db,"exactly_31_characters_long_name","password3"));
  CHECK(!login(db,"carol","wrong"));
  CHECK(!login(db,"nobody","password2"));
  CHECK(!login(db,0,"password2"));
  CHECK(!login(db,"carol",0));
  uadb_closeUserAccountDatabase(&db);
}

//authenticateUser printed the caller's token before filling it , with an uninitialized token that is a wild pointer
static void test_uninitializedToken()
{
  struct UserAccountDatabase * db = uadb_initializeUserAccountDatabase(path("token.db"));
  CHECK(uadb_addUser(db,"alice","password1","0","x"));
  struct UserAccountAuthenticationToken token;
  memset(&token,0x41,sizeof(token));
  CHECK(uadb_authenticateUser(db,"alice","password1",&token));
  CHECK( (token.username!=0) && (strcmp(token.username,"alice")==0) && (token.uid==0) );
  uadb_closeUserAccountDatabase(&db);
}

static void test_lookups()
{
  struct UserAccountDatabase * db = uadb_initializeUserAccountDatabase(path("lookup.db"));
  CHECK(uadb_addUser(db,"alice","password1","0","x"));
  CHECK(uadb_addUser(db,"bob","password2","0","x"));
  struct UserAccountAuthenticationToken token={0};
  CHECK(uadb_getUserTokenFromUserID(db,&token,1) && (strcmp(token.username,"bob")==0));
  CHECK(!uadb_getUserTokenFromUserID(db,&token,2));   //Was not bounds checked
  CHECK(!uadb_getUserTokenFromUserID(db,&token,500));

  UserAccount_UserID uid=99;
  CHECK(uadb_getUserIDFromSessionID(db,db->userList[1].sessionID,&uid) && (uid==1));
  CHECK(!uadb_getUserIDFromSessionID(db,"",&uid));
  CHECK(!uadb_getUserIDFromSessionID(db,0,&uid));
  CHECK(!uadb_getUserIDFromSessionID(db,"no-such-session",&uid));
  CHECK(uadb_getUserTokenFromSessionID(db,db->userList[0].sessionID,&token) && (strcmp(token.username,"alice")==0));
  uadb_closeUserAccountDatabase(&db);
}

//Corrupt files used to be parsed with unbounded %s and an unchecked count ( 4294967295 overflowed the heap )
static void test_corruptFiles()
{
  writeFile("count.db","4294967295\nalice\nh\ns\n");
  CHECK(uadb_initializeUserAccountDatabase(path("count.db"))==0);
  writeFile("negative.db","-5\nalice\nh\ns\n");
  CHECK(uadb_initializeUserAccountDatabase(path("negative.db"))==0);
  writeFile("garbage.db","not a number\n");
  CHECK(uadb_initializeUserAccountDatabase(path("garbage.db"))==0);

  char longField[400]; memset(longField,'x',300); longField[300]=0;
  char content[512];
  snprintf(content,sizeof(content),"1\n%s\nh\ns\n",longField);
  writeFile("longname.db",content);
  CHECK(uadb_initializeUserAccountDatabase(path("longname.db"))==0); //Refused , not loaded as an empty database
  snprintf(content,sizeof(content),"1\nalice\nh\n%s\n",longField);
  writeFile("longsession.db",content);
  CHECK(uadb_initializeUserAccountDatabase(path("longsession.db"))==0);
  writeFile("half.db","1\nalice\nh\n");
  CHECK(uadb_initializeUserAccountDatabase(path("half.db"))==0);

  writeFile("short.db","3\nalice\nh1\ns1\nbob\nh2\ns2\n"); //Lists 3 , holds 2 : load what is there
  struct UserAccountDatabase * db = uadb_initializeUserAccountDatabase(path("short.db"));
  CHECK( (db!=0) && (db->userListSize==2) && (strcmp(db->userList[1].username,"bob")==0) );
  if (db) { free(db->userList); free(db); }

  db = uadb_initializeUserAccountDatabase(path("does_not_exist.db")); //A missing file is a fresh , empty database
  CHECK( (db!=0) && (db->userListSize==0) );
  if (db) { free(db->userList); free(db); }
  CHECK(uadb_initializeUserAccountDatabase(0)==0);
}

//Saving rewrote the file in place ( a crash lost every account ) and left it world-readable
static void test_saving()
{
  umask(022);
  struct UserAccountDatabase * db = uadb_initializeUserAccountDatabase(path("save.db"));
  CHECK(uadb_addUser(db,"alice","password1","0","x"));
  struct stat st;
  CHECK( (stat(path("save.db"),&st)==0) && ((st.st_mode&0777)==0600) );
  CHECK(access(path("save.db.tmp"),F_OK)!=0);
  CHECK(!uadb_saveUserAccountDatabase(0));

  //A save that can't happen must not report success , and memory must keep matching the file
  CHECK(mkdir(path("ro"),0700)==0);
  struct UserAccountDatabase * ro = uadb_initializeUserAccountDatabase(path("ro/users.db"));
  CHECK(uadb_addUser(ro,"alice","password1","0","x"));
  CHECK(chmod(path("ro"),0500)==0);
  if (access(path("ro/probe"),F_OK)!=0) //Skip the rest when running as root , which ignores the permission
  {
    CHECK(!uadb_addUser(ro,"bob","password2","0","x"));
    CHECK(ro->userListSize==1);
    CHECK(!uadb_setPassword(ro,"alice","newpassword"));
    CHECK(login(ro,"alice","password1"));
    CHECK(!login(ro,"alice","newpassword"));
  }
  chmod(path("ro"),0700);
  CHECK(uadb_setPassword(ro,"alice","newpassword"));
  CHECK(login(ro,"alice","newpassword"));
  CHECK(!uadb_setPassword(ro,"nobody","newpassword"));
  uadb_closeUserAccountDatabase(&ro);
  CHECK(!uadb_closeUserAccountDatabase(&ro)); //Already closed
  uadb_closeUserAccountDatabase(&db);
}

static void test_randomDigits()
{
  char * digits = uadb_getBackRandomFileDigits(5000);
  CHECK(digits!=0);
  if (digits==0) { return; }
  CHECK(strlen(digits)==5000);
  CHECK(strchr(digits,'z')!=0); //'z' could never come up ( off by one )
  CHECK(strspn(digits,"abcdefghijklmnopqrstuvwxyz")==5000);
  free(digits);
}

int main()
{
  strcpy(tempDir,"/tmp/useraccountstestXXXXXX");
  if (mkdtemp(tempDir)==0) { perror("mkdtemp"); return 2; }

  test_usernames();
  test_uninitializedToken();
  test_lookups();
  test_corruptFiles();
  test_saving();
  test_randomDigits();

  char command[160];
  snprintf(command,sizeof(command),"rm -rf '%s'",tempDir);
  if (system(command)!=0) { fprintf(stderr,"could not remove %s\n",tempDir); }

  if (failures) { fprintf(stderr,"UserAccounts tests : %u failures\n",failures); return 1; }
  fprintf(stderr,"UserAccounts tests : all passed\n");
  return 0;
}
