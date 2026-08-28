#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>

#include "state.h"

static unsigned int tokenSeeded=0;

//Starter history for a brand-new list , kept in step with configuration.default.php SEED_STAPLES
//These all begin checked ( "in the basket" ) , so the active list is clean but re-adding a known name is instant
static const char * SEED_STAPLES[]=
{
  "ψωμί","γάλα","αυγά","φέτα","γιαούρτι","βούτυρο","τυρί τοστ","ζαμπόν",
  "κοτόπουλο","κιμάς","ρύζι","μακαρόνια","φακές","φασόλια","αλεύρι","ζάχαρη",
  "αλάτι","πιπέρι","ελαιόλαδο","ξύδι","ντομάτες","πατάτες","κρεμμύδια","σκόρδο",
  "λεμόνια","μπανάνες","μήλα","πορτοκάλια","καρότα","σαλάτα","καφές","τσάι",
  "χυμός","νερό","δημητριακά","μέλι","μαρμελάδα","σοκολάτα","μπισκότα",
  "κατεψυγμένα λαχανικά","χαρτί υγείας","χαρτί κουζίνας","χαρτομάντηλα",
  "οδοντόκρεμα","σαμπουάν","σαπούνι","απορρυπαντικό πλυντηρίου","υγρό πιάτων",
  "χλωρίνη","σακούλες σκουπιδιών","αλουμινόχαρτο","λαδόκολλα"
};
#define SEED_STAPLES_COUNT (sizeof(SEED_STAPLES)/sizeof(SEED_STAPLES[0]))


void ensureStorageDirectories()
{
  mkdir("cartdata",0770);
  mkdir("cartdata/photos",0770);
}

const char * cartsDirectory()   { return "cartdata"; }
const char * photosDirectory()  { return "cartdata/photos"; }

void cartPathForToken(const char * token,char * out,unsigned int outSize)
{
  snprintf(out,outSize,"%s/%s.dat",cartsDirectory(),token);
}

void photoPathFor(const char * token,const char * photoID,char * out,unsigned int outSize)
{
  snprintf(out,outSize,"%s/%s-%s.jpg",photosDirectory(),token,photoID);
}


int sanitizeToken(const char * in,char * out,unsigned int outSize)
{
  if ( (in==0) || (out==0) || (outSize==0) ) { return 0; }
  unsigned int oi=0,ii=0;
  while ( (in[ii]!=0) && (oi+1<outSize) && (oi<40) )
  {
    char c=in[ii];
    if ( isalnum((unsigned char)c) || (c=='_') || (c=='-') ) { out[oi++]=c; }
    ++ii;
  }
  out[oi]=0;
  return (oi>0);
}

int sanitizeItemID(const char * in,char * out,unsigned int outSize)
{
  if ( (in==0) || (out==0) || (outSize==0) ) { return 0; }
  unsigned int oi=0,ii=0;
  while ( (in[ii]!=0) && (oi+1<outSize) && (oi<16) )
  {
    char c=(char) tolower((unsigned char)in[ii]);
    if ( ( (c>='a')&&(c<='z') ) || ( (c>='0')&&(c<='9') ) ) { out[oi++]=c; }
    ++ii;
  }
  out[oi]=0;
  return (oi>0);
}

void generateHexID(char * out,unsigned int hexChars)
{
  static const char hexAlphabet[]="0123456789abcdef";
  if (!tokenSeeded) { srand( (unsigned int) time(0) ^ (unsigned int) getpid() ); tokenSeeded=1; }
  unsigned int i=0;
  for (i=0; i<hexChars; i++) { out[i]=hexAlphabet[rand()%16]; }
  out[hexChars]=0;
}


/* ------------------------------------------------------------------------
   Greeklish folding , kept in step with norm() in go.php / supermarket.py :
   products are compared by how they SOUND rather than how they are spelled,
   so "Ψωμι" , the misspelt "Ψωμή" and a typed "psomi" all fold to the same
   key. All the substitutions below operate directly on UTF-8 BYTE sequences
   ( every Greek letter used here is exactly 2 bytes ) , the same trick the
   PHP/Python originals rely on. Digraphs are listed before the single
   letters they contain, so a pass never re-matches what an earlier rule in
   the same pass already produced ( each pass is single left-to-right scan ,
   first matching rule at a position wins , non-overlapping ).
   ------------------------------------------------------------------------ */
