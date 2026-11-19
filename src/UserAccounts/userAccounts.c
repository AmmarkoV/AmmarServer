#include "userAccounts.h"
#include "sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define PASSWORD_HASH_ITERATIONS 10000
#define PASSWORD_SALT_BYTES 16
#define MAX_USERS_IN_FILE 1000000 //Anything above this in the count line means the file is corrupt

//The database stores one whitespace separated field per line , so a username must fit in RegisteredUser.username
//and contain no whitespace or control bytes. A space used to shift every later field on the next load , and a
//name of 32+ characters was stored truncated so its owner could never log in with it.
static int isStorableUsername(const char * username)
{
  if (username==0) { return 0; }
  size_t length = strlen(username);
  if ( (length==0) || (length>=sizeof(((struct RegisteredUser*)0)->username)) ) { return 0; }
  size_t i=0;
  for (i=0; i<length; i++)
  {
    unsigned char c = (unsigned char) username[i];
    if ( (c<=' ') || (c==0x7f) ) { return 0; }
  }
  return 1;
}

//Reads the next whitespace separated field into out : 1=read , 0=end of file , -1=does not fit ( a corrupt file )
static int readField(FILE * fp,char * out,size_t outSize)
{
  char field[1024];
  if (fscanf(fp,"%1023s",field)!=1) { return 0; }
  if (strlen(field)>=outSize) { return -1; }
  snprintf(out,outSize,"%s",field);
  return 1;
}

//Reads `len` cryptographically random bytes via getrandom() ( kernel CSPRNG ) , retrying on EINTR , falling back
//to /dev/urandom only if getrandom() itself is unavailable/fails outright. Deliberately self-contained here
//rather than reusing AmmServerlib's AmmServer_GenerateSecureToken() ( tools/http_tools.c ) - this library
//( userAccounts.c specifically, as opposed to userAccountsWeb.c ) has no dependency on AmmServerlib today and
//this keeps it that way.
static int getRandomBytes(unsigned char * out,unsigned int len)
{
  unsigned int got=0;
  while (got<len)
  {
    ssize_t r = getrandom(out+got,len-got,0);
    if (r>0)                       { got+=(unsigned int)r; continue; }
    if ( (r<0) && (errno==EINTR) ) { continue; }
    break;
  }

  if (got<len)
  {
    FILE * fUrandom = fopen("/dev/urandom","rb");
    if (fUrandom==0) { return 0; }
    while (got<len)
    {
      size_t r = fread(out+got,1,len-got,fUrandom);
      if (r==0) { break; }
      got+=(unsigned int)r;
    }
    fclose(fUrandom);
  }

  return (got==len);
}

//Stores "<saltHex>:<hashHex>" - salt is random per user, hash is SHA-256 of (salt||password), then re-applied to
//its own output PASSWORD_HASH_ITERATIONS-1 more times ( simple manual stretching - slows brute force down
//somewhat without pulling in a bcrypt/Argon2/scrypt dependency ). Deliberately modest - good enough for a
//personal/hobby deployment, not state-of-the-art - upgrade this if that ever changes.
static void hashPassword(const char * password,const unsigned char * salt,unsigned int saltLen,char * outStorageString,unsigned int outStorageStringSize)
{
  unsigned char digest[SHA256_DIGEST_SIZE];
  unsigned int passwordLen = strlen(password);

  //salt || password , bounded - a password longer than this is truncated for hashing purposes only ( still
  //hashed/compared consistently both times, so this doesn't break correctness - it just caps how much of an
  //extremely long password actually contributes entropy )
  unsigned char mixed[PASSWORD_SALT_BYTES+256];
  unsigned int mixedPasswordLen = (passwordLen<256) ? passwordLen : 256;
  memcpy(mixed,salt,saltLen);
  memcpy(mixed+saltLen,password,mixedPasswordLen);
  SHA256_Hash(mixed,saltLen+mixedPasswordLen,digest);

  unsigned int i=0;
  for (i=1; i<PASSWORD_HASH_ITERATIONS; i++) { SHA256_Hash(digest,SHA256_DIGEST_SIZE,digest); }

  char saltHex[PASSWORD_SALT_BYTES*2+1];
  char hashHex[SHA256_DIGEST_SIZE*2+1];
  SHA256_ToHex(salt,saltLen,saltHex,sizeof(saltHex));
  SHA256_ToHex(digest,SHA256_DIGEST_SIZE,hashHex,sizeof(hashHex));
  snprintf(outStorageString,outStorageStringSize,"%s:%s",saltHex,hashHex);
}

