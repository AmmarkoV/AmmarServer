#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

#include "../../AmmServerlib/AmmServerlib.h"

#define MAX_ITEMS 800
#define ID_BUF_SIZE 17          //hex ids are 8 chars , "cover" is 5 , +margin+nul
#define NAME_BUF_SIZE 512       //MAX_NAME_LEN(100) worst case UTF-8 bytes , +margin
#define TITLE_BUF_SIZE 256      //MAX_TITLE_LEN(60)  worst case UTF-8 bytes , +margin
#define TOKEN_BUF_SIZE 48
#define NORM_BUF_SIZE 512

#define MAX_QTY 999
#define MAX_NAME_LEN 100
#define MAX_TITLE_LEN 60

#define DEFAULT_TITLE "Λίστα Σούπερ Μάρκετ"

struct item
{
  char id[ID_BUF_SIZE];
  char name[NAME_BUF_SIZE];
  unsigned int qty;
  unsigned int checked;
};

struct cart
{
  char title[TITLE_BUF_SIZE]; //empty = use DEFAULT_TITLE
  unsigned int rev;
  unsigned int numberOfItems;
  struct item items[MAX_ITEMS];
};

enum cartAction
{
  ACTION_NOOP=0,   //still bumps rev and rewrites the file , does nothing else ( photo upload success , imgdel handled separately )
  ACTION_ADD,
  ACTION_QTY,
  ACTION_TOGGLE,
  ACTION_DEL,
  ACTION_IMGDEL,
  ACTION_NAME
};

struct cartActionParams
{
  enum cartAction action;
  char itemID[ID_BUF_SIZE];
  char text[NAME_BUF_SIZE];  //new item name ( ACTION_ADD ) or new list title ( ACTION_NAME )
  int hasDelta; int delta;   //ACTION_QTY relative change
  int hasValue; int value;   //ACTION_QTY absolute value
};

//Sanitize a cart token to [A-Za-z0-9_-] , max 40 chars , returns 1 if the result is non-empty
int sanitizeToken(const char * in,char * out,unsigned int outSize);

//Sanitize a photo id ( item id , or the literal "cover" ) to [a-z0-9] , max 16 chars
int sanitizeItemID(const char * in,char * out,unsigned int outSize);

//Generate a fresh random lowercase-hex token/id of the given number of hex characters
void generateHexID(char * out,unsigned int hexChars);

//Greeklish-fold a UTF-8 name for case/accent/spelling-insensitive comparisons , see state.c for details
void normGreeklish(const char * in,char * out,unsigned int outSize);

const char * cartsDirectory();
const char * photosDirectory();
void ensureStorageDirectories();

void cartPathForToken(const char * token,char * out,unsigned int outSize);
void photoPathFor(const char * token,const char * photoID,char * out,unsigned int outSize);

//Lock-free cheap probe of just the rev counter , 0 if the cart doesn't exist yet or is unreadable
unsigned int peekCartRev(const char * token);

//Load ( creating + seeding if missing ) , optionally apply a mutation , always writes the file back , exactly like go.php's with_cart()
//params==0 means a plain read ( still writes back e.g. to materialize a brand new seeded cart , but does not bump rev )
int withCart(const char * token,struct cart * outCart,struct cartActionParams * params);

#endif // STATE_H_INCLUDED
