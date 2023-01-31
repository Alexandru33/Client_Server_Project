#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "helpers.h"
#include "afisari.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s id_client ip_server port_server \n", file);
	exit(0);
}

int main (int argc , char** argv)
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    if (argc < 4) {
		usage(argv[0]);
	}

    /*------ creare socket comunicare cu server-----*/
    int sockfd;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");


    /*------------creare server_addr---------*/
    struct sockaddr_in serv_addr;
    int ret; // <---- valoarea de return a diferitelor apeluri pentru detectare de erori


    serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "Nu a merts conversia inet_aton\n");

    /*-----------conectare la server ----------*/
    ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "Nu a mers connectul la server\n");
    
    /*--------trimitere ID la server---------*/
    ret = send ( sockfd, argv[1], strlen(argv[1])+1 , 0);
    DIE(ret < 0, "Nu a mers sa trimit IDul la server\n");

    
    /*--------creare fd_set-uri si gasire maxim--------*/
    fd_set readset, tempset;
	FD_ZERO( &readset );
	FD_ZERO( &tempset);
	FD_SET (STDIN_FILENO ,&readset);
	FD_SET (sockfd, &readset);

    // gasesc maximul 
	int maxim = STDIN_FILENO > sockfd ? STDIN_FILENO+1 : sockfd+1;

    while (1)
    {
        tempset = readset; /// <---- modific doar tempset-ul

		int rc = select ( maxim , &tempset , NULL, NULL, NULL );
		DIE ( rc < 0 , "Nu a mers slectul\n");

        if ( FD_ISSET ( STDIN_FILENO , &tempset ) ) // avem un imput de la tastatura
		{
			// se citeste de la stdin
            char buffer[BUFLEN];
			memset(buffer, 0, sizeof(buffer));
			int n = read(0, buffer, sizeof(buffer) - 1);
			DIE(n < 0, "read");

			if (strncmp(buffer, "exit", 4) == 0) // am citit "exit" => inchid socket-ul si la revdere!
            {
                close(sockfd);
                return 0;
			}
            if ( strncmp( buffer , "subscribe", 9) == 0) // am citit subscribe =>instiintez serverul ca vreau sa fac subscribe
            {
                ret = send ( sockfd, buffer, strlen(buffer) , 0); 
                DIE(ret < 0, "Nu a mers sa trimit SUBSCRIBE serverului\n");
                printf("Subscribed to topic.\n");


            }
            if ( strncmp( buffer , "unsubscribe", 11) == 0) // am citit unsubscribe
            {
                ret = send ( sockfd, buffer, strlen(buffer) , 0); 
                DIE(ret < 0, "Nu a mers sa trimit UNSUBSCRIBE serverului\n");
                printf("Unsubscribed to topic.\n");

            }
        }

        if (FD_ISSET ( sockfd , &tempset) )
		{

			char buffer[sizeof (mesajUDP) + 1] ;
			// serverul a scris ceva pe socket si eu fac recv() pe acel socket de la server
			memset(buffer, 0, sizeof(buffer));
			int n = recv( sockfd , buffer, sizeof(buffer) - 1, 0);
			DIE(n < 0, "Nu a mers recv-ul de la server\n");

            if ( n == 0 ) // s-a inchis serverul aiurea
            {
                close(sockfd);
                return 0;

            }
			
            
            mesajUDP* msjPrimit = (mesajUDP*) malloc ( sizeof ( mesajUDP));
            memcpy ( msjPrimit, buffer, n);

            printmesaj ( msjPrimit);
            //printf ("ID-ul mesajului %d\n", msjPrimit->idmesaj);
            


		}
        

    }



    return 0;
    
}