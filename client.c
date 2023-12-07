#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <termios.h>
extern int errno;
int port;

int main (int argc, char *argv[]) {
    int sd;
    char msg[100];
    struct sockaddr_in server;
    if (argc != 3){
        printf ("[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }
    port = atoi (argv[2]);
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1){
        perror ("[client] Eroare la socket().\n");
        return errno;
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons (port);
    printf("Connecting...\n");
    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1){
        perror ("[client]Eroare la connect().\n");
        return errno;
    }
    char name_msg[100];
    bzero (name_msg, 100);
    printf ("Bun venit la Offline Messenger! Pentru a vedea optiunile, scrieti /menu\n");
    fflush (stdout);

    while(1){
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sd,&readfds);
        FD_SET(fileno(stdin),&readfds);
        struct timeval tv;
        tv.tv_sec = 1;		
        tv.tv_usec = 0;
        if(select(sd+1,&readfds,NULL,NULL,&tv)<0){
            printf("[client]Eroare la select\n");
            return errno;
        }
        if(FD_ISSET(sd,&readfds)){
            char buf[1024];
            int biti=read(sd,buf,1024);
            buf[biti]='\0';
            if(biti<1){
                printf("Conexiune inchisa\n");
                break;
            }
            printf("%s",buf);
            if(strcmp(buf,"La revedere!\n")==0){
                close(sd);
                return 0;
            }
        }
        if(FD_ISSET(fileno(stdin),&readfds)){
            char buf[1024];
            if(!fgets(buf,1024,stdin)) break;
            int biti=send(sd,buf,strlen(buf),0);
        }
    }
    close (sd);
}
