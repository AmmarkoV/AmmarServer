#include <stdio.h>     /* files */
#include <stdlib.h>     /* qsort */
#include <string.h>     /* memset */
#include <unistd.h>     /* unlink , close */

#include "csvparse.h"

static unsigned int failures=0;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr,"FAIL %s:%d : %s\n",__FILE__,__LINE__,#cond); ++failures; } } while(0)

static int fieldIs(struct CSVParser * csv,unsigned int field,const char * expected)
{
  char * f = csvParser_GetField(csv,field);
  if (f==0)                   { fprintf(stderr,"  field %u is null , expected `%s`\n",field,expected); return 0; }
  if (strcmp(f,expected)!=0)  { fprintf(stderr,"  field %u is `%s` , expected `%s`\n",field,f,expected); return 0; }
  return 1;
}

//Writes content ( length given so it can hold any byte ) to a fresh temp file , path goes to filename
static void writeTempFile(char * filename,const char * content,size_t length)
{
  strcpy(filename,"/tmp/csvparsetestXXXXXX");
  int fd = mkstemp(filename);
  if (fd<0) { perror("mkstemp"); exit(2); }
  FILE * fp = fdopen(fd,"wb");
  if (fp==0) { perror("fdopen"); exit(2); }
  fwrite(content,1,length,fp);
  fclose(fp);
}
#define TEMPFILE(name,literal) char name[64]; writeTempFile(name,literal,sizeof(literal)-1)

static struct CSVParser * openCSV(const char * filename,const char * delimiters)
{
  struct CSVParser * csv = csvParserCreate(delimiters,strlen(delimiters));
  CHECK(csv!=0);
  CHECK(csvParser_StartParsingFile(csv,filename));
  return csv;
}

//The original test.csv walk
static void test_basic()
{
  TEMPFILE(f,"FieldA;Field B;Field C\n1;2.3;3.0000000000001\n2;3.4;0\n");
  const char * expected[3][3] = { {"FieldA","Field B","Field C"} , {"1","2.3","3.0000000000001"} , {"2","3.4","0"} };
  struct CSVParser * csv = openCSV(f,";");
  unsigned int line=0,i;
  while (csvParser_ParseNextLine(csv))
  {
    CHECK(line<3);
    if (line>=3) { break; }
    CHECK(csvParser_GetNumberOfFields(csv)==3);
    for (i=0; i<csvParser_GetNumberOfFields(csv); i++) { CHECK(fieldIs(csv,i,expected[line][i])); }
    ++line;
  }
  CHECK(line==3);
  CHECK(csv->linesParsed==3);
  csvParser_StopParsingFile(csv);
  csvParserDestroy(csv); //Used to be a double free , StopParsingFile freed the parser
  unlink(f);
}

//The NUL written into the previous line used to be "restored" into the next line's buffer
static void test_stateDoesNotLeakAcrossLines()
{
  TEMPFILE(f,"a;b;c\nxxxxxxxx;y\n");
  struct CSVParser * csv = openCSV(f,";");
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(fieldIs(csv,2,"c"));
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(fieldIs(csv,0,"xxxxxxxx"));
  CHECK(fieldIs(csv,1,"y"));
  CHECK(csvParser_GetNumberOfFields(csv)==2);
  csvParserDestroy(csv);
  unlink(f);

  //Same , but the second line is long enough to make getline() reallocate the buffer ( was a use-after-free )
  char longContent[4096];
  strcpy(longContent,"a;b;c\n");
  size_t l = strlen(longContent);
  memset(longContent+l,'z',3000); l+=3000;
  strcpy(longContent+l,";tail\n");
  char f2[64]; writeTempFile(f2,longContent,strlen(longContent));
  csv = openCSV(f2,";");
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(fieldIs(csv,2,"c"));
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(fieldIs(csv,1,"tail"));
  CHECK(strlen(csvParser_GetField(csv,0))==3000);
  csvParserDestroy(csv);
  unlink(f2);
}

//The field count used to be cached from the first line , and asking for a missing field dereferenced 0
static void test_fieldCountPerLine()
{
  TEMPFILE(f,"a;b;c\nd;e\nf;g;h;i\n");
  struct CSVParser * csv = openCSV(f,";");
  CHECK(csvParser_ParseNextLine(csv)); CHECK(csvParser_GetNumberOfFields(csv)==3);
  CHECK(csvParser_ParseNextLine(csv)); CHECK(csvParser_GetNumberOfFields(csv)==2);
  CHECK(csvParser_GetField(csv,2)==0);
  CHECK(csvParser_GetField(csv,100)==0);
  CHECK(fieldIs(csv,1,"e")); //Still fine after an out of range request
  CHECK(csvParser_ParseNextLine(csv)); CHECK(csvParser_GetNumberOfFields(csv)==4);
  CHECK(fieldIs(csv,3,"i"));
  CHECK(!csvParser_ParseNextLine(csv));
  csvParserDestroy(csv);
  unlink(f);
}

//The last field of a line without a trailing newline used to dereference 0
static void test_noTrailingNewline()
{
  TEMPFILE(f,"a;b;c\nlast;field");
  struct CSVParser * csv = openCSV(f,";");
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(csvParser_GetNumberOfFields(csv)==2);
  CHECK(fieldIs(csv,0,"last"));
  CHECK(fieldIs(csv,1,"field"));
  csvParserDestroy(csv);
  unlink(f);
}

