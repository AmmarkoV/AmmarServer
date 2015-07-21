#ifndef SQLITE_H_INCLUDED
#define SQLITE_H_INCLUDED


struct SQLiteSession
{
 sqlite3 *db;
 sqlite3_stmt *res;
 char *err_msg;

 int rc;
};

int SQL_init(struct SQLiteSession * sqlserver , const char * dbFilename);
int SQL_close(struct SQLiteSession * sqlserver);
int SQL_getVersion(struct SQLiteSession * sqlserver);
int SQL_populate(struct SQLiteSession * sqlserver );
int SQL_fetchcars(struct SQLiteSession * sqlserver);

#endif // SQLITE_H_INCLUDED
