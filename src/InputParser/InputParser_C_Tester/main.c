#include <stdio.h>
#include <stdlib.h>
#include "../InputParser_C.h"
#include <time.h>


#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */

#define max_ret_word 256

void ParseString(struct InputParserC * ipc,char * thestr);
int IntermediateTests();



int main()
{
    //InputParserC is a library to tokenize words from a string "zero,one,two,three(four)" -> "zero" "one" "two" "three" "four"..!
    printf("Hello World , InputParserC linked version is %s \n",InputParserC_Version());

    //To use it you must first allocate a InputParserC structure!
     struct InputParserC * ipc=0;
    //After you declared the structure you will have to initialize it to accomodate the data you want to input
    //The next command initializes our input parser to accomodate a max of 512 different words , which result from 5 seperating delimeters
    //In the example "zero,one,two,three(four)" the delimeters used are  , ( ) and \0
     ipc = InputParser_Create(512,5);


    // The default delimiters are..
    printf("The default delimiters are.. ");
    int i=0;
    for ( i = 0; i< ipc->max_delimeter_count; i++ )
    {
      printf("%u ascii or   %c  ",InputParser_GetDelimeter(ipc,i),InputParser_GetDelimeter(ipc,i));
    }

     //We will now tokenize the string "zero,one,two,three(four)" , we set the last parameter to zero to indicate that
     //input parser should keep a copy of the string , because after the call is done it will be disposed of..!
     //The return value ( res ) is the number of tokens extracted!
     int res = InputParser_SeperateWords(ipc,"zero,one,2,three(four)",1);
     printf("\nInput Parser tokenized a total of %u tokens \n",res);

     //To retrieve the tokens as a string you will need some memory to store the strings in!
     //The following word_space has space for a string with up to 128 characters
     char word_space[128]={0};

     //We now want to get the token with number 1 and store it in word_space!
     //Please note that the tokens are counted from 0 to n-1 where n is the total number of tokens..
    /*
       "zero,one,two,three(four)"
          0   1   2    3     4    = a total of n=5 tokens !
    */

     // The following command will return the second token ( token with number 1 ) and store it to the word_space
     // that contains up to 128 character strings
     InputParser_GetWord(ipc,1,word_space,128);
     printf("InputParser_GetWord returns %s\n",word_space);

     //If you want you can retrieve an upcase/lowercase version of the string
     InputParser_GetUpcaseWord(ipc,1,word_space,128);
     printf("InputParser_GetUpcaseWord returns %s\n",word_space);

     //If the token is a number you can return an integer! :)
     i = InputParser_GetWordInt(ipc,2);
     printf("InputParser_GetWordInt(ipc,2) should return 2 , our build returns %u\n",i);

     // After you are finished with the InputParser , you should deallocate it to free memory..!
     InputParser_Destroy(ipc);

     printf("\n\nThat concludes the rationale tutorial \n Press any key to start XP bug check.. \n\n");
     getchar();
    // Now run some tests to find bugs , if they return 0 SeperateWordsC should be working ok!
  return IntermediateTests();

}


