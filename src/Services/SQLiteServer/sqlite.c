#include <sqlite3.h>
#include "sqlite.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int SQL_init(struct SQLiteSession * sqlserver , const char * dbFilename)
{
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

    if (sqlserver->rc != SQLITE_OK ) {

        fprintf(stderr, "SQL error: %s\n", sqlserver->err_msg);

        sqlite3_free(sqlserver->err_msg);
        sqlite3_close(sqlserver->db);

        return 0;
    }

 return 1;
}



int printCars(void *NotUsed, int argc, char **argv,
                    char **azColName)
{
    int i = 0;
    NotUsed = 0;

    for (i = 0; i < argc; i++) {

        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");

    return 0;
}




int SQL_fetchcars(struct SQLiteSession * sqlserver)
{
    char *sqlSelect = "SELECT * FROM Cars";
    sqlserver->rc = sqlite3_exec(sqlserver->db, sqlSelect, printCars, 0, &sqlserver->err_msg);

    if (sqlserver->rc != SQLITE_OK ) {

        fprintf(stderr, "Failed to select data\n");
        fprintf(stderr, "SQL error: %s\n", sqlserver->err_msg);

        sqlite3_free(sqlserver->err_msg);
        sqlite3_close(sqlserver->db);

        return 0;
    }
  return 1;
}
