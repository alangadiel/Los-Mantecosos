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

#define TAMANIOHEADER 2
#define STRHANDSHAKE "10"


typedef struct
{
	char esHandShake;
	unsigned short tamPayload;
	//char emisor[11];
} Header;


typedef struct
{
	Header header;
	char* Payload;
} Paquete;


int StartServidor(char* MyIP,int MyPort);
int ConectarServidor(int PUERTO_KERNEL,char* IP_KERNEL);
void EnviarPaquete(int socketCliente, Paquete* msg,int cantAEnviar);
void EnviarMensaje(int socketFD, char* msg);
void EnviarHandshake(int socketFD);
char* RecibirHandshake(int socketFD);
char* RecibirMensaje(int socketFD);
void EnviarHandshakeString(int socketFD);
void EnviarMensajeString(int socketFD,char* str);
void RecibirMensajeString(int socketFD);
int RecibirPaquete(void* paquete,int socketFD,unsigned short cantARecibir);


#endif //SOCKETS_H_
