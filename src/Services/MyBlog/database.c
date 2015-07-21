#include "database.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct website myblog={0};
struct SQLiteSession sqlserver={0};


int printCars(void *rqstV, int argc, char **argv, char **azColName)
{
    struct AmmServer_DynamicRequest  * rqst = (struct AmmServer_DynamicRequest  *) rqstV;
    char formatBuffer[512]={0};

    unsigned int i=0;
    for (i = 0; i < argc; i++)
    {
        snprintf(formatBuffer,512,"<tr><td>%s</td><td>%s</td></tr>\n", azColName[i], argv[i] ? argv[i] : "NULL");
        strcat(rqst->content,formatBuffer);
    }

   return 0;
}

int SQL_init(struct SQLiteSession * sqlserver , const char * dbFilename)
{
    if (sqlite3_config(SQLITE_CONFIG_SERIALIZED)!=SQLITE_OK)
    {
     AmmServer_Error("Cannot set SQLite to serialized mode , cannot continue we are not going to be thread safe..!");
     return 0;
    }

    sqlserver->rc = sqlite3_open(dbFilename, &sqlserver->db);
    if (sqlserver->rc != SQLITE_OK)
    {
     fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(sqlserver->db));
     sqlite3_close(sqlserver->db);

     return 0;
    }

 return 1;
}

int SQL_close(struct SQLiteSession * sqlserver)
{
    sqlite3_close(sqlserver->db);
    return 1;
}

int SQL_getVersion(struct SQLiteSession * sqlserver)
{
    sqlserver->rc = sqlite3_prepare_v2(sqlserver->db, "SELECT SQLITE_VERSION()", -1, &sqlserver->res, 0);
    if (sqlserver->rc != SQLITE_OK)
    {
     fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(sqlserver->db));
     sqlite3_close(sqlserver->db);
     return 0;
    }

    sqlserver->rc = sqlite3_step(sqlserver->res);

    if (sqlserver->rc == SQLITE_ROW)
    {
        printf("%s\n", sqlite3_column_text(sqlserver->res, 0));
    }

    sqlite3_finalize(sqlserver->res);

  return 1;
}


int SQL_populate(struct SQLiteSession * sqlserver )
{
    char *sql = "DROP TABLE IF EXISTS Cars;"
                "CREATE TABLE Cars(Id INT, Name TEXT, Price INT);"
                "INSERT INTO Cars VALUES(1, 'Audi', 52642);"
                "INSERT INTO Cars VALUES(2, 'Mercedes', 57127);"
                "INSERT INTO Cars VALUES(3, 'Skoda', 9000);"
                "INSERT INTO Cars VALUES(4, 'Volvo', 29000);"
                "INSERT INTO Cars VALUES(5, 'Bentley', 350000);"
                "INSERT INTO Cars VALUES(6, 'Citroen', 21000);"
                "INSERT INTO Cars VALUES(7, 'Hummer', 41400);"
                "INSERT INTO Cars VALUES(8, 'Volkswagen', 21600);";

    sqlserver->rc = sqlite3_exec(sqlserver->db, sql, 0, 0, &sqlserver->err_msg);

    if (sqlserver->rc != SQLITE_OK )
    {

        fprintf(stderr, "SQL error: %s\n", sqlserver->err_msg);

        sqlite3_free(sqlserver->err_msg);
        sqlite3_close(sqlserver->db);

        return 0;
    }

 return 1;
}


int serveCarsPageWithSQL(struct SQLiteSession * sqlserver , struct AmmServer_DynamicRequest  * rqst)
{
    strncpy(rqst->content,"<html><head><title>Car List</title></head><body><table>",rqst->MAXcontentSize);

    char *sqlSelect = "SELECT * FROM Cars";
    sqlserver->rc = sqlite3_exec(sqlserver->db, sqlSelect, printCars, (void*) rqst, &sqlserver->err_msg);

    strcat(rqst->content,"</table></body></html>");
    rqst->contentSize=strlen(rqst->content);

    if (sqlserver->rc != SQLITE_OK )
    {
     fprintf(stderr, "Failed to select data\n");
     fprintf(stderr, "SQL error: %s\n", sqlserver->err_msg);

     sqlite3_free(sqlserver->err_msg);
     sqlite3_close(sqlserver->db);

     return 0;
    }



  return 1;
}






/*

   struct menuItemList menu;
   struct linkItemList linksLeft;
   struct linkItemList linksRight;
   struct postItemList post;

   struct widgetItemList widget;

struct postItem
{
  unsigned char title[MAX_STR];
  unsigned char dateStr[MAX_STR];
  unsigned char author[MAX_STR];
  struct tagItemList tags;
  struct htmlContent content;
};


*/



int SQL_createInitialTables(struct SQLiteSession * sqlserver )
{
    char *sql = "DROP TABLE IF EXISTS website;"
                "CREATE TABLE website(Id INT,allowComments INT,allowPing INT,blogTitle TEXT,siteName TEXT,siteDescription TEXT,siteURL TEXT);"
                // - - -
                "DROP TABLE IF EXISTS socialLinks;"
                "CREATE TABLE socialLinks(Id INT,label TEXT,url TEXT);"
                // - - -
                "DROP TABLE IF EXISTS linksLeft;"
                "CREATE TABLE linksLeft(Id INT,label TEXT,url TEXT);"
                // - - -
                "DROP TABLE IF EXISTS linksRight;"
                "CREATE TABLE linksRight(Id INT,label TEXT,url TEXT);"
                // - - -
                "DROP TABLE IF EXISTS tags;"
                "CREATE TABLE tags(Id INT,label TEXT,int postID);"
                // - - -
                "DROP TABLE IF EXISTS posts;"
                "CREATE TABLE posts(Id INT,title TEXT,date TEXT,author TEXT,content TEXT);"
                // - - -
                "INSERT INTO socialLinks VALUES(1,'Facebook','http://facebook.com/ammarkov');"
                "INSERT INTO socialLinks VALUES(2,'Twitter','http://twitter.com/ammarkov');"
                "INSERT INTO socialLinks VALUES(3,'Youtube','http://youtube.com/ammarkov');"
                // - - -
                "INSERT INTO website VALUES(1,1,1,'Ammar`s Website','Powered by AmmarServer','Description of Site','http://ammar.gr');" ;

    sqlserver->rc = sqlite3_exec(sqlserver->db, sql, 0, 0, &sqlserver->err_msg);

    if (sqlserver->rc != SQLITE_OK )
    {

        fprintf(stderr, "SQL error: %s\n", sqlserver->err_msg);

        sqlite3_free(sqlserver->err_msg);
        sqlite3_close(sqlserver->db);

        return 0;
    }

 return 1;
}