/*
    >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    XP / BUG FINDING TESTS!
    >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/


void ParseString(struct InputParserC * ipc,char * thestr)
{
    float fl;
    int i,z;
    char word_space[max_ret_word]={0};


    printf(MAGENTA "\n\nParsing a string..  ( %s ) \n" NORMAL,thestr);
    time_t msec = time(NULL) * 1000;
    int res = InputParser_SeperateWords(ipc,thestr,0);
    time_t msec2 = time(NULL) * 1000;
    printf("Words Seperated ( %u ) at %u msec.. ",res,(unsigned int) (msec2-msec));

    printf("Int Check ");
    for (i=0; i<res; i++) {  z=InputParser_GetWordInt(ipc,i); printf(" %u = %u ",i,z); } printf("\n\n");


    printf("Float Check ");
    for (i=0; i<res; i++) {  fl=InputParser_GetWordFloat(ipc,i); printf(" %u = %05f ",i,fl); } printf("\n\n");

    printf("Length Check ");
    for (i=0; i<res; i++) { printf(" %u = %u ",i,InputParser_GetWordLength(ipc,i)); } printf("\n\n");


    printf("String Check ");
    for (i=0; i<res; i++) { InputParser_GetWord(ipc,i,word_space,max_ret_word);
                           printf(" %u = %s ",i,word_space); } printf("\n\n");

    printf("Upcase String Check ");
    for (i=0; i<res; i++) { InputParser_GetUpcaseWord(ipc,i,word_space,max_ret_word);
                           printf(" %u = %s ",i,word_space); } printf("\n\n");

    printf("Lowercase String Check ");
    for (i=0; i<res; i++) { InputParser_GetLowercaseWord(ipc,i,word_space,max_ret_word);
                           printf(" %u = %s ",i,word_space); } printf("\n\n");


    printf("First Erroneous String Check ");
    strcpy(word_space,"");
    for (i=res; i<res+2; i++) { InputParser_GetWord(ipc,i,word_space,max_ret_word);
                                printf(" %u = %s ",i,word_space); } printf("\n\n");

    printf("Second Erroneous String Check ");
    char * empty_place_to_pass_string = 0;
    for (i=res; i<res+2; i++) { InputParser_GetWord(ipc,i,empty_place_to_pass_string,0);
                                printf(" %u = %s ",i,word_space); } printf("\n\n");

    printf("Third Erroneous String Check ");
    strcpy(word_space,"");
    for (i=res; i<res+2; i++) { InputParser_GetWord(ipc,i,word_space,max_ret_word);
                                printf(" %u = %s ",i,word_space); } printf("\n\n");

    char check_test[max_ret_word]={0};
    printf("Make a bogus WordCompare check ");
    for (i=0; i<res; i++) { InputParser_GetLowercaseWord(ipc,i,check_test,max_ret_word);
                            if ( InputParser_WordCompareNoCase(ipc,i,check_test,strlen(check_test)) ) { printf(" (%u) success " , i ); } else
                                                                                                {
                                                                                                  printf(" Error at string with number %u and content %s \n" , i ,  check_test);
                                                                                                }

                          } printf("\n\n");

    unsigned int retries=0 , max_retries=10000 , errors = 0;
    printf("Bashing WordCompare checks ");
        for (i=0; i<res; i++)
                          {
                             for ( retries=0; retries<max_retries; retries++ )
                              {
                                InputParser_GetLowercaseWord(ipc,i,check_test,max_ret_word);
                                if ( InputParser_WordCompareNoCase(ipc,i,check_test,strlen(check_test)) ) {  } else
                                                                                                          { ++errors; }
                                InputParser_GetWord(ipc,i,check_test,max_ret_word);
                                if ( InputParser_WordCompare(ipc,i,check_test,strlen(check_test)) ) {  } else  {   }

                              }

                          }
}



int IntermediateTests()
{
   time_t startmsec = time(NULL) * 1000;

    printf("Testing InputParserC ");
    struct InputParserC * ipc=0;
    printf(", linked version is %s \n",InputParserC_Version());


    printf("Creating a new instance of the parser\n");

    printf("Starting memory point of IPC struct is %p , and afterwards",ipc);
    ipc = InputParser_Create(256,5);
    if ( ipc == 0 ) { fprintf(stderr,"\nError Commiting InputParser Context \n"); return(1); }
    printf(" %p \n",ipc);


    struct InputParserC * ipc2=0;
    printf("Starting memory point of second IPC struct is %p , and afterwards",ipc2);
    ipc2 = InputParser_Create(256,5);
    if ( ipc2 == 0 ) { fprintf(stderr,"\nError Commiting InputParser Context \n"); return(1); }
    printf(" %p \n",ipc2);


    printf("SelfChecking\n");
    if ( InputParser_SelfCheck(ipc) == 0 ) { fprintf(stderr,"\nInputParser Self Check Returns erroneous state \n"); return(1); }
     printf("SelfChecking2\n");
    if ( InputParser_SelfCheck(ipc2) == 0 ) { fprintf(stderr,"\nInputParser2 Self Check Returns erroneous state \n"); return(1); }


    printf("Checking if default delimiters work correctly\n");
    InputParser_DefaultDelimeters(ipc);


    char parsemessage[] = "0,1,2,3,4,5,,6,7,8,9,10\0";
    ParseString(ipc,parsemessage);


    char parsemessage2[] = "miden,ena,dyo,tria,tessera,pente,eksi,epta,okto,ennea,deka\0";
    ParseString(ipc,parsemessage2);
    if (InputParser_WordCompare(ipc,0,"miden",5)!=1) { fprintf(stderr,"\n\n\n!!!!!!!!!!!!!!!!!! CATCHED COMPARISON ERROR !!!!!!!!!!!!!!!!!!\n\n\n");} else
                                                     { fprintf(stderr,"Comparison check ok..\n");}
    if (InputParser_WordCompare(ipc,1,"ena",3)!=1) { fprintf(stderr,"\n\n\n!!!!!!!!!!!!!!!!!! CATCHED COMPARISON ERROR !!!!!!!!!!!!!!!!!!\n\n\n");} else
                                                   { fprintf(stderr,"Comparison check ok..\n");}
    if (InputParser_WordCompare(ipc,2,"dy",2)!=0) { fprintf(stderr,"\n\n\n!!!!!!!!!!!!!!!!!! CATCHED COMPARISON ERROR !!!!!!!!!!!!!!!!!!\n\n\n");} else
                                                  { fprintf(stderr,"Comparison check ok..\n");}

    char parsemessage3[] = "010\0";
    ParseString(ipc,parsemessage3);

    char parsemessage4[] = "\0";
    ParseString(ipc2,parsemessage4);


    char parsemessage5[] = ",,,,(),\0";
    ParseString(ipc,parsemessage5);

    char parsemessage6[] = "FORWARD(45)\0";
    ParseString(ipc,parsemessage6);


    char parsemessage7[] = "0,1,2,-45,-0.0,1,1000000000000000\0";
    ParseString(ipc2,parsemessage7);


    char parsemessage8[] = "DEPTH MAP,(192.168.1.1)\n\0";
    ParseString(ipc,parsemessage8);


    char parsemessage9[] = "POS(hand,1000,   2.0,1.0,-3.0 , 0.0,0.0,0.0,-0.0314169\0";
    ParseString(ipc,parsemessage9);

    fprintf(stderr,"This line should crash to remind us of the correct way to declare a string..\n");
    char * parsemessage10 = "POS(hand,1000,   -2.0,1.0,3.0 , 0.0,0.0,0.0,-0.0314169\0";
    ParseString(ipc,parsemessage10);

    char parsemessage11[] = "3.141567\n\0";
    ParseString(ipc,parsemessage11);

    char parsemessage12[] = "3.141567,3.141567\n,3.141567\0";
    ParseString(ipc,parsemessage12);


    // MINI DATE TEST
        struct InputParserC * ipc3=0;
        ipc3 = InputParser_Create(50,2);
        InputParser_SetDelimeter(ipc3,1,'/');
        int res = InputParser_SeperateWords(ipc3,"0/0/0",0);
         if ( res >= 3 )
         {
           int year=InputParser_GetWordInt(ipc3,2);
           int month=InputParser_GetWordInt(ipc3,1);
           int day=InputParser_GetWordInt(ipc3,0);
           year = month + day;
         }
         InputParser_Destroy(ipc3);
    // MINI DATE TEST


    printf("Deleting the new instance of the parser\n");
    InputParser_Destroy(ipc);
    InputParser_Destroy(ipc2);
    printf("InputParser Instances destructed.. \n");

    time_t endmsec = time(NULL) * 1000;
    printf("\n%u total msec.. \n",(unsigned int) (endmsec-startmsec));

    printf(GREEN "Passed..\n" NORMAL);

    return 0;
}
