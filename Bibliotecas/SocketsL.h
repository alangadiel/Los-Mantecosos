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

#define TAMANIOHEADER 14
#define STRHANDSHAKE "10"
#define PAYLOADHANDSHAKE "HANDSHAKE"
#define CONEXIONCERRADA -2
#define ERRORRECEPCION -1

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


int StartServidor(char* MyIP,int MyPort);
int ConectarServidor(int PUERTO_KERNEL,char* IP_KERNEL);
void EnviarPaquete(int socketCliente, Paquete* msg, int cantAEnviar);
void EnviarMensaje(int socketFD, char* msg,char emisor[11]);
void EnviarHandshake(int socketFD,char emisor[11]);
//char* RecibirHandshake(int socketFD);
//char* RecibirMensaje(int socketFD);
void EnviarHandshakeString(int socketFD);
void EnviarMensajeString(int socketFD,char* str);
void RecibirMensajeString(int socketFD);
int RecibirPaquete(void* paquete, int socketFD, unsigned short cantARecibir);
int RecibirPayload(int socketFD,char* mensaje,unsigned short tamPayload);
int RecibirHeader(int socketFD,Header* headerRecibido);


#endif //SOCKETS_H_
