// TIPO DE DATO DE LOS BUFFER
#ifndef CONEX_H_
#define CONEX_H_


#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
typedef int socket_t;

typedef unsigned char* buffer_t;

/* Estructuras NIPC */

typedef struct {
	int length;
	char* data;
} t_stream ;



typedef struct t_rutina {
	char* archivos;
	char* map;
	char* reduce;
	char* resultado;
	char* combiner;
} __attribute__((__packed__)) t_rutina;

typedef struct mensajeOperacion {
	char* nodo;
	char* ip;
	char* puerto;
	char* bloque;
	char* resultado;
} __attribute__((__packed__)) mensajeOperacion;

typedef struct t_resultadoMapper {
	char* nodo;
	char* bloque;
} __attribute__((__packed__)) t_resultadoMapper;

typedef struct t_out {
	char* nodo;
} __attribute__((__packed__)) t_out;

typedef struct _MPS_MSG
{
 int8_t PayloadDescriptor;
 int16_t PayLoadLength;
 void *Payload;
} __attribute__((__packed__)) MPS_MSG;


typedef struct _MPS_MSG_HEADER
{
   int8_t PayloadDescriptor;
   int16_t PayLoadLength;
} __attribute__((__packed__)) MPS_MSG_HEADER;

socket_t iniciarServidor(int puerto);
socket_t conectarAlServidor(char *IPServidor, int PuertoServidor);
int enviarMensaje(socket_t Socket, MPS_MSG *mensaje);
int recibirMensaje(int Socket, MPS_MSG *mensaje);
int recibirInfo(int Socket, buffer_t Buffer, int CantidadARecibir);
int enviarInfo(int Socket, buffer_t Buffer, int CantidadAEnviar);
t_stream* operacion_serializer(mensajeOperacion *self);
mensajeOperacion* operacion_desserializer(char *stream);
void armarMensaje(MPS_MSG* mensaje,int descriptor,int payloadLength,void* payload);
char* obtenerIpLocal();

t_rutina* rutina_desserializer(char *stream);
t_resultadoMapper* resultadoMapper_desserializer(char *stream);
t_out* out_desserializer(char* stream);

// Inicia el servidor y devuelve el puerto asignado aleatoreamente.
int realizarConexion(int socketEscucha);

#endif /* CONEX_H_ */
