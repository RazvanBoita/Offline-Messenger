#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

#define PORT 2728
#define MAX_USERS 1024
#define MAX_ONLINE_USERS 200
#define SIGNUP_START_INDEX 8
#define LOGIN_START_INDEX 7
#define STATUS_START_INDEX 8
#define POKE_START_INDEX 6
#define UNBLOCK_START_INDEX 9
#define HISTORY_START_INDEX 9
#define DATABASE "toti_userii.txt"
extern int errno;

char* mesaj_clean_screen="\033[2J\033[HPentru meniu, scrieti /menu!\n";
char* mesaj_afisare_meniu="\033[2J\033[HIata meniul nostru!\n\n/menu --> Pentru afisarea optiunilor disponibile\n/signup {NUME} --> Pentru inregistrare(creare cont)\n/login {NUME} --> Pentru autentificare\n/send {NUME} {MESAJ} --> Pentru a trimite un mesaj\n/reply {MESAJ} -->Pentru a raspunde la mesaj\n/history {NUME} -->Pentru a vedea istoricul conversatiei intre 2 useri\n/whoson --> Pentru a vedea cine este conectat\n/whosoff --> Pentru a vedea cine este offline\n/whoall --> Pentru a vedea toti utilizatorii\n/unread --> Pentru a vedea mesajele primite cat timp ati fost offline\n/status {NUME} --> Pentru a vedea daca utilizatorul cu acel {NUME} este online\n/poke {NUME} --> Atrage atentia utilizatorului {NUME} ciupindu-l!\n/block {NUME} --> Blocati toate mesajele ale utilizatorului {NUME}\n/unblock {NUME} --> Deblocati user-ul cu numele {NUME}\n/disconnect --> Deconectati-va de pe chat\n/quit --> Parasiti aplicatia\n/clear --> Curatati ecranul\n\n";
char* mesaj_unkown="Actiune invalida! Pentru mai multe detalii, consultati meniul(/menu)!\n";
char* mesaj_goodbye="La revedere!\n";
char* mesaj_alreadyLogged="Nu putem face acest lucru, deoarece sunteti deja logat!\n";
char* mesaj_errorSignup="Acest user nu este autentificat!\n";
char* mesaj_userExists="Acest user deja exista! Incearca alt nume, sau alege sa te loghezi!\n";
char* mesaj_noPrivilege="Trebuie sa fii logat pentru a putea efectua aceasta operatie!\n";
char* mesaj_disconnectedFailure="Inca nu sunteti conectat, deci nu va putem deconecta...*_*\n";
char* mesaj_invalid_username="Username-ul dvs nu poate contine spatii sau caractere speciale! Incercati din nou!\n";
char* mesaj_no_user="Acest user nu exista!\n";

struct User {
    int socket;
    char name[50];
};
struct User online_users [MAX_ONLINE_USERS]={0,"\0"};
int online_counter=0;

void launchTextDB() {
    FILE *file;
    file = fopen("toti_userii.txt", "a+");
    if (file == NULL) {
        printf("Nu s a deschis fisierul\n");
        return;
    }
    fclose(file);
}

int fileExists(const char *pathname) {
    // Check if the file exists
    if (access(pathname, F_OK) != -1)
        return 1;
    return 0;
}

int citire_date_din_fisier(char pathname[], char dest[]) {
    FILE *file = fopen(pathname, "r");
    if (file == NULL) {
        fprintf(stderr, "Nu s a deschis fisierul '%s'\n", pathname);
        strcpy(dest,"Nu aveti mesaje necitite!\n");
        return 0;
    }
    int destIndex = 0;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        size_t rowLength = strlen(buffer);
        strncpy(dest + destIndex, buffer, rowLength);
        destIndex += rowLength;
    }
    destIndex++;
    dest[destIndex]='\n';
    fclose(file);
    return 1;
}

void eliminare_fisier(char* pathname){

}

char* special_characters="!@#$^&*(),.?";
int verify_username(char username[]){
    int has_space=0, has_special=0;
    for(int i=0;i<strlen(username);i++){
        if(username[i]==' ') has_space=1;
        if(strchr(special_characters,username[i])!=NULL) has_special=1;
        if(has_space+has_special!=0) return 0;
    }
    return 1;
}

