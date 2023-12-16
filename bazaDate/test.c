#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
int insertData(sqlite3 *db, char *name1, char *name2) {
    char *errMsg = 0;
    int code;
    char sql_insert[100];
    sprintf(sql_insert, "INSERT INTO Blocked (Nume1, Nume2) VALUES ('%s', '%s');", name1, name2);
    code = sqlite3_exec(db, sql_insert, 0, 0, &errMsg);
    if (code != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        return 0;
    }
    printf("Succes la introducere date in bd!\n");
    return 1;
}

int main() {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    rc = sqlite3_open("main.db", &db);

    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return(0);
    }
    else fprintf(stdout, "Opened database successfully\n");

    char *sql_check = "SELECT name FROM sqlite_master WHERE type='table' AND name='Blocked';";
    rc = sqlite3_exec(db, sql_check, 0, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return(0);
    }

    if (sqlite3_changes(db) > 0) {
        fprintf(stdout, "Table 'Blocked' already exists. Skipping creation.\n");
    } else {
        char *sql_create = "CREATE TABLE Blocked (Nume1 TEXT, Nume2 TEXT);";
        rc = sqlite3_exec(db, sql_create, 0, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        } 
        else fprintf(stdout, "Table 'Blocked' created successfully\n");
    }

    insertData(db, "John", "Doe");
    insertData(db, "Alice", "Smith");
    sqlite3_close(db);
    return 0;
}
