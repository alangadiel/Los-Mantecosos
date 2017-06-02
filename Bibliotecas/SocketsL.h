#ifndef SOCKETS_H_
#define SOCKETS_H_

#include "Helper.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10 // Cuántas conexiones pendientes se mantienen en cola del server
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
#define LIBE_PAG 4
#define FIN_PROG 5

typedef struct {
	int8_t tipoMensaje;
	uint32_t tamPayload;
	char emisor[11];
}__attribute__((packed)) Header;

typedef struct {
	Header header;
	void* Payload;
}__attribute__((packed)) Paquete;

void Servidor(char* ip, int puerto, char nombre[11],
		void (*accion)(Paquete* paquete, int socketFD),
		int (*RecibirPaquete)(int socketFD, char receptor[11], Paquete* paquete));
int StartServidor(char* MyIP, int MyPort);
int ConectarAServidor(int puertoAConectar, char* ipAConectar, char servidor[11], char cliente[11],
		void RecibirElHandshake(int socketFD, char emisor[11])); //Sobrecarga para Kernel

void EnviarHandshake(int socketFD, char emisor[11]);
void EnviarDatos(int socketFD, char emisor[11], void* datos, int tamDatos);
void EnviarPaquete(int socketCliente, Paquete* paquete);
void EnviarMensaje(int socketFD, char* msg, char emisor[11]);

void RecibirHandshake(int socketFD, char emisor[11]);
int RecibirDatos(void* paquete, int socketFD, uint32_t cantARecibir);
int RecibirPaqueteServidor(int socketFD, char receptor[11], Paquete* paquete); //Responde al recibir un Handshake
int RecibirPaqueteCliente(int socketFD, char receptor[11], Paquete* paquete); //No responde los Handshakes

//Interfaz para comunicarse con Memoria: (definido por el TP, no se puede modificar)
/*
 * Inicializar programa
Parámetros: [Identificador del Programa] [Páginas requeridas]
Cuando el Proceso Kernel comunique el inicio de un nuevo Programa, se crearán las
estructuras necesarias para administrarlo correctamente. En una misma página no se podrán
tener datos referentes a 2 segmentos diferentes (por ej. Código y Stack, o Stack y Heap).
 */
uint32_t IM_InicializarPrograma(int socketFD, char emisor[11], uint32_t ID_Prog,
		uint32_t CantPag); //Solicitar paginas para un programa nuevo, devuelve la cant de paginas que pudo asignar
/*
 * Solicitar bytes de una página
Parámetros: [Identificador del Programa], [#página], [offset] y [tamaño]
Ante un pedido de lectura de página de alguno de los procesos CPU, se realizará la
traducción a marco (frame) y se devolverá el contenido correspondiente consultando
primeramente a la Memoria Caché. En caso de que esta no contenga la info, se informará un
Cache Miss, se deberá cargar la página en Caché y se devolverá la información solicitada.
 */
void* IM_LeerDatos(int socketFD, char emisor[11], uint32_t ID_Prog,
		uint32_t PagNum, uint32_t offset, uint32_t cantBytes); //Devuelve los datos de una pagina, ¡Recordar hacer free(puntero) cuando los terminamos de usar!
/*
 * Almacenar bytes en una página
 * Parámetros: [Identificador del Programa], [#página], [offset], [tamaño] y [buffer]
Ante un pedido de escritura de página de alguno de los procesadores, se realizará la
traducción a marco (frame), y se actualizará su contenido. En caso de que la página se
encuentre en Memoria Caché, se deberá actualizar también el frame alojado en la misma.
 */
void IM_GuardarDatos(int socketFD, char emisor[11], uint32_t ID_Prog,
		uint32_t PagNum, uint32_t offset, uint32_t cantBytes, void* buffer);
/*
 * Asignar Páginas a Proceso
Parámetros: [Identificador del Programa] [Páginas requeridas]
Ante un pedido de asignación de páginas por parte del kernel, el proceso memoria deberá
asignarle tantas páginas como se soliciten al proceso ampliando así su tamaño. En caso de
que no se pueda asignar más páginas se deberá informar de la imposibilidad de asignar por
falta de espacio.
 */
uint32_t IM_AsignarPaginas(int socketFD, char emisor[11], uint32_t ID_Prog,	uint32_t CantPag); //Devuelve la cant de paginas que pudo asignar
/*
 * Liberar Página de un Proceso
  Parámetros: [Identificador del Programa] [Número de Página elegida]
  Ante un pedido de liberación de página por parte del kernel, el proceso memoria deberá liberar
  la página que corresponde con el número solicitado. En caso de que dicha página no exista
  o no pueda ser liberada, se deberá informar de la imposibilidad de realizar dicha operación
  como una excepcion de memoria.
 */
uint32_t IM_LiberarPagina(int socketFD, char emisor[11], uint32_t ID_Prog, uint32_t NumPag); //Agregado en el Fe de Erratas
/*
 * Finalizar programa
Parámetros: [Identificador del Programa]
Cuando el Proceso Kernel informe el fin de un Programa, se deberán eliminar las entradas en
estructuras usadas para administrar la memoria.
 */
void IM_FinalizarPrograma(int socketFD, char emisor[11], uint32_t ID_Prog); //Borra las paginas de ese programa.

#endif //SOCKETS_H_
