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


typedef struct {
	char esHandShake;
	unsigned short tamPayload;
	char* emisor;
} Header;
typedef struct {
	Header header;
	char* Payload;
} Paquete;


void EnviarPaquete(int socketCliente,Paquete msg);
void EnviarMensaje(int socketFD, char* msg,char* emisor);
void EnviarHandshake(int socketFD,char* emisor);
char* RecibirHandshake(int socketFD);
char* RecibirMensaje(int socketFD);
#endif //SOCKETS_H_