//1=matches , 0=doesn't ( including a stored value that isn't in this salt:hash format at all - e.g. leftover
//plaintext from before this change, which can no longer authenticate and needs the account re-created )
static int verifyPassword(const char * password,const char * storedValue)
{
  const char * colon = strchr(storedValue,':');
  if (colon==0) { return 0; }

  unsigned int saltHexLen = (unsigned int)(colon-storedValue);
  if (saltHexLen != PASSWORD_SALT_BYTES*2) { return 0; }

  unsigned char salt[PASSWORD_SALT_BYTES];
  unsigned int i=0;
  for (i=0; i<PASSWORD_SALT_BYTES; i++)
  {
    unsigned int byteVal=0;
    if (sscanf(storedValue+i*2,"%2x",&byteVal)!=1) { return 0; }
    salt[i]=(unsigned char)byteVal;
  }

  char recomputed[PASSWORD_SALT_BYTES*2+1+SHA256_DIGEST_SIZE*2+1];
  hashPassword(password,salt,PASSWORD_SALT_BYTES,recomputed,sizeof(recomputed));

  //Constant-time compare on the final hash comparison - cheap, closes a minor timing side-channel
  unsigned int len1=strlen(recomputed), len2=strlen(storedValue);
  if (len1!=len2) { return 0; }
  unsigned char diff=0;
  for (i=0; i<len1; i++) { diff |= (unsigned char)(recomputed[i]^storedValue[i]); }
  return (diff==0);
}

struct UserAccountDatabase * uadb_initializeUserAccountDatabase(const char * filename)
{
 if (filename==0) { return 0; }
 FILE * pFile;
 pFile = fopen (filename,"rb");
 if (pFile!=0)
    {
     //A file that can't be parsed is refused ( returns 0 ) rather than loaded as empty , an empty database would be
     //saved over it later and every account would be lost. Fields used to be read with unbounded %s and no checks.
     unsigned int count=0;
     if ( (fscanf(pFile,"%u",&count)!=1) || (count>MAX_USERS_IN_FILE) )
       { fprintf(stderr,"User database %s has a corrupt user count , not loading it\n",filename); fclose(pFile); return 0; }

     struct UserAccountDatabase * uadb = (struct UserAccountDatabase *) calloc(1,sizeof(struct UserAccountDatabase));
     if (uadb==0) { fclose(pFile); return 0; }
     snprintf(uadb->filename,512,"%s",filename);
     uadb->userListMaxSize=count+100;
     uadb->userList = (struct RegisteredUser *) calloc(uadb->userListMaxSize,sizeof(struct RegisteredUser));
     if (uadb->userList==0) { free(uadb); fclose(pFile); return 0; }

     unsigned int i=0;
     for (i=0; i<count; i++)
     {
       struct RegisteredUser * user = &uadb->userList[i];
       int gotName = readField(pFile,user->username,sizeof(user->username));
       if (gotName==0) { break; } //The file ends early , keep the users read so far
       if ( (gotName<0) || (readField(pFile,user->password,sizeof(user->password))!=1) || (readField(pFile,user->sessionID,sizeof(user->sessionID))!=1) )
       {
         fprintf(stderr,"User database %s is corrupt at user %u , not loading it\n",filename,i);
         free(uadb->userList); free(uadb); fclose(pFile); return 0;
       }
       fprintf(stderr,"User %u : %s\n",i,user->username); //The session ID is a login credential , it is not logged any more
     }
     uadb->userListSize=i;
     if (i<count) { fprintf(stderr,"User database %s lists %u users but only has %u\n",filename,count,i); }
     fclose (pFile);

     fprintf(stderr,"Sucessfully initialized from user list %s with %u users\n",filename,uadb->userListSize);

     return uadb;
    }

