#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <stdio.h>
#include <math.h>

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
#include <fcntl.h>
#include <sys/sendfile.h>
#include <time.h>


#define TAMANIOHEADER sizeof(Header)
#define STRHANDSHAKE "10"
//Emisores:
#define KERNEL "Kernel    "
#define CONSOLA "Consola   "
#define FS "FileSystem"
#define MEMORIA "Memoria   "
#define CPU "CPU       "
//Tipos de envios:
#define ESHANDSHAKE -1
#define ESDATOS 0 //tipo generico
#define ESSTRING 1
#define ESARCHIVO 2
#define ESINT 3
#define ESPCB 4
#define KILLPROGRAM 5
//API Memoria:
#define INIC_PROG 0
#define SOL_BYTES 1
#define ALM_BYTES 2
#define ASIG_PAG 3
#define FIN_PROG 4

typedef struct
{
	int8_t tipoMensaje;
	uint32_t tamPayload;
	char emisor[11];
}  __attribute__((packed)) Header;


typedef struct
{
	Header header;
	void* Payload;
}  __attribute__((packed)) Paquete;

void Servidor(char* ip, int puerto, char nombre[11], void (*accion)(Paquete* paquete, int socketFD));
int StartServidor(char* MyIP,int MyPort);
int ConectarAServidor(int puertoAConectar, char* ipAConectar, char servidor[11], char cliente[11]);

void EnviarHandshake(int socketFD,char emisor[11]);
void EnviarDatos(int socketFD,char emisor[11], void* datos, int tamDatos);
void EnviarPaquete(int socketCliente, Paquete* paquete);
void EnviarMensaje(int socketFD, char* msg,char emisor[11]);

void RecibirHandshake(int socketFD,char emisor[11]);
int RecibirDatos(void* paquete, int socketFD, uint32_t cantARecibir);
int RecibirPaqueteServidor(int socketFD, char receptor[11], Paquete* paquete); //Responde al recibir un Handshake
int RecibirPaqueteCliente(int socketFD, char receptor[11], Paquete* paquete); //No responde los Handshakes

//Interfaz para comunicarse con Memoria: (definido por el TP, no se puede modificar)
uint32_t IM_InicializarPrograma(int socketFD,char emisor[11], uint32_t ID_Prog, uint32_t CantPag); //Solicitar paginas para un programa nuevo, devuelve la cant de paginas que pudo asignar
void* IM_LeerDatos(int socketFD,char emisor[11], uint32_t ID_Prog, uint32_t PagNum, uint32_t offset, uint32_t cantBytes); //Devuelve los datos de una pagina, ¡Recordar hacer free(puntero) cuando los terminamos de usar!
void IM_GuardarDatos(int socketFD,char emisor[11], uint32_t ID_Prog, uint32_t PagNum, uint32_t offset, uint32_t cantBytes, void* buffer);
uint32_t IM_AsignarPaginas(int socketFD,char emisor[11], uint32_t ID_Prog, uint32_t CantPag);//Devuelve la cant de paginas que pudo asignar
void IM_FinalizarPrograma(int socketFD,char emisor[11], uint32_t ID_Prog);//Borra las paginas de ese programa.

#endif //SOCKETS_H_