struct byteRule { const char * needle; const char * replacement; };

static void applyByteRules(const char * in,const struct byteRule * rules,unsigned int numRules,char * out,unsigned int outSize)
{
  unsigned int ii=0,oi=0;
  unsigned int inLen=(unsigned int) strlen(in);
  while ( (ii<inLen) && (oi+1<outSize) )
  {
    unsigned int r=0; int matched=0;
    for (r=0; r<numRules; r++)
    {
      unsigned int nlen=(unsigned int) strlen(rules[r].needle);
      if ( (nlen>0) && (ii+nlen<=inLen) && (memcmp(in+ii,rules[r].needle,nlen)==0) )
      {
        unsigned int rlen=(unsigned int) strlen(rules[r].replacement);
        if (oi+rlen>=outSize) { rlen=outSize-oi-1; }
        memcpy(out+oi,rules[r].replacement,rlen);
        oi+=rlen; ii+=nlen; matched=1;
        break;
      }
    }
    if (!matched) { out[oi++]=in[ii++]; }
  }
  out[oi]=0;
}

//Uppercase Greek -> lowercase Greek ( mirrors the mbstring-less polyfill in go.php )
static const struct byteRule GREEK_CASEFOLD[]=
{
  {"Α","α"},{"Β","β"},{"Γ","γ"},{"Δ","δ"},{"Ε","ε"},{"Ζ","ζ"},{"Η","η"},
  {"Θ","θ"},{"Ι","ι"},{"Κ","κ"},{"Λ","λ"},{"Μ","μ"},{"Ν","ν"},{"Ξ","ξ"},{"Ο","ο"},
  {"Π","π"},{"Ρ","ρ"},{"Σ","σ"},{"Τ","τ"},{"Υ","υ"},{"Φ","φ"},{"Χ","χ"},{"Ψ","ψ"},
  {"Ω","ω"},{"Ά","ά"},{"Έ","έ"},{"Ή","ή"},{"Ί","ί"},{"Ό","ό"},{"Ύ","ύ"},{"Ώ","ώ"},
  {"Ϊ","ϊ"},{"Ϋ","ϋ"}
};

//Accents/διαλυτικά/final-sigma off
static const struct byteRule GREEK_STRIP_ACCENTS[]=
{
  {"ά","α"},{"έ","ε"},{"ή","η"},{"ί","ι"},{"ό","ο"},{"ύ","υ"},{"ώ","ω"},
  {"ϊ","ι"},{"ϋ","υ"},{"ΐ","ι"},{"ΰ","υ"},{"ς","σ"}
};

//Greek -> Latin , digraphs first so they claim their letters before the single-letter rules do
static const struct byteRule GREEKLISH[]=
{
  {"ου","u"},{"ει","i"},{"οι","i"},{"υι","i"},{"αι","e"},
  {"α","a"},{"β","v"},{"γ","g"},{"δ","d"},{"ε","e"},{"ζ","z"},
  {"η","i"},{"θ","th"},{"ι","i"},{"κ","k"},{"λ","l"},{"μ","m"},
  {"ν","n"},{"ξ","ks"},{"ο","o"},{"π","p"},{"ρ","r"},{"σ","s"},
  {"τ","t"},{"υ","i"},{"φ","f"},{"χ","x"},{"ψ","ps"},{"ω","o"}
};