int wants_operation(char *input,char* prefix) {
    
    if (strncmp(input, prefix, strlen(prefix)) == 0) {
        char *nameStart = input + strlen(prefix);
        if (*nameStart != '\0') return 1;
    }
    return 0;
}

int wants_send(char *input) {
    if (strncmp(input, "/send", 5) != 0)
        return 0;
    char *space1 = strchr(input + 5, ' ');
    if (space1 == NULL)
        return 0;
    char *space2 = strchr(space1 + 1, ' ');
    if (space2 == NULL)
        return 0;
    int nameLength = space2 - (space1 + 1);
    int msgLength = strlen(space2 + 1);
    if (nameLength == 0 || msgLength == 0)
        return 0;
    return 1;
}

//? e ok
void retrieve_data_fromSend(char buf[], char username[], char msg[]){
    int counter_username=0,counter_msg=0,i;
    for(i=6;i<strlen(buf)&&buf[i]!=' ';i++)
        username[counter_username++]=buf[i];
    int j;
    for (j=i+1;j<strlen(buf)&&buf[i]!='\n'&&buf[i]!='\0';j++)
        msg[counter_msg++]=buf[j];
    msg[strlen(msg)]='\0';
}


void remove_endline_from_buffer(char buffer[1024]){
    char good_buffer[1024]="";
    for(int i=0;i<strlen(buffer);i++){
        if(buffer[i]!='\n') good_buffer[i]=buffer[i];
        else break;
    }
    strcpy(buffer,good_buffer);
}

void getNameBySocket(int socket, char dest[]){
    for(int i=0;i<online_counter;i++)
        if(online_users[i].socket==socket){
            strcpy(dest,online_users[i].name);
            break;
        }
}

int getSocketByName(char buf[]){
    for(int i=0;i<online_counter;i++)
        if(strcmp(online_users[i].name,buf)==0)
            return online_users[i].socket;
    return 0;
}

int existaDejaUserul(char *user_cautat) {
    FILE *fis;
    char buffer[100];

    fis = fopen(DATABASE, "r");
    if (fis == NULL) {
        printf("Eroare la deschidere fisier usernames.\n");
        exit(0);
    }

    while (fgets(buffer, sizeof(buffer), fis) != NULL) {
        remove_endline_from_buffer(buffer);
        if (strcmp(buffer, user_cautat) == 0) {
            fclose(fis);
            return 1;
        }
    }
    fclose(fis);
    return 0;
}

void adaugaUser(char* username){
    FILE *file;
    file = fopen(DATABASE, "a");
    if (file == NULL) {
        printf("Error opening the file.\n");
        return;
    }
    fprintf(file, "%s\n", username);
    fclose(file);
}

void extragereUsername(char sir_sursa[], int index, char sir_destinatie[]){
    if(index<0 || index>strlen(sir_sursa)){
        printf("Eroare, index gresit!\n");
        return;
    }
    int i;
    for(i=0;i<strlen(sir_sursa)-index;i++)
        sir_destinatie[i]=sir_sursa[index+i];
    sir_destinatie[i]='\0';
}

void addOnlineUser(int socket, char* username){
    if(online_counter<MAX_ONLINE_USERS){
        online_users[online_counter].socket=socket;
        strncpy(online_users[online_counter].name, username, sizeof(online_users[online_counter].name)-1);
        online_users[online_counter].name[sizeof(online_users[online_counter].name)-1]='\0';
        online_counter++;
        printf("Userul %s este online\n",username);        
    }
    else{
        printf("Nu mai am loc de useri online\n");
    }
}

void deleteOnlineUser(int socket){
    int i, exists=0;
    for(i=0;i<online_counter;i++){
        if(online_users[i].socket==socket){
            printf("Userul %s este offline\n",online_users[i].name);
            exists=1;
            break;
        }
    }
    if(exists){
        for(int ii=i;ii<online_counter-1;ii++)
            online_users[ii]=online_users[ii+1];
        online_counter--;
    }
    else{
        printf("A dat quit cineva care nu era logat\n");
    }
}

int userIsOnlineBySocket(int socket){
    for(int i=0;i<online_counter;i++)
        if(online_users[i].socket==socket)
            return 1;
    return 0;
}

int userIsOnlineByName(char name[]){
    for(int i=0;i<online_counter;i++)
        if(strcmp(online_users[i].name,name)==0)
            return 1;
    return 0;
}

