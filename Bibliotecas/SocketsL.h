/*
 * SocketsL.h
 *
 *  Created on: 14/4/2017
 *      Author: utnso
 */


#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/txt.h>


#define TAMANIOHEADER 14
#define STRHANDSHAKE "10"

#define KERNEL "Kernel    "
#define CONSOLA "Consola   "
#define FS "FileSystem"
#define MEMORIA "Memoria   "
#define CPU "CPU       "

typedef struct
{
	char esHandShake;
	unsigned short tamPayload;
	char emisor[11];
}  __attribute__((packed)) Header;


typedef struct
{
	Header header;
	void* Payload;
}  __attribute__((packed)) Paquete;

//void Servidor(char nombre[11], void (*queHacer)(void* payload));
//void Cliente(void (*f)());
int StartServidor(char* MyIP,int MyPort);
int ConectarServidor(int PUERTO_KERNEL, char* IP_KERNEL, char servidor[11], char cliente[11]);
void EnviarPaquete(int socketCliente, Paquete* msg, int cantAEnviar);
void EnviarMensaje(int socketFD, char* msg,char emisor[11]);
void EnviarHandshake(int socketFD,char emisor[11]);
void RecibirHandshake(int socketFD,char emisor[11]);
int RecibirDatos(void* paquete, int socketFD, unsigned short cantARecibir);
int RecibirPaquete(int socketFD, char receptor[11], Paquete* paquete);

#endif //SOCKETS_H_