void normGreeklish(const char * in,char * out,unsigned int outSize)
{
  if ( (in==0) || (out==0) || (outSize==0) ) { return; }

  char stage[NORM_BUF_SIZE]={0};
  unsigned int i=0,oi=0;
  //trim + ASCII lowercase first ( single-byte pass , safe to do byte-wise )
  unsigned int len=(unsigned int) strlen(in);
  unsigned int start=0,end=len;
  while ( (start<end) && (isspace((unsigned char)in[start])) ) { ++start; }
  while ( (end>start) && (isspace((unsigned char)in[end-1])) ) { --end; }
  for (i=start; (i<end) && (oi+1<sizeof(stage)); i++)
  {
    char c=in[i];
    stage[oi++]=(char) tolower((unsigned char)c);
  }
  stage[oi]=0;

  char stage2[NORM_BUF_SIZE]={0};
  applyByteRules(stage,GREEK_CASEFOLD,sizeof(GREEK_CASEFOLD)/sizeof(GREEK_CASEFOLD[0]),stage2,sizeof(stage2));

  char stage3[NORM_BUF_SIZE]={0};
  applyByteRules(stage2,GREEK_STRIP_ACCENTS,sizeof(GREEK_STRIP_ACCENTS)/sizeof(GREEK_STRIP_ACCENTS[0]),stage3,sizeof(stage3));

  applyByteRules(stage3,GREEKLISH,sizeof(GREEKLISH)/sizeof(GREEKLISH[0]),out,outSize);
}


/* ------------------------------------------------------------------------
   Percent-encoding for the free-text fields ( item names , list title ) kept
   in the on-disk cart file , so a name containing spaces/commas/newlines can
   never corrupt the simple whitespace-delimited line format.
   ------------------------------------------------------------------------ */
static void percentEncode(const char * in,char * out,unsigned int outSize)
{
  static const char hexDigits[]="0123456789ABCDEF";
  unsigned int oi=0,ii=0;
  unsigned char c;
  if (outSize==0) { return; }
  while ( (in[ii]!=0) && (oi+1<outSize) )
  {
    c=(unsigned char) in[ii];
    if ( isalnum(c) || (c=='-') || (c=='_') || (c=='.') || (c=='~') )
    {
      out[oi++]=(char)c;
    } else
    {
      if (oi+3>=outSize) { break; }
      out[oi++]='%';
      out[oi++]=hexDigits[(c>>4)&0xF];
      out[oi++]=hexDigits[c&0xF];
    }
    ++ii;
  }
  out[oi]=0;
}

static int hexVal(char c)
{
  if ( (c>='0')&&(c<='9') ) { return c-'0'; }
  if ( (c>='a')&&(c<='f') ) { return c-'a'+10; }
  if ( (c>='A')&&(c<='F') ) { return c-'A'+10; }
  return -1;
}

static void percentDecode(const char * in,char * out,unsigned int outSize)
{
  unsigned int oi=0,ii=0;
  if (outSize==0) { return; }
  while ( (in[ii]!=0) && (oi+1<outSize) )
  {
    if ( (in[ii]=='%') && (in[ii+1]!=0) && (in[ii+2]!=0) )
    {
      int hi=hexVal(in[ii+1]),lo=hexVal(in[ii+2]);
      if ( (hi>=0) && (lo>=0) ) { out[oi++]=(char)((hi<<4)|lo); ii+=3; continue; }
    }
    out[oi++]=in[ii++];
  }
  out[oi]=0;
}


static void seedCart(struct cart * cart)
{
  unsigned int i=0;
  memset(cart,0,sizeof(struct cart));
  for (i=0; (i<SEED_STAPLES_COUNT) && (i<MAX_ITEMS); i++)
  {
    struct item * it=&cart->items[i];
    generateHexID(it->id,8);
    snprintf(it->name,sizeof(it->name),"%s",SEED_STAPLES[i]);
    it->qty=1;
    it->checked=1;
  }
  cart->numberOfItems=i;
  cart->rev=0;
}