 //No database file yet ( first run ) , start with an empty , writable one instead of failing outright..!
 struct UserAccountDatabase * uadb = (struct UserAccountDatabase *) calloc(1,sizeof(struct UserAccountDatabase));
 if (uadb==0) { return 0; }
 snprintf(uadb->filename,512,"%s",filename);
 uadb->userListSize=0;
 uadb->userListMaxSize=100;
 uadb->userList = (struct RegisteredUser *) malloc(sizeof(struct RegisteredUser) * uadb->userListMaxSize);
 if (uadb->userList==0) { free(uadb); return 0; }
 fprintf(stderr,"No existing user database at %s , starting with an empty one\n",filename);
 return uadb;
};


//Writes "<file>.tmp" and renames it over the database , so a crash mid-write can no longer leave a truncated
//database ( the old in-place "wb" lost every account ). The file holds password hashes and session IDs ( which log
//a user in ) , so it is created 0600 instead of world-readable.
int uadb_saveUserAccountDatabase(struct UserAccountDatabase * uadb)
{
 if (uadb==0) { return 0; }
 char temporary[sizeof(uadb->filename)+8];
 snprintf(temporary,sizeof(temporary),"%s.tmp",uadb->filename);

 int fd = open(temporary,O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,0600);
 if (fd<0) { return 0; }
 FILE * pFile = fdopen(fd,"wb");
 if (pFile==0) { close(fd); unlink(temporary); return 0; }

 fprintf(pFile,"%u\n",uadb->userListSize);
 unsigned int i=0;
 for (i=0; i<uadb->userListSize; i++)
 {
   fprintf(pFile,"%s\n",uadb->userList[i].username);
   fprintf(pFile,"%s\n",uadb->userList[i].password);
   fprintf(pFile,"%s\n",uadb->userList[i].sessionID);
 }

 int failed = (fflush(pFile)!=0) || (ferror(pFile)) || (fsync(fileno(pFile))!=0);
 if (fclose(pFile)!=0) { failed=1; }
 if ( (failed) || (rename(temporary,uadb->filename)!=0) ) { unlink(temporary); return 0; }
 return 1;
}

//Saves , frees and sets *uadb to 0 , 1=saved
int uadb_closeUserAccountDatabase(struct UserAccountDatabase **  uadb)
{
  if ( (uadb==0) || (*uadb==0) ) { return 0; }
  int saved = uadb_saveUserAccountDatabase(*uadb);
  free((*uadb)->userList);
  free(*uadb);
  *uadb=0;
  return saved;
};


int uadb_authenticateUser(
                           struct UserAccountDatabase *  uadb,
                           const char * username ,
                           const char * password ,
                           struct UserAccountAuthenticationToken * outputToken
                         )
{
 if (uadb==0)
 {
   fprintf(stderr,"Cannot authenticate user without an initialized database\n");
   return 0;
 }
 if (outputToken==0)
 {
   fprintf(stderr,"Cannot return values without a UserAccountAuthenticationToken\n");
   return 0;
 }
 if ( (username==0) || (password==0) ) { return 0; }
 unsigned int i=0;

 for (i=0; i<uadb->userListSize; i++)
 {
   if (strcmp(uadb->userList[i].username,username)==0)
   {
    if (verifyPassword(password,uadb->userList[i].password))
    {
       outputToken->username =uadb->userList[i].username;
       outputToken->password =uadb->userList[i].password;
       outputToken->sessionID=uadb->userList[i].sessionID;
       outputToken->uid      =i;
       fprintf(stderr,"Found Account %s\n",outputToken->username); //Used to print the caller's token before filling it in
       return 1;
    }
   }
 }

 return 0;
}