//?tested OK
void afisare_useri_online(char dest[]){
    strcpy(dest,"Userii online sunt:\n");
    for(int i=0;i<online_counter;i++){
        strcat(dest,online_users[i].name);
        strcat(dest,"\n");
    }
}
//?tested OK
int fetchAllUsernames(char all_users[MAX_USERS][256]) {
    FILE *file;
    file = fopen(DATABASE, "r");
    if (file == NULL) {
        printf("Error opening the file.\n");
        return -1;
    }
    int i = 0;
    while (fgets(all_users[i], MAX_USERS, file) != NULL) {
        all_users[i][strcspn(all_users[i], "\n")] = '\0';
        i++;
        if (i >= MAX_USERS) {
            printf("Reached maximum number of lines.\n");
            break;
        }
    }
    return i;
    fclose(file);
}


void afisare_useri_offline(char dest[]){
    char offline_users[MAX_USERS][256];
    char all_users[MAX_USERS][256];
    int nr=fetchAllUsernames(all_users);
    int off_counter=0;
    for(int i=0;i<nr;i++)
        if(userIsOnlineByName(all_users[i])==0)
            strcpy(offline_users[off_counter++],all_users[i]);
    strcpy(dest,"Userii offline sunt:\n");
    for(int i=0;i<off_counter;i++){
        strcat(dest,offline_users[i]);
        strcat(dest,"\n");
    }
}

void trimit_unread(int from_socket, char to_username[], char msg_to_send[]){
    FILE *file;
    char path[256]="";
    char from_name[256]="";
    getNameBySocket(from_socket,from_name);
    snprintf(path,1024,"unread%s.txt",to_username);
    file = fopen(path, "a+");
    char finisare_mesaj[1024]="";
    snprintf(finisare_mesaj,2048,"[%s]: %s\n",from_name,msg_to_send);
    if (file == NULL) {
        printf("Nu s a deschis fisierul\n");
        return;
    }
    else fprintf(file,"%s",finisare_mesaj);
    fclose(file);
}

void clear_unread(char path[], char dest[]){

}

void citire_mesaje_unread(char pathname[], char dest[]){

}

char * conv_addr (struct sockaddr_in address){
    static char str[25];
    char port[7];
    strcpy (str, inet_ntoa (address.sin_addr));	
    bzero (port, 7);
    sprintf (port, ":%d", ntohs (address.sin_port));	
    strcat (str, port);
    return (str);
}


int callbackCount(void *data, int argc, char **argv, char **azColName) {
    int *result = (int *)data;
    *result = atoi(argv[0]);
    return 0;
}

int existaBlocare(sqlite3 *db, const char *Nume1, const char *Nume2) {
    char *errMsg = 0;
    int code;
    char sql_check[100];
    sprintf(sql_check, "SELECT COUNT(*) FROM Blocked WHERE Nume1 = '%s' AND Nume2 = '%s';", Nume1, Nume2);
    int result = 0;
    code = sqlite3_exec(db, sql_check, callbackCount, &result, &errMsg);
    if (code != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        return 0;
    }
    return result>0;
}

