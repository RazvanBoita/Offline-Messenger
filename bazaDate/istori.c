#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

// Functie pentru a afisa mesajele de eroare SQLite
void handleSQLiteError(sqlite3* db, int rc) {
    fprintf(stderr, "SQLite error: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(rc);
}

// Functie pentru a executa comenzi SQL
void executeSQLCommand(sqlite3* db, const char* sql) {
    char* errMsg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        handleSQLiteError(db, rc);
    }
}

// Functie pentru a insera o conversatie in tabel
void insertConversation(sqlite3* db, const char* sender, const char* receiver, const char* message) {
    char sql[256];
    snprintf(sql, sizeof(sql), "INSERT INTO Conversatii (SenderName, ReceiverName, MsgContent) VALUES ('%s', '%s', '%s');",
             sender, receiver, message);
    
    executeSQLCommand(db, sql);
}

// Functie pentru a obtine toate conversatiile dintre doi utilizatori sub forma de string
char* getConversationsBetweenUsers(sqlite3* db, const char* user1, const char* user2) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT * FROM Conversatii WHERE (SenderName = '%s' AND ReceiverName = '%s') OR (SenderName = '%s' AND ReceiverName = '%s') ORDER BY ConversationID;",
             user1, user2, user2, user1);

    sqlite3_stmt* statement;
    int rc = sqlite3_prepare_v2(db, sql, -1, &statement, 0);

    if (rc != SQLITE_OK) {
        handleSQLiteError(db, rc);
    }

    // Buffer pentru stocarea rezultatului sub forma de string
    char* result = malloc(1); // Incepem cu un string gol
    result[0] = '\0';

    while (sqlite3_step(statement) == SQLITE_ROW) {
        const char* sender = sqlite3_column_text(statement, 1);
        const char* receiver = sqlite3_column_text(statement, 2);
        const char* message = sqlite3_column_text(statement, 3);

        // Concatenam detaliile conversatiei la rezultatul sub forma de string
        char temp[256];
        snprintf(temp, sizeof(temp), "%s -> %s: %s\n", sender, receiver, message);

        // Redimensionam bufferul rezultatului si concatenam noua conversatie
        result = realloc(result, strlen(result) + strlen(temp) + 1);
        strcat(result, temp);
    }

    sqlite3_finalize(statement);

    return result;
}

int main() {
    sqlite3* db;
    char* errMsg = 0;
    int code;
    code = sqlite3_open("myDB.db", &db);

    if (code) {
        printf("Nu putem deschide baza de date: %s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    } else {
        printf("Baza de date a fost deschisa!\n");
    }

    char* sql_create = "CREATE TABLE Conversatii (ConversationID INTEGER PRIMARY KEY AUTOINCREMENT, SenderName TEXT, ReceiverName TEXT, MsgContent TEXT, Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    code = sqlite3_exec(db, sql_create, 0, 0, &errMsg);
    if (code != SQLITE_OK) {
        fprintf(stderr, "SQL said: %s\n", errMsg);
        sqlite3_free(errMsg);
        printf("Nu mai cream tabelul Conversatii, acesta exista!\n");
    } else {
        printf("Table 'Blocked' created successfully\n");
    }

    insertConversation(db,"Cristi","Razvan","salll");
    insertConversation(db,"Cristi","Razvan","salll2");
    insertConversation(db,"Razvan","Cristi","salll inapoi");
    // Obtine si afiseaza toate conversatiile dintre doi utilizatori
    const char* user1 = "Cristi";
    const char* user2 = "Razvan";
    char* conversations = getConversationsBetweenUsers(db, user1, user2);
    printf("Conversatii intre %s si %s:\n%s", user1, user2, conversations);

    // Elibereaza memoria alocata pentru rezultatul sub forma de string
    free(conversations);

    // Inchide conexiunea la baza de date
    sqlite3_close(db);

    return 0;
}
