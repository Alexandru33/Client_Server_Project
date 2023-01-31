#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "structuri.h"
#include "helpers.h"




void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}
/*----- VECTORI GLOBALI-------*/

pairID_SOCK *perechi;
int nr_perechi = 0;

topic *topics;
int nr_topicuri = 0;

int id_mesaj_UDP = 1;
celula* storage;
int nr_celule = 0;
/*---------------------------*/

#include "prelucrariUDP.h"

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	// Verificare cate argumente primesc
	if (argc < 2)
	{
		usage(argv[0]);
	}

	/*------------------Variabile----------------*/
	topics = (topic *)malloc(CHUNK * sizeof(topic));
	perechi = (pairID_SOCK *)malloc(CHUNK * sizeof(pairID_SOCK));
	storage =  (celula*) malloc ( CHUNK * sizeof (celula));

	int socketUdp, socketTcp, portNr, newsocket;
	struct sockaddr_in serverAddr, clientAddr;

	portNr = atoi(argv[1]);
	DIE(portNr == 0, "portNr e ciudat");

	/*------ Server Address----------*/

	memset((char *)&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(portNr);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	/*------------------------------------*/

	/*--------SOCKET TCP---------------*/

	socketTcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(socketTcp < 0, "socketulTCP a murit\n");
	

	if (bind(socketTcp, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) < 0)
	{
		perror("Eroare la bindingul socketului de TCP\n");
		exit(-1);
	}

	int flag = 1;
	if (setsockopt(socketTcp, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == -1)
	{
		perror("Eroare in dezactivarea algoritmului Nagle la socketulTcp\n");
		exit(-1);
	};

	if (listen(socketTcp, 1000) < 0) // 1000 e nr maxim de clients
	{
		perror("Eroare la ascultarea pe socketul de TCP\n");
		exit(-1);
	}
	/*--------------------------------*/

	/*-------Socket UDP---------------*/

	socketUdp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(socketUdp < 0, "socketulUDP a murit\n");
	if (bind(socketUdp, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		perror("Eroare la bindingul socketului de UDP");
		exit(-1);
	}

	/*------------------------------- */

	/*------Seturile de file descriptori----*/
	fd_set setCitire;
	fd_set setCopie;

	FD_ZERO(&setCitire);
	FD_ZERO(&setCopie);

	// pun cei 2 socketi si fd-ul STDIN-ului (adica 0) in setul de file descriptori
	FD_SET(0, &setCitire);
	FD_SET(socketUdp, &setCitire);
	FD_SET(socketTcp, &setCitire);

	int fdmax = socketUdp;
	if (socketTcp > fdmax)
		fdmax = socketTcp;

	/*------------------------------------*/

	/*----- CONFIGURARI DE VECTORI-----------*/

	

	while (1)
	{
		//printf( "Sunt in while\n");
		setCopie = setCitire;
		if (select(fdmax + 1, &setCopie, NULL, NULL, NULL) < 0)
		{
			perror("Eroare la functia select() ");
			exit(-1);
		}

		// Trec prin toti socketii si vad care este activ
		for (int i = 0; i <= fdmax; ++i)
		{

			if (FD_ISSET(i, &setCopie))
			{

				if (i == 0) // Am citit ceva de la tastatura (pot citi doar mesajul de "exit")
				{
					char buff[BUFLEN];
					memset(buff, 0, BUFLEN);
					fgets(buff, BUFLEN, stdin);

					if (strcmp(buff, "exit\n") == 0)
					{
						// s-a primit "exit" de la STDIN, s-a terminat joaca
						for ( int j=1 ; j < fdmax+1 ; ++j)
						{
							if ( FD_ISSET( j , &setCitire));
							int ret = shutdown(j, SHUT_RDWR );
							DIE( (ret = 0) , "Nu s-a putut face SHUTDOWN pe socket\n");
						}
						return 0;
					}
					else
					{
						// s-a primit altceva, dar nu "exit" de la STDIN
						printf("Input de la tastatura nedefinit\n");
					}
				}

				else if (i == socketUdp) // primesc un mesaj UDP despre un topic
				{
					char buff[BUFLEN];
					memset(buff, 0, BUFLEN);

					memset(&clientAddr, 0, sizeof(struct sockaddr_in));
					socklen_t len = sizeof(clientAddr);

					int n = recvfrom(socketUdp, buff, BUFLEN, 0, (struct sockaddr *)&clientAddr, &len);
					
					DIE(n < 0, "Eroare la recvfrom() de la un client UDP\n");

					struct mesajUDP mesaj_primit;

					mesaj_primit.idmesaj = id_mesaj_UDP;
					id_mesaj_UDP++;
					mesaj_primit.ip_udp = clientAddr.sin_addr.s_addr;
					mesaj_primit.port_udp = ntohs(clientAddr.sin_port);
					memset(mesaj_primit.mesaj, 0, BUFLEN);
					memcpy(mesaj_primit.mesaj, buff, n);

					
					prelucraremesajUDP ( mesaj_primit);
				}
				else if (i == socketTcp) // un client TCP vrea sa se conecteze la mine
				{
					socklen_t len = sizeof(clientAddr);
					newsocket = accept(socketTcp, (struct sockaddr *)&clientAddr, &len);
					DIE(newsocket < 0, "Nu a mers accept-ul\n");
					
					

					char buff[BUFLEN];
					/*------bagam noul socket in setCitire si actualizam fdmax-ul----*/
					FD_SET(newsocket, &setCitire);
					if (newsocket > fdmax)
					fdmax = newsocket;
					/*---------------------------------------*/
					/*---------primesc mesaj de la newsocket in care mi se transmite id-ul-----*/
					int bytes = recv(newsocket, buff, BUFLEN, 0);
					DIE(bytes < 0, "NU am primit ID-ul de la un nou client TCP");

					//acum am in buff[] doar id-ul noului client TCP conectat
					/*-------actualizam, daca e nevoie, vectorul de perechi ID_SOCKET ----*/
					int found = 0;
					int nume_identic = 0;
					for (int i = 0; i < nr_perechi; ++i)
					{
						if (strcmp(buff, perechi[i].id) == 0) // a mai fost pe aici acel client, deci sa ii actualizam socketul
						{
							if ( perechi[i].socket != -1) // noua cerere de conectare vine de la un client cu ID identic cu al altuia
							{
								nume_identic = 1;
								printf ("Client %s already connected.\n", buff);
								close(newsocket);
								FD_CLR(newsocket, &setCitire);

								
							}
							else // noua cerere de conectare este in regula
							{
								
								
								perechi[i].socket = newsocket;
								printf("New client %s connected from %s:%d.\n", buff , inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port) );
								
								send_stored_messages ( i );
								found = 1;
							}
							
							

							break;
						}
					}
					if (found == 0 && nume_identic == 0) // daca nu a fost gasit printre clientii vechi, facem o pereche noua
					{
						printf("New client %s connected from %s:%d.\n", buff , inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port) );
						perechi[nr_perechi].nr_mesaje_stored = 0;
						perechi[nr_perechi].id_mesaje_stored = (int *) calloc ( MAXSTORECAPACITY, sizeof (int));
						perechi[nr_perechi].socket = newsocket;
						strcpy(perechi[nr_perechi].id, buff);

						nr_perechi++;
						if (nr_perechi % CHUNK == 0) // daca e cazul, reallocam vectorul de perechi
						{
							
							perechi = (pairID_SOCK *)realloc(perechi, (CHUNK + nr_perechi) * sizeof(pairID_SOCK));
						}
					}
				}
				else // primesc mesaje de la un client TCP deja conectat
				{
					char buffer[BUFLEN];
					memset(buffer, 0, BUFLEN);
					int n = recv(i, buffer, BUFLEN, 0);
					DIE(n < 0, "Probleme la recv() de la un client TCP socketat\n");
					

					if (n == 0) // clientul TCP de pe socketul "i" se deconecteaza
					{
						
						close(i);
						FD_CLR(i, &setCitire);
						int doarodata = 0;
						// in vectorul de perechi, notam socketul clientului care s-a deconectat cu -1
						// Ar trebui sa facem aceasta operatie doar la un singur element, intrucat pe
						// socketul acela era doar un client.
						// Daca facem operatia de setare a socketului la -1 de mai multe ori sau nicioadata, e o eroare pe undeva.

						for (int j = 0; j < nr_perechi; ++j)
						{
							if (perechi[j].socket == i)
							{
								doarodata++;
								perechi[j].socket = -1;
								printf("Client %s disconnected.\n", perechi[j].id);
							}
						}
						DIE ((doarodata != 1) , "Am setat mai multi socketi la -1 la deconectarea unui client TCP\n");
					}
					else if (strncmp(buffer, "subscribe", 9) == 0) // avem un apel de SUBSCRIBE
					{
						
						
						char topicul[50];
						int sful;

						strcpy(topicul, (buffer + 10));
						DIE ( (isdigit ( topicul [strlen(topicul) - 2] == 0 )), "Format gresit la SUBSCRIBE\n");
						

						sful = atoi(topicul + strlen(topicul) - 2);
						DIE ( (sful != 1 && sful != 0) , "Numar SF gresit la SUBSCRIBE\n" );

						topicul[strlen(topicul) - 3] = '\0'; // scoatem sful si newlineul din topicul[]
						

						int found = 0;
						int index = -1;
						for (int j = 0; j < nr_topicuri; ++j)
						{
							if (strcmp(topicul, topics[j].title) == 0)
							{
								found++;
								index = j;
							}
						}
						DIE(found > 1, "Am gasit 2 topicuri cu acelasi nume\n");

						if (found == 0) // clientul a dat SUBSCRIBE la un topic care nu exista, deci creez un nou topic
						{
							topics[nr_topicuri].sfs = (int *)malloc(CHUNK * sizeof(int));
							topics[nr_topicuri].sfs[0] = sful;

							topics[nr_topicuri].iduri = (char **)malloc(CHUNK * sizeof(char *));
							for (int k = 0; k < CHUNK; ++k)
							{
								topics[nr_topicuri].iduri[k] = (char *)malloc(ID_LENGTH * sizeof(char));
							}

							strcpy(topics[nr_topicuri].title, topicul);

							topics[nr_topicuri].nr_abonati = 1;

							for (int k = 0; k < nr_perechi; ++k)
							{
								if (perechi[k].socket == i)
								{
									strcpy(topics[nr_topicuri].iduri[0], perechi[k].id);

									break;
								}
							}
							nr_topicuri++;
						}
						if (found == 1) // clientul a dat SUBSCRIBE la topicul de pe pozitia "index" din vector, deci il adaugam la abonati
						{
							
							int abonati = topics[index].nr_abonati;

							for (int k = 0; k < nr_perechi; ++k)
							{
								if (perechi[k].socket == i)
								{
									strcpy(topics[index].iduri[abonati], perechi[k].id);
									break;
								}
							}
							topics[index].sfs[abonati] = sful;
							topics[index].nr_abonati++;

							// daca e nevoie facem reallocuri

							if (topics[index].nr_abonati % CHUNK == 0)
							{
								topics[index].iduri = (char **)realloc(topics[index].iduri, (CHUNK + topics[index].nr_abonati) * sizeof(char *));
								for (int k = nr_topicuri; k < nr_topicuri + CHUNK; ++k)
								{
									topics[index].iduri[k] = (char *)malloc(ID_LENGTH * sizeof(char));
								}

								topics[index].sfs = (int *)realloc(topics[index].sfs, (CHUNK + topics[index].nr_abonati) * sizeof(int));
							}
						}
					}
					else if (strncmp(buffer, "unsubscribe", 11) == 0) // clientul TCP vrea sa se dezaboneze de la un topic la care era deja abonat!!!!
					{
						char topicul[50];
						char idul[ID_LENGTH];

						for (int j = 0; j < nr_perechi; ++j)
						{
							if (perechi[j].socket == i)
							{
								strcpy(idul, perechi[j].id);
							}
						}

						strcpy(topicul, buffer + 12);
						topicul[strlen(topicul) - 1] = '\0';

						int found  = 0; // un found de verificare

						for (int j = 0; j < nr_topicuri; ++j)
						{
							if (strcmp(topicul, topics[j].title) == 0) // am gasit topicul
							{
								found++;
								int abonati = topics[j].nr_abonati;
								if (abonati == 1) // daca avem doar 1 abonat, scadem doar nr_abonati
								{
									topics[j].nr_abonati--;
								}
								else
								{
									for (int k = 0; k < topics[j].nr_abonati; ++k)
									{
										if (strcmp(idul, topics[j].iduri[k]) == 0)
										{
											//am gasit clientul care vrea sa se dezaboneze
											// mutam ultimul abonat in locul acestuia (valabil si pentru sf-uri)
											// scadem numarul abonatilor

											strcpy(topics[j].iduri[k], topics[j].iduri[abonati - 1]);
											topics[j].sfs[j] = topics[j].sfs[abonati - 1];
											topics[j].nr_abonati--;
										}
									}
								}
							}
						}
						DIE ( found > 1, "Am gasit 2 topicuri cu acelasi nume in timp ce faceam UNSUBSCRIBE");
						DIE ( found < 1, "S-a dat UNSUBSCRIBE la un topic inexistent");
					}

					
				}
				
			}
		}
	}
	close(socketTcp);
	close(socketUdp);

	return 0;
}