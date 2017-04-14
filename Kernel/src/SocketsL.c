/*
 * SocketsL.c
 *
 *  Created on: 14/4/2017
 *      Author: utnso
 */
#include "SocketsL.c"

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

void EnviarMensaje(int socketFD, char* msg){
	Paquete paquete;
	Header header;
	header.esHandShake='0';
	header.tamPayload= sizeof(msg);
	header.emisor= "Kernel";
	paquete.header=header;
	paquete.Payload=msg;
	EnviarPaquete(socketFD,paquete);
}

void Handshake(int socketFD){
	Paquete paquete;
	Header header;
	header.esHandShake='1';
	header.tamPayload= 0;
	header.emisor= "Kernel";
	paquete.header=header;
	paquete.Payload=NULL;
	EnviarPaquete(socketFD,paquete);
}