//CRLF lines used to report an extra empty field
static void test_crlf()
{
  TEMPFILE(f,"a;b\r\nc;d\r\n");
  struct CSVParser * csv = openCSV(f,";");
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(csvParser_GetNumberOfFields(csv)==2);
  CHECK(fieldIs(csv,1,"b"));
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(fieldIs(csv,0,"c"));
  CHECK(fieldIs(csv,1,"d"));
  csvParserDestroy(csv);
  unlink(f);
}

static void test_emptyFieldsAndLines()
{
  TEMPFILE(f,";;\n\na;;b\n");
  struct CSVParser * csv = openCSV(f,";");
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(csvParser_GetNumberOfFields(csv)==3);
  CHECK(fieldIs(csv,0,"")); CHECK(fieldIs(csv,1,"")); CHECK(fieldIs(csv,2,""));
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(csvParser_GetNumberOfFields(csv)==1);
  CHECK(fieldIs(csv,0,""));
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(csvParser_GetNumberOfFields(csv)==3);
  CHECK(fieldIs(csv,0,"a")); CHECK(fieldIs(csv,1,"")); CHECK(fieldIs(csv,2,"b"));
  csvParserDestroy(csv);
  unlink(f);
}

static void test_fieldAccessPatterns()
{
  TEMPFILE(f,"one|two,three|four\n");
  struct CSVParser * csv = openCSV(f,"|,");
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(csvParser_GetNumberOfFields(csv)==4);
  CHECK(fieldIs(csv,0,"one"));
  CHECK(fieldIs(csv,0,"one")); //Cached path
  CHECK(fieldIs(csv,3,"four"));
  CHECK(fieldIs(csv,1,"two"));
  CHECK(fieldIs(csv,2,"three"));
  CHECK(fieldIs(csv,0,"one"));
  CHECK(fieldIs(csv,3,"four"));
  csvParserDestroy(csv);
  unlink(f);
}

//Used a char for fgetc() : a 0xFF byte looked like EOF ( and EOF was never seen where char is unsigned )
static void test_countNumberOfLines()
{
  TEMPFILE(ff,"x\n\xff" "after\n");
  TEMPFILE(nonl,"a\nb");
  TEMPFILE(empty,"");
  CHECK(csvParser_CountNumberOfLines(0,ff)==2);
  CHECK(csvParser_CountNumberOfLines(0,nonl)==2);
  CHECK(csvParser_CountNumberOfLines(0,empty)==0);
  CHECK(csvParser_CountNumberOfLines(0,"/nonexistent/csvparse/file")==0);
  CHECK(csvParser_CountNumberOfLines(0,0)==0);
  unlink(ff); unlink(nonl); unlink(empty);
}

static void test_lifecycle()
{
  TEMPFILE(f,"a;b\nc;d\n");
  struct CSVParser * csv = openCSV(f,";");
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(fieldIs(csv,1,"b"));
  CHECK(csvParser_StartParsingFile(csv,f)); //Restart : must not leak the old handle , must start from the top
  CHECK(csv->linesParsed==0);
  CHECK(csvParser_ParseNextLine(csv));
  CHECK(fieldIs(csv,0,"a"));
  csvParser_StopParsingFile(csv);
  CHECK(!csvParser_ParseNextLine(csv));
  CHECK(csvParser_GetField(csv,0)==0);
  CHECK(csvParser_GetNumberOfFields(csv)==0);
  csvParser_StopParsingFile(csv); //Stopping twice is harmless
  csvParserDestroy(csv);

  csv = openCSV(f,";");
  CHECK(csvParser_ParseNextLine(csv));
  csvParserDestroy(csv); //Without StopParsingFile : must not leak ( LeakSanitizer checks this )

  csv = csvParserCreate(";",1);
  CHECK(!csvParser_StartParsingFile(csv,"/nonexistent/csvparse/file"));
  CHECK(!csvParser_StartParsingFile(csv,0));
  CHECK(!csvParser_ParseNextLine(csv));
  csvParserDestroy(csv);
  unlink(f);
}

static void test_nullSafety()
{
  CHECK(csvParserCreate(0,1)==0);
  CHECK(csvParser_GetField(0,0)==0);
  CHECK(!csvParser_ParseNextLine(0));
  CHECK(csvParser_GetNumberOfFields(0)==0);
  CHECK(!csvParser_StartParsingFile(0,"x"));
  CHECK(!csvParser_StartParsingFile(0,0));
  csvParser_StopParsingFile(0);
  csvParserDestroy(0);
}

int main()
{
  test_basic();
  test_stateDoesNotLeakAcrossLines();
  test_fieldCountPerLine();
  test_noTrailingNewline();
  test_crlf();
  test_emptyFieldsAndLines();
  test_fieldAccessPatterns();
  test_countNumberOfLines();
  test_lifecycle();
  test_nullSafety();

  if (failures) { fprintf(stderr,"CSVParser tests : %u failures\n",failures); return 1; }
  fprintf(stderr,"CSVParser tests : all passed\n");
  return 0;
}