int uadb_getUserTokenFromUserID(
                                 struct UserAccountDatabase *  uadb,
                                 struct UserAccountAuthenticationToken * outputToken ,
                                 UserAccount_UserID userID
                              )
{
 if (uadb==0)        {  fprintf(stderr,"Cannot authenticate user without an initialized database\n");      return 0; }
 if (outputToken==0) {  fprintf(stderr,"Cannot return values without a UserAccountAuthenticationToken\n"); return 0; }
 if (userID>=uadb->userListSize) { return 0; } //Was not bounds checked
  outputToken->username =uadb->userList[userID].username;
  outputToken->password =uadb->userList[userID].password;
  outputToken->sessionID=uadb->userList[userID].sessionID;
  outputToken->uid      =userID;
 return 1;
}

int uadb_getUserIDFromSessionID(
                                    struct UserAccountDatabase *  uadb,
                                    const char * sessionID,
                                    UserAccount_UserID *userID
                               )
{
 if ( (uadb==0) || (sessionID==0) || (sessionID[0]==0) || (userID==0) ) { return 0; }
 unsigned int i=0;

 for (i=0; i<uadb->userListSize; i++)
 {
   if (strcmp(uadb->userList[i].sessionID,sessionID)==0)
   {
       *userID=i;
       return 1;
   }
 }
 return 0; //notFound
}

int uadb_getUserTokenFromSessionID(
                                   struct UserAccountDatabase *  uadb,
                                   const char * sessionID,
                                   struct UserAccountAuthenticationToken * outputToken
                                 )
{
 if (uadb==0) { return 0; }
 UserAccount_UserID userID=0;

 if (
     uadb_getUserIDFromSessionID(
                                 uadb ,
                                 sessionID,
                                 &userID
                                )
     )
     {
       if (
           uadb_getUserTokenFromUserID(
                                       uadb,
                                       outputToken ,
                                       userID
                                      )
           )
           {
             return 1;
           }
     }
 return 0; //notFound
}




int uadb_loginUser(
                   struct UserAccountDatabase *  uadb,
                   struct UserAccountAuthenticationToken * outputToken,
                   const char * username,
                   const char * password,
                   UserAccount_PasswordEncoding encoding,
                   const char * ip,
                   const char * browserFingerprint
                   )
{
 //Check UserAccount_PasswordEncoding here

 if (
 uadb_authenticateUser(
                       uadb,
                       username ,
                       password ,
                       outputToken
                      )
     )
     {
       return 1;
     }
  //DO LOG HERE..!
 return 0;
}


int uadb_getBackRandomFileDigitsInplace(char * str , unsigned int numberOfDigits)
{
 unsigned int i=0,range=0;
 for (i=0; i<numberOfDigits; i++)
 {
   range='z'-'a'+1; //Without the +1 'z' could never come up
   str[i]='a'+rand()%range;
 }
str[numberOfDigits]=0;
return 1;
}

char * uadb_getBackRandomFileDigits(unsigned int numberOfDigits)
{
 char * response= (char *) malloc(sizeof(char)* (numberOfDigits+1));
 if (response==0) { return 0; }
 uadb_getBackRandomFileDigitsInplace(response,numberOfDigits);
 return response;
}




