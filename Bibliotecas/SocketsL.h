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
#include <pthread.h>

#define TAMANIOHEADER sizeof(Header)
#define STRHANDSHAKE "10"
//Emisores:
#define KERNEL "Kernel    "
#define CONSOLA "Consola   "
#define FS "FileSystem"
#define MEMORIA "Memoria   "
#define CPU "CPU       "
//Tipos de envios:
#define ESHANDSHAKE '1'
#define ESSTRING '0'
#define ESARCHIVO '2'
#define ESINT '3'
#define ESPCB '4'
//usar uint32_t

//API Memoria: (usar uint32_t)

#define INIC_PROG 0
#define SOL_BYTES 1
#define ALM_BYTES 2
#define ASIG_PAG 3
#define FIN_PROG 4

typedef struct
{
	char tipoMensaje;
	uint32_t tamPayload;
	char emisor[11];
}  __attribute__((packed)) Header;


typedef struct
{
	Header header;
	void* Payload;
}  __attribute__((packed)) Paquete;

void Servidor(char* ip, int puerto, char nombre[11], void (*accion)(Paquete* paquete, int socketFD));
//void Cliente(void (*f)());
int StartServidor(char* MyIP,int MyPort);
int ConectarServidor(int puertoAConectar, char* ipAConectar, char servidor[11], char cliente[11]);
void EnviarPaquete(int socketCliente, Paquete* paquete, int cantAEnviar);
void EnviarInt(int socketFD, int numero,char emisor[11]);
void EnviarMensaje(int socketFD, char* msg,char emisor[11]);
void EnviarHandshake(int socketFD,char emisor[11]);
void RecibirHandshake(int socketFD,char emisor[11]);
int RecibirDatos(void* paquete, int socketFD, uint32_t cantARecibir);
int RecibirPaquete(int socketFD, char receptor[11], Paquete* paquete);

#endif //SOCKETS_H_
