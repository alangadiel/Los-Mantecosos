/*
 * SocketsL.c
 *
 *  Created on: 14/4/2017
 *      Author: utnso
 */


#include "SocketsL.h"

void EnviarPaquete(int socketCliente,Paquete msg){
	char* punteroMsg = &msg;
	int largo = sizeof(msg);
	int enviado; //bytes enviados
	do {
		enviado = send(socketCliente,punteroMsg,largo,0);
		largo -= enviado;
		punteroMsg += enviado; //avanza la cant de bytes que ya mando
	} while (largo!=0);
}
 void EnviarMensaje(int socketFD, char* msg,char* emisor){
	Paquete paquete;
	Header header;
	header.esHandShake='0';
	header.tamPayload= sizeof(msg);
	header.emisor= emisor;
	paquete.header=header;
	paquete.Payload=msg;
	EnviarPaquete(socketFD,paquete);
}

void EnviarHandshake(int socketFD,char * emisor){
	Paquete paquete;
	Header header;
	header.esHandShake='1';
	header.tamPayload= 0;
	header.emisor= emisor;
	paquete.header=header;
	paquete.Payload=(void*)0; //Seria lo mismo que NULL
	EnviarPaquete(socketFD,paquete);
}
char* RecibirHandshake(int socketFD){
	Paquete paquete;
	int var = recv(socketFD,&paquete,sizeof(paquete),0);
	if (var!=-1)
		return "Se recibió el mensaje";
	else
		return "Hubo un error al intentar recibir";
}

char* RecibirMensaje(int socketFD){
	Paquete paquete;
	int var = recv(socketFD,&paquete,sizeof(paquete),0);
	if (var!=-1)
		return "Se recibió el mensaje";
	else
		return "Hubo un error al intentar recibir";
}