int uadb_addUser(
                   struct UserAccountDatabase *  uadb,
                   const char * username,
                   const char * password,
                   const char * ip,
                   const char * browserFingerprint
                 )
{
 //This used to only fprintf-append the new user's lines to the file , without ever touching uadb->userList[] /
 //uadb->userListSize , and without updating the header count line either. That meant a freshly signed up user
 //could not log in until the process restarted , and even then a later uadb_saveUserAccountDatabase() call
 //( e.g. on shutdown ) would rewrite the file using the stale in-memory count and silently drop the new account.
 //It also always `return 0;` , reporting failure even when nothing went wrong. Fixed to actually register the
 //user in memory and persist through the existing save path , which keeps the file's header count correct.
 if (uadb==0) { return 0; }
 if (password==0) { return 0; }
 if (!isStorableUsername(username)) { fprintf(stderr,"Usernames must be 1-31 characters without spaces or control characters\n"); return 0; }

 unsigned int i=0;
 for (i=0; i<uadb->userListSize; i++)
 {
   if (strcmp(uadb->userList[i].username,username)==0) { return 0; } //Username already taken
 }

 if (uadb->userListSize>=uadb->userListMaxSize)
 {
   unsigned int newMaxSize=uadb->userListMaxSize+100;
   struct RegisteredUser * grown=(struct RegisteredUser*) realloc(uadb->userList,sizeof(struct RegisteredUser)*newMaxSize);
   if (grown==0) { return 0; }
   uadb->userList=grown;
   uadb->userListMaxSize=newMaxSize;
 }

 unsigned int newID=uadb->userListSize;
 snprintf(uadb->userList[newID].username,32,"%s",username);

 unsigned char salt[PASSWORD_SALT_BYTES];
 if (!getRandomBytes(salt,sizeof(salt)))
 {
   fprintf(stderr,"Could not obtain secure randomness for a new account's password salt - refusing to create it\n");
   return 0;
 }
 hashPassword(password,salt,sizeof(salt),uadb->userList[newID].password,sizeof(uadb->userList[newID].password));

 //sessionID is char[32] , so it only has room for 31 characters plus the NUL terminator. Hex-encoded CSPRNG bytes
 //( 15 bytes -> 30 hex chars ) preferred over the legacy unseeded-rand() uadb_getBackRandomFileDigitsInplace() -
 //falls back to it only if the CSPRNG is genuinely unavailable, same as the password salt above would refuse
 //outright, but a session ID isn't worth failing account creation over.
 unsigned char sessionSeed[15];
 if (getRandomBytes(sessionSeed,sizeof(sessionSeed)))
      { SHA256_ToHex(sessionSeed,sizeof(sessionSeed),uadb->userList[newID].sessionID,sizeof(uadb->userList[newID].sessionID)); }
 else { uadb_getBackRandomFileDigitsInplace(uadb->userList[newID].sessionID,31); }

 ++uadb->userListSize;

 //Persist immediately so a crash right after signup doesn't lose the account , and report a failed save instead of
 //claiming success for an account that would vanish on restart
 if (!uadb_saveUserAccountDatabase(uadb)) { --uadb->userListSize; return 0; }

 return 1;
}


int uadb_setPassword(
                     struct UserAccountDatabase *  uadb,
                     const char * username,
                     const char * password
                    )
{
 if ( (uadb==0) || (username==0) || (password==0) ) { return 0; }

 unsigned int i=0;
 for (i=0; i<uadb->userListSize; i++)
 {
   if (strcmp(uadb->userList[i].username,username)==0)
   {
     unsigned char salt[PASSWORD_SALT_BYTES];
     if (!getRandomBytes(salt,sizeof(salt)))
     {
       fprintf(stderr,"Could not obtain secure randomness for a new password salt - leaving the password unchanged\n");
       return 0;
     }
     char previous[sizeof(uadb->userList[i].password)];
     memcpy(previous,uadb->userList[i].password,sizeof(previous));
     hashPassword(password,salt,sizeof(salt),uadb->userList[i].password,sizeof(uadb->userList[i].password));
     if (!uadb_saveUserAccountDatabase(uadb))
     { //Keep memory and file in agreement , the old password still works
       memcpy(uadb->userList[i].password,previous,sizeof(previous));
       return 0;
     }
     return 1;
   }
 }
 return 0;
}