//Returns 1 if the stream looked like one of our cart files ( found a REV line ) , 0 if empty/corrupt/brand-new
static int parseCartStream(FILE * fp,struct cart * cart)
{
  memset(cart,0,sizeof(struct cart));
  char line[2048];
  int sawHeader=0;

  while ( fgets(line,sizeof(line),fp)!=0 )
  {
    size_t l=strlen(line);
    while ( (l>0) && ( (line[l-1]=='\n') || (line[l-1]=='\r') ) ) { line[--l]=0; }
    if (l==0) { continue; }

    if (strncmp(line,"REV ",4)==0)
    {
      sawHeader=1;
      cart->rev=(unsigned int) strtoul(line+4,0,10);
    } else
    if (strncmp(line,"NAME ",5)==0)
    {
      sawHeader=1;
      percentDecode(line+5,cart->title,sizeof(cart->title));
      if (strcmp(cart->title,"-")==0) { cart->title[0]=0; }
    } else
    if (strncmp(line,"ITEM ",5)==0)
    {
      sawHeader=1;
      if (cart->numberOfItems>=MAX_ITEMS) { continue; }
      char idBuf[ID_BUF_SIZE]={0};
      char qtyBuf[16]={0};
      char checkedBuf[4]={0};
      char nameBuf[1536]={0};
      if ( sscanf(line+5,"%16s %15s %3s %1535s",idBuf,qtyBuf,checkedBuf,nameBuf)==4 )
      {
        struct item * it=&cart->items[cart->numberOfItems];
        snprintf(it->id,sizeof(it->id),"%s",idBuf);
        it->qty=(unsigned int) strtoul(qtyBuf,0,10);
        it->checked=(strtoul(checkedBuf,0,10)!=0);
        percentDecode(nameBuf,it->name,sizeof(it->name));
        ++cart->numberOfItems;
      }
    }
  }

  return sawHeader;
}

static void serializeCartStream(FILE * fp,struct cart * cart)
{
  char encTitle[TITLE_BUF_SIZE*3]={0};
  if (cart->title[0]!=0) { percentEncode(cart->title,encTitle,sizeof(encTitle)); }
  else { snprintf(encTitle,sizeof(encTitle),"-"); }

  fprintf(fp,"REV %u\n",cart->rev);
  fprintf(fp,"NAME %s\n",encTitle);

  unsigned int i=0;
  for (i=0; i<cart->numberOfItems; i++)
  {
    struct item * it=&cart->items[i];
    char encName[NAME_BUF_SIZE*3]={0};
    percentEncode(it->name,encName,sizeof(encName));
    fprintf(fp,"ITEM %s %u %u %s\n",it->id,it->qty,it->checked?1:0,encName);
  }
}


unsigned int peekCartRev(const char * token)
{
  char path[512]={0};
  cartPathForToken(token,path,sizeof(path));
  FILE * fp=fopen(path,"r");
  if (fp==0) { return 0; }
  struct cart cart;
  parseCartStream(fp,&cart);
  fclose(fp);
  return cart.rev;
}


static int findItemIndex(struct cart * cart,const char * id)
{
  unsigned int i=0;
  for (i=0; i<cart->numberOfItems; i++)
  {
    if (strcmp(cart->items[i].id,id)==0) { return (int) i; }
  }
  return -1;
}

static void removeItemAt(struct cart * cart,unsigned int index)
{
  if (index>=cart->numberOfItems) { return; }
  unsigned int remaining=cart->numberOfItems-index-1;
  if (remaining>0) { memmove(&cart->items[index],&cart->items[index+1],sizeof(struct item)*remaining); }
  --cart->numberOfItems;
}

