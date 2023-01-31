#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN	 2000	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	5	// numarul maxim de clienti in asteptare
#define CHUNK 100 	// capacitatea dupa care facem realloc()
#define ID_LENGTH 100 // lungimea maxima a unui id de client TCP
#define MAXSTORECAPACITY 1000  // numarul maxim de mesaje pe care le pot tine minte pentruun client

#endif

typedef struct mesajUDP {
	int idmesaj;
	uint64_t ip_udp;
	uint16_t port_udp;
	char mesaj[BUFLEN]; 

} mesajUDP;

typedef struct abonament {
	char subiect[50];
	int SF;
} abonament;

typedef struct pairID_SOCK {
	int socket;
	char id[100];
	int* id_mesaje_stored;
	int nr_mesaje_stored;

} pairID_SOCK;


typedef struct topic {
	char title[50];
	char** iduri;
	int* sfs;
	int nr_abonati;
} topic;

typedef struct celula
{
	int nr_subscriberi;
	mesajUDP mesajUDP;
	
} celula;

