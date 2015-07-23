#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

struct SQLiteSession
{
 sqlite3 *db;
 sqlite3_stmt *res;
 char *err_msg;

 int rc;
};


struct SQLiteSession sqlserver={0};



int SQL_error(struct SQLiteSession * sqlserver,int rc,const char * msg , unsigned int line)
{
    if (rc != SQLITE_OK )
    {
        fprintf(stderr, "Error encountered while running SQL %s : %u\n",msg , line);
        fprintf(stderr, "SQL error: %s\n", sqlserver->err_msg);

        sqlite3_free(sqlserver->err_msg);
        sqlite3_close(sqlserver->db);
        return 1;
    }
  return 0;
}


int SQL_init(struct SQLiteSession * sqlserver , const char * dbFilename)
{
    if (sqlite3_config(SQLITE_CONFIG_SERIALIZED)!=SQLITE_OK)
    {
     fprintf(stderr,"Cannot set SQLite to serialized mode , cannot continue we are not going to be thread safe..!");
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





/*
       =======================================================================================
       =======================================================================================
       =======================================================================================
       =======================================================================================
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
                "DROP TABLE IF EXISTS menu;"
                "CREATE TABLE menu(Id INT,label TEXT,url TEXT);"
                // - - -
                "DROP TABLE IF EXISTS widgets;"
                "CREATE TABLE widgets(Id INT,label TEXT,url TEXT,data TEXT);"
                // - - -
                "DROP TABLE IF EXISTS posts;"
                "CREATE TABLE posts(Id INT,title TEXT,date TEXT,author TEXT,content TEXT);"
                // - - -
                "INSERT INTO linksLeft VALUES(1,'Best Links in the world','bestlinks.html');"
                "INSERT INTO linksLeft VALUES(2,'ELLAK Planet','http://planet.ellak.gr/');"
                "INSERT INTO linksLeft VALUES(3,'FOSS AUEB','http://foss.aueb.gr/');"
                // - - -
                "INSERT INTO linksRight VALUES(1,'Free Software Foundation','http://www.fsf.org/');"
                "INSERT INTO linksRight VALUES(2,'Guarddog project blog','+++++++++WEBROOT+++++++++gddg.html');"
                // - - -
                "INSERT INTO socialLinks VALUES(1,'Facebook','http://facebook.com/ammarkov');"
                "INSERT INTO socialLinks VALUES(2,'Twitter','http://twitter.com/ammarkov');"
                "INSERT INTO socialLinks VALUES(3,'Youtube','http://youtube.com/ammarkov');"
                // - - -
                "INSERT INTO menu VALUES(1,'About','menu0.html');"
                "INSERT INTO menu VALUES(2,'Linux Coding','menu1.html');"
                "INSERT INTO menu VALUES(3,'Windows Coding','menu2.html');"
                "INSERT INTO menu VALUES(4,'GuarddoG Robot Project','menu3.html');"
                "INSERT INTO menu VALUES(5,'DeviantArt Gallery','menu4.html');"
                // - - -
                "INSERT INTO website VALUES(1,1,1,'Ammar`s Website','Powered by AmmarServer','Description of Site','http://ammar.gr');" ;


    sqlserver->rc = sqlite3_exec(sqlserver->db, sql, 0, 0, &sqlserver->err_msg);

    if ( SQL_error(sqlserver,sqlserver->rc, __FILE__, __LINE__) )
    {
        return 0;
    }


 return 1;
}


int main(int argc, char *argv[])
{



    printf("Hello world!\n");
    return 0;
}