int inserareBlocare(sqlite3 *db, char *name1, char *name2) {
    //0 pt eroare, 1 pt succes, 2 pt exista deja
    char *errMsg = 0;
    int code;
    char sql_insert[100];
    if(existaBlocare(db,name1,name2)){
        printf("Aceasta pereche exista deja!\n");
        return 2;
    }
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

int stergereBlocare(sqlite3 *db, char *name1, const char *name2) {
    //0 pt eroare, 1 pt succes, 2 pt nu exista
    char *errMsg = 0;
    int code;
    char sql_delete[100];
    if(existaBlocare(db,name1,name2)==0){
        printf("Nu putem sterge aceasta pereche, ea nu exista!\n");
        return 2;
    }
    sprintf(sql_delete, "DELETE FROM Blocked WHERE Nume1 = '%s' AND Nume2 = '%s';", name1, name2);
    code = sqlite3_exec(db, sql_delete, 0, 0, &errMsg);
    if (code != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        return 0;
    }
    printf("Am sters cu succes din tabela Blocked!\n");
    return 1;
}

void executeSQLCommand(sqlite3* db, char* sql) {
    char* errMsg = 0;
    int code = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (code != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
}

void insertConversation(sqlite3* db, char* sender, char* receiver, char* message) {
    char sql[256];
    snprintf(sql, sizeof(sql), "INSERT INTO Conversatii (SenderName, ReceiverName, MsgContent) VALUES ('%s', '%s', '%s');",
             sender, receiver, message);
    
    char* errMsg=0;
    int code=sqlite3_exec(db,sql,0,0,&errMsg);
    if (code != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
}

char* getConversationsBetweenUsers(sqlite3* db, char* user1, char* user2) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT * FROM Conversatii WHERE (SenderName = '%s' AND ReceiverName = '%s') OR (SenderName = '%s' AND ReceiverName = '%s') ORDER BY Timestamp;",
             user1, user2, user2, user1);

    sqlite3_stmt* statement;
    int rc = sqlite3_prepare_v2(db, sql, -1, &statement, 0);

    if (rc != SQLITE_OK) {
        printf("SQLite error: %s\n", sqlite3_errmsg(db));
        exit(rc);
    }
    char* result = malloc(1);
    result[0] = '\0';

    while (sqlite3_step(statement) == SQLITE_ROW) {
        const char* id= sqlite3_column_text(statement, 0);
        const char* sender = sqlite3_column_text(statement, 1);
        const char* receiver = sqlite3_column_text(statement, 2);
        const char* message = sqlite3_column_text(statement, 3);

        char temp[256];
        snprintf(temp, sizeof(temp), "[ID:%s] %s -> %s: %s\n",id, sender, receiver, message);
        result = realloc(result, strlen(result) + strlen(temp) + 1);
        strcat(result, temp);
    }
    sqlite3_finalize(statement);
    return result;
}

int main () {
    //TODO initializari pentru stocare informatiilor, BD si fisiere .txt
    sqlite3* db;
    char* errMsg=0;
    int code;
    code=sqlite3_open("myDB.db",&db);
    if(code){
        printf("Nu putem deschide baza de date: %s\n",sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }
    else printf("Baza de date a fost deschisa!\n");

    char *sql_create = "CREATE TABLE Blocked (Nume1 TEXT, Nume2 TEXT);";
    code = sqlite3_exec(db, sql_create, 0, 0, &errMsg);
    if (code != SQLITE_OK) {
        fprintf(stderr, "SQL said: %s\n", errMsg);
        sqlite3_free(errMsg);
        printf("Nu mai cream tabelul Blocked, acesta exista!\n");
    } 
    else printf("Table 'Blocked' created successfully\n");
    
    sql_create = "CREATE TABLE Conversatii (ConversationID INTEGER PRIMARY KEY AUTOINCREMENT, SenderName TEXT, ReceiverName TEXT, MsgContent TEXT, Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    code = sqlite3_exec(db, sql_create, 0, 0, &errMsg);
    if (code != SQLITE_OK) {
        fprintf(stderr, "SQL said: %s\n", errMsg);
        sqlite3_free(errMsg);
        printf("Nu mai cream tabelul Conversatii, acesta exista!\n");
    }
    else printf("Tabelul 'Conversatii' creat cu succes!\n");


    //? manual introducere si interogare istoric conversatii
    // insertConversation(db,"Nume1","Nume2","Mesaj");
    // char* user1="Razvan";
    // char* user2="Ionel";
    // char* conversations=getConversationsBetweenUsers(db,user1,user2);
    // printf("Conversatii intre %s si %s:\n%s", user1, user2, conversations);
    // free(conversations);
    //? manual introducere si interogare istoric conversatii

    launchTextDB();


    struct sockaddr_in server,client_address;
    fd_set readfds,actfds;	
    struct timeval tv;	
    int sd, client, optval=1,fd,nfds,len, client_counter=0;		

    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1){
        perror ("[server] Eroare la socket().\n");
        return errno;
    }
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,&optval,sizeof(optval));
    bzero (&server, sizeof (server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl (INADDR_ANY);
    server.sin_port = htons (PORT);
    if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1){
        perror ("[server] Eroare la bind().\n");
        return errno;
    }
    if (listen (sd, 5) == -1){
        perror ("[server] Eroare la listen().\n");
        return errno;
    }
    
    FD_ZERO (&actfds);
    FD_SET (sd, &actfds);
    tv.tv_sec = 1;		
    tv.tv_usec = 0;
    nfds = sd;
    printf ("[server] Asteptam la portul %d...\n", PORT);
    fflush (stdout);     

    while (1){
        readfds=actfds;
        if (select (nfds+1, &readfds, NULL, NULL, &tv) < 0){
            perror ("[server] Eroare la select().\n");
            return errno;
        }
        int i;
        for(i=3;i<=nfds;i++){
            if(FD_ISSET(i,&readfds)){
                if(i==sd){
                    len = sizeof (client_address);
                    bzero (&client_address, sizeof (client_address));
                    client = accept (sd, (struct sockaddr *) &client_address, &len);

                    if (client < 0){
                        perror ("[server] Eroare la accept().\n");
                        continue;
                        return 1;
                    }
                    if (nfds < client)
                        nfds = client;
                            
                    FD_SET (client, &actfds);
                    printf("[server] S-a conectat clientul cu descriptorul %d, de la adresa %s.\n",client, conv_addr (client_address));
                }
                else{
                    char buf[1024]="";
                    char toBeCreated[1024]="";
                    int biti=read(i,buf,1024);
                    if(biti<1){
                        printf("S a deconectat un client\n");
                        if(userIsOnlineBySocket(i)==1)
                            deleteOnlineUser(i);
                        FD_CLR(i,&actfds);
                        close(i);
                    }
                    // afisare_useri_online();
                    int menu_flag=0,close_flag=0,signup_flag=0,login_flag=0,send_flag=0,reply_flag=0,history_flag=0,whoson_flag=0,whosoff_flag=0,whoall_flag=0,unread_flag=0;
                    
                    char current_username[256]="";
                    getNameBySocket(i,current_username);
                    remove_endline_from_buffer(buf);
                    if(strcmp(buf,"/menu")==0)
                        menu_flag=1;
                    else if(strcmp(buf,"/quit")==0){
                        close_flag=1;
                        deleteOnlineUser(i);
                    }
                    else if(wants_operation(buf,"/signup ")){
                        signup_flag=1;
                        if(userIsOnlineBySocket(i)){
                            send(i,mesaj_alreadyLogged,strlen(mesaj_alreadyLogged),0);
                            continue;
                        }
                        extragereUsername(buf,SIGNUP_START_INDEX,current_username);
                        if(existaDejaUserul(current_username)==0){
                            if(verify_username(current_username)){
                                adaugaUser(current_username);
                                send(i,"Autentificare cu succes, continua cu logarea!\n",200,0);
                            }
                            else send(i,mesaj_invalid_username,strlen(mesaj_invalid_username),0);
                            continue;
                        }
                        else{
                            send(i,mesaj_userExists,strlen(mesaj_userExists),0);\
                            continue;
                        }
                    }
                    else if(wants_operation(buf,"/login ")){
                        login_flag=1;
                        if(userIsOnlineBySocket(i)){
                            send(i,mesaj_alreadyLogged,strlen(mesaj_alreadyLogged),0);
                            continue;
                        }
                        extragereUsername(buf,LOGIN_START_INDEX,current_username);
                        if(existaDejaUserul(current_username)==1){
                            if(userIsOnlineByName(current_username)){
                                send(i,"Userul este deja conectat!\n",200,0);
                                continue;
                            }
                            else{
                                addOnlineUser(i,current_username);
                                char welcome_msg[256]="";
                                snprintf(welcome_msg,274,"Bine ai venit, %s!\n",current_username);
                                send(i,welcome_msg,strlen(welcome_msg),0);
                                continue;
                            }
                        }
                        else{
                            send(i,mesaj_errorSignup,strlen(mesaj_errorSignup),0);
                            continue;
                        }
                    }
                        
                    else if(wants_operation(buf,"/reply ")){
                        if(!userIsOnlineBySocket(i)){
                            send(i,mesaj_noPrivilege,strlen(mesaj_noPrivilege),0);
                            continue;
                        }
                        reply_flag=1;
                    }
                        
                    else if(wants_operation(buf,"/history ")){
                        if(!userIsOnlineBySocket(i)){
                            send(i,mesaj_noPrivilege,strlen(mesaj_noPrivilege),0);
                            continue;
                        }
                        else{
                            char username_to_send[256];
                            extragereUsername(buf,HISTORY_START_INDEX,username_to_send);
                            if(strcmp(username_to_send,current_username)==0){
                                send(i,"Nu poti vedea istoricul tau cu tine...!\n",100,0);
                                continue;
                            }
                            char* conversatie=getConversationsBetweenUsers(db,current_username,username_to_send);
                            if(conversatie[0]=='\0'){
                                send(i,"Nu ai vorbit inca cu acest utilizator!\n",100,0);
                                continue;
                            }
                            strcpy(toBeCreated,"");
                            snprintf(toBeCreated,4098,"Conversatii intre tine si %s:\n%s",username_to_send,conversatie);
                            send(i,toBeCreated,strlen(toBeCreated),0);
                            free(conversatie);
                        }
                        continue;
                    }
                        
                    else if(strcmp(buf,"/whoson")==0){
                        if(!userIsOnlineBySocket(i))
                            send(i,mesaj_noPrivilege,strlen(mesaj_noPrivilege),0);
                        else{
                            char result_onUsers[1024]="";
                            afisare_useri_online(result_onUsers);
                            send(i,result_onUsers,strlen(result_onUsers),0);
                        }
                        continue;
                        whoson_flag=1;
                    }
                        
                    else if(strcmp(buf,"/whosoff")==0){
                        if(!userIsOnlineBySocket(i))
                            send(i,mesaj_noPrivilege,strlen(mesaj_noPrivilege),0);
                        else{
                            char result_offUsers[1024]="";
                            afisare_useri_offline(result_offUsers);
                            send(i,result_offUsers,strlen(result_offUsers),0);
                        }
                        continue;
                        whosoff_flag=1;
                    }
                        
                    else if(strcmp(buf,"/whoall")==0){
                        if(!userIsOnlineBySocket(i))
                            send(i,mesaj_noPrivilege,strlen(mesaj_noPrivilege),0);
                        else{
                            char all_users[MAX_USERS][256];
                            int total=fetchAllUsernames(all_users);
                            char result_allUsers[1024]="Toti userii existenti sunt:\n";
                            for(int i=0;i<total;i++){
                                strcat(result_allUsers,all_users[i]);
                                strcat(result_allUsers,"\n");
                            }
                            send(i,result_allUsers,strlen(result_allUsers),0);
                        }
                        continue;
                        whoall_flag=1;
                    }
                        
                    else if(strcmp(buf,"/unread")==0){
                        if(!userIsOnlineBySocket(i)){
                            send(i,mesaj_noPrivilege,strlen(mesaj_noPrivilege),0);
                            continue;
                        }
                        else{
                            //functia access care aparent verifica daca exista fifser cu nume x
                            char pathname[256]="";
                            snprintf(pathname,512,"unread%s.txt",current_username);
                            if(fileExists(pathname)!=-1){
                                //fisieru exista, fac citire din el si trimitere mesaje unread
                                char msj_necitite[4096]="";
                                citire_date_din_fisier(pathname,msj_necitite);
                                remove(pathname);
                                send(i,msj_necitite,strlen(msj_necitite),0);
                            }
                            else{
                                //fisieru nu exista, nu ii trimit nica
                                send(i,"Nu aveti niciun mesaj necitit!\n",50,0);
                            }
                        }
                        unread_flag=1;
                        continue;
                    }
                        
                    else if(wants_send(buf)){
                        if(!userIsOnlineBySocket(i)){
                            send(i,mesaj_noPrivilege,strlen(mesaj_noPrivilege),0);
                        }
                        else{
                            char username_to_send[256]="", msg_to_send[256]="";
                            retrieve_data_fromSend(buf,username_to_send,msg_to_send);
                            if(existaDejaUserul(username_to_send)){
                                if(userIsOnlineByName(username_to_send)){
                                    if(strcmp(username_to_send,current_username)==0)
                                        send(i,"Nu ti poti trimite mesaje singur...:)\n",100,0);
                                    else{
                                        if(existaBlocare(db,username_to_send,current_username)){
                                            strcpy(toBeCreated,"");
                                            snprintf(toBeCreated,1024,"%s te-a blocat, deci nu ii poti trimite mesaje!\n",username_to_send);
                                            send(i,toBeCreated,strlen(toBeCreated),0);
                                        }
                                        else{
                                            int socket_to_send=getSocketByName(username_to_send);
                                            char full_msg[1024]="";
                                            snprintf(full_msg,2048,"[%s]: %s\n",current_username,msg_to_send);
                                            send(socket_to_send,full_msg,strlen(full_msg),0);
                                            send(i,"Am trimis mesajul!\n",200,0);
                                            insertConversation(db,current_username,username_to_send,msg_to_send);
                                        }
                                    }
                                }
                                else{
                                    send(i,"User-ul nu este online, ii trimitem acest mesaj in unread\n",200,0);
                                    trimit_unread(i,username_to_send,msg_to_send);
                                    insertConversation(db,current_username,username_to_send,msg_to_send);
                                }
                            }
                            else send(i,"Nu exista acest user, deci nu ii poti trimite mesaje!\n",200,0);
                        }
                        continue;
                        send_flag=1;
                    }
                    else if(strcmp(buf,"/disconnect")==0){
                        if(userIsOnlineBySocket(i)){
                            char mesaj_disconnectedSuccess[312];
                            char username[50];
                            getNameBySocket(i,username);
                            snprintf(mesaj_disconnectedSuccess,312,"Speram sa te revedem, %s!\n",username);
                            send(i,mesaj_disconnectedSuccess,strlen(mesaj_disconnectedSuccess),0);
                            deleteOnlineUser(i);
                        }
                        else{
                            send(i,mesaj_disconnectedFailure,strlen(mesaj_disconnectedFailure),0);
                        }
                        continue;
                    }
                    else if(wants_operation(buf,"/status ")==1){
                        if(!userIsOnlineBySocket(i)){
                            send(i,mesaj_noPrivilege,strlen(mesaj_noPrivilege),0);
                            continue;
                        }
                        else{
                            char username_to_send[256];
                            extragereUsername(buf,STATUS_START_INDEX,username_to_send);
                            if(existaDejaUserul(username_to_send)){
                                strcpy(toBeCreated,"");
                                if(existaBlocare(db,username_to_send,current_username)){
                                    strcpy(toBeCreated,"");
                                    snprintf(toBeCreated,1024,"%s te-a blocat, deci nu ii poti vedea statusul!\n",username_to_send);
                                    send(i,toBeCreated,strlen(toBeCreated),0);
                                }
                                else{
                                    if(userIsOnlineByName(username_to_send))
                                        snprintf(toBeCreated,2048,"%s este online!\n",username_to_send);
                                    else
                                        snprintf(toBeCreated,2048,"%s este offline!\n",username_to_send);
                                    send(i,toBeCreated,strlen(toBeCreated),0);
                                }
                            }
                            else{
                                send(i,mesaj_no_user,30,0);
                            }
                            continue;
                        }
                        continue;
                    }
                    else if(strcmp(buf,"/clear")==0){
                        send(i,mesaj_clean_screen,strlen(mesaj_clean_screen),0);
                        continue;
                    }

                    else if(wants_operation(buf,"/poke ")){
                        if(!userIsOnlineBySocket(i)){
                            send(i,mesaj_noPrivilege,strlen(mesaj_noPrivilege),0);
                            continue;
                        }
                        else{
                            char username_to_send[256];
                            extragereUsername(buf,POKE_START_INDEX,username_to_send);
                            if(existaDejaUserul(username_to_send)){
                                strcpy(toBeCreated,"");
                                if(existaBlocare(db,username_to_send,current_username)){
                                    snprintf(toBeCreated,1024,"%s te-a blocat, deci nu il poti ciupi!\n",username_to_send);
                                    send(i,toBeCreated,strlen(toBeCreated),0);
                                }
                                else{
                                    if(userIsOnlineByName(username_to_send)){
                                        snprintf(toBeCreated,1200,"%s te-a ciupit!\n",current_username);
                                        int j=getSocketByName(username_to_send);
                                        if(i==j) strcpy(toBeCreated,"Nu te poti ciupi singur!\n");
                                        send(j,toBeCreated,strlen(toBeCreated),0);
                                    }
                                        
                                    else{
                                        snprintf(toBeCreated,1200,"%s nu este online, deci nu putem face asta!\n",username_to_send);
                                        send(i,toBeCreated,strlen(toBeCreated),0);
                                    }
                                } 
                            }
                            else send(i,mesaj_no_user,strlen(mesaj_no_user),0);
                        }
                        continue;
                    }

                    else if(wants_operation(buf,"/block ")){
                        if(!userIsOnlineBySocket(i))
                            send(i,mesaj_noPrivilege,strlen(mesaj_noPrivilege),0);
                        else{
                            char newUsername[256];
                            extragereUsername(buf,LOGIN_START_INDEX,newUsername);
                            if(existaDejaUserul(newUsername)){
                                if(existaBlocare(db,current_username,newUsername))
                                    send(i,"Deja ai blocat acest user!\n",50,0);
                                else{
                                    int j=getSocketByName(newUsername);
                                    if(i==j)
                                        send(i,"Nu te poti bloca singur!\n",50,0);
                                    else{
                                        inserareBlocare(db,current_username,newUsername);
                                        strcpy(toBeCreated,"");
                                        snprintf(toBeCreated,2048,"L-ai blocat cu succes pe %s!\n",newUsername);
                                        send(i,toBeCreated,strlen(toBeCreated),0);
                                    }
                                } 
                            }
                            else send(i,mesaj_no_user,strlen(mesaj_no_user),0);
                        }
                        continue;
                    }
                    else if(wants_operation(buf,"/unblock ")){
                        if(!userIsOnlineBySocket(i))
                            send(i,mesaj_noPrivilege,strlen(mesaj_noPrivilege),0);
                        else{
                            strcpy(toBeCreated,"");
                            char newUsername[256];
                            extragereUsername(buf,UNBLOCK_START_INDEX,newUsername);
                            if(existaDejaUserul(newUsername)){
                                if(existaBlocare(db,current_username,newUsername)){
                                    stergereBlocare(db,current_username,newUsername);
                                    snprintf(toBeCreated,1024,"L-ai deblocat cu succes pe %s!\n",newUsername);
                                    send(i,toBeCreated,strlen(toBeCreated),0);
                                }
                                else{
                                    int j=getSocketByName(newUsername);
                                    if(i==j)
                                        send(i,"Nu te poti debloca singur...\n",50,0);
                                    else{    
                                        snprintf(toBeCreated,1024,"Nu l-ai blocat pe %s, deci nu il poti debloca!\n",newUsername);
                                        send(i,toBeCreated,strlen(toBeCreated),0);
                                    }
                                }
                            }
                            else send(i,mesaj_no_user,strlen(mesaj_no_user),0);
                        }
                        continue;
                    }
                        

                    for(int j=3;j<=nfds;j++){
                        if(FD_ISSET(j,&actfds)){
                            if(j==sd || j!=i) continue;
                            else{
                                if(menu_flag==1)
                                    send(j,mesaj_afisare_meniu,strlen(mesaj_afisare_meniu),0);
                                else if(close_flag==1){
                                    send(j,mesaj_goodbye,strlen(mesaj_goodbye),0);
                                    close(j);
                                    FD_CLR(j,&actfds);
                                }
                                else if(login_flag==1){
                                    //aici verific daca exista in usernames.txt;
                                    send(j,"Serverul va executa login\n",100,0);
                                }
                                else if(signup_flag==1){
                                    //aici verific daca exista in usernames.txt, daca nu, il si adaug
                                    send(j,"Serverul va executa signup\n",100,0);
                                }
                                else if(reply_flag==1){
                                    send(j,"Serverul va executa reply\n",100,0);
                                }
                                else if(history_flag==1){
                                    send(j,"Serverul va executa history\n",100,0);
                                }
                                else if(whosoff_flag==1){
                                    send(j,"Serverul va executa whosoff\n",100,0);
                                }
                                else if(whoson_flag==1){
                                    send(j,"Serverul va executa whoson\n",100,0);
                                }
                                else if(whoall_flag==1){
                                    send(j,"Serverul va executa whoall\n",100,0);
                                }
                                else if(unread_flag==1){
                                    send(j,"Serverul va executa unread\n",100,0);
                                }
                                else if(send_flag==1){
                                    send(j,"Serverul va executa send\n",100,0);
                                }
                                else{
                                    send(j,mesaj_unkown,strlen(mesaj_unkown),0);
                                }
                                    
                            }
                        }
                    }
                }
            }
        }
    }//while
    printf("Closing listening socket...\n");
    close(sd);
}//main				