static void applyAction(const char * token,struct cart * cart,struct cartActionParams * params)
{
  switch (params->action)
  {
    case ACTION_ADD:
    {
      char trimmed[NAME_BUF_SIZE]={0};
      unsigned int s=0,e=(unsigned int) strlen(params->text);
      while ( (s<e) && isspace((unsigned char)params->text[s]) ) { ++s; }
      while ( (e>s) && isspace((unsigned char)params->text[e-1]) ) { --e; }
      unsigned int L=e-s; if (L>=sizeof(trimmed)) { L=sizeof(trimmed)-1; }
      memcpy(trimmed,params->text+s,L); trimmed[L]=0;

      if ( (trimmed[0]==0) || (strlen(trimmed)>MAX_NAME_LEN*4) ) { break; }

      char newKey[NORM_BUF_SIZE]={0};
      normGreeklish(trimmed,newKey,sizeof(newKey));

      unsigned int i=0;
      for (i=0; i<cart->numberOfItems; i++)
      {
        char key[NORM_BUF_SIZE]={0};
        normGreeklish(cart->items[i].name,key,sizeof(key));
        if (strcmp(key,newKey)==0) { cart->items[i].checked=0; return; }
      }

      if (cart->numberOfItems>=MAX_ITEMS) { break; }
      memmove(&cart->items[1],&cart->items[0],sizeof(struct item)*cart->numberOfItems);
      generateHexID(cart->items[0].id,8);
      snprintf(cart->items[0].name,sizeof(cart->items[0].name),"%s",trimmed);
      cart->items[0].qty=1;
      cart->items[0].checked=0;
      ++cart->numberOfItems;
      break;
    }

    case ACTION_QTY:
    {
      int idx=findItemIndex(cart,params->itemID);
      if (idx<0) { break; }
      struct item * it=&cart->items[idx];
      int newQty=(int) it->qty;
      if (params->hasDelta) { newQty=(int)it->qty+params->delta; }
      else if (params->hasValue) { newQty=params->value; }
      if (newQty<1) { newQty=1; }
      if (newQty>MAX_QTY) { newQty=MAX_QTY; }
      it->qty=(unsigned int) newQty;
      break;
    }

    case ACTION_TOGGLE:
    {
      int idx=findItemIndex(cart,params->itemID);
      if (idx<0) { break; }
      cart->items[idx].checked=cart->items[idx].checked?0:1;
      break;
    }

    case ACTION_DEL:
    {
      int idx=findItemIndex(cart,params->itemID);
      if (idx<0) { break; }
      char photoPath[512]={0};
      photoPathFor(token,cart->items[idx].id,photoPath,sizeof(photoPath));
      unlink(photoPath);
      removeItemAt(cart,(unsigned int) idx);
      break;
    }

    case ACTION_IMGDEL:
    {
      char photoPath[512]={0};
      photoPathFor(token,params->itemID,photoPath,sizeof(photoPath));
      unlink(photoPath);
      break;
    }

    case ACTION_NAME:
    {
      char trimmed[TITLE_BUF_SIZE]={0};
      unsigned int s=0,e=(unsigned int) strlen(params->text);
      while ( (s<e) && isspace((unsigned char)params->text[s]) ) { ++s; }
      while ( (e>s) && isspace((unsigned char)params->text[e-1]) ) { --e; }
      unsigned int L=e-s; if (L>=sizeof(trimmed)) { L=sizeof(trimmed)-1; }
      memcpy(trimmed,params->text+s,L); trimmed[L]=0;

      if ( (trimmed[0]==0) || (strlen(trimmed)<=MAX_TITLE_LEN*4) )
      {
        snprintf(cart->title,sizeof(cart->title),"%s",trimmed);
      }
      break;
    }

    case ACTION_NOOP:
    default:
      break;
  }
}


int withCart(const char * token,struct cart * outCart,struct cartActionParams * params)
{
  if ( (token==0) || (token[0]==0) || (outCart==0) ) { return 0; }

  ensureStorageDirectories();

  char path[512]={0};
  cartPathForToken(token,path,sizeof(path));

  int fd=open(path,O_CREAT|O_RDWR,0660);
  if (fd<0) { return 0; }
  flock(fd,LOCK_EX);

  FILE * fp=fdopen(fd,"r+");
  if (fp==0) { close(fd); return 0; }

  struct cart cart;
  if ( ! parseCartStream(fp,&cart) ) { seedCart(&cart); }

  if (params!=0)
  {
    applyAction(token,&cart,params);
    ++cart.rev;
  }

  rewind(fp);
  if (ftruncate(fd,0)!=0) { /*best effort , fall through and overwrite what fits*/ }
  serializeCartStream(fp,&cart);
  fflush(fp);

  flock(fd,LOCK_UN);
  fclose(fp); //also closes fd

  *outCart=cart;
  return 1;
}
