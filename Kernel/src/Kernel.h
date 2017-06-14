/*
 * Kernel.h
 *
 *  Created on: 13/6/2017
 *      Author: utnso
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include "CapaFS.h"
#include "SocketsL.h"
#include "Helper.h"
#include "Service.h"


//Tipos de ExitCode
#define FINALIZACIONNORMAL
#define NOSEPUDIERONRESERVARRECURSOS -1
#define DESCONECTADODESDECOMANDOCONSOLA -7
#define SOLICITUDMASGRANDEQUETAMANIOPAGINA -8

#define SIZEMETADATA 5

typedef struct  {
 uint32_t size;
 bool isFree;
} HeapMetadata;

typedef struct {
	FILE * file;
	BloqueControlProceso pcb;
} PeticionTamanioStack;

typedef struct {
	uint32_t pid;
	uint32_t nroPagina; 		//Numero de pagina DEL PROCESO
	uint32_t espacioDisponible;
} PaginaDelProceso;

typedef struct {
	uint32_t numero;
	t_list* HeapsMetadata;
} Pagina;

typedef struct {
	uint32_t PID;
	uint32_t FD; //Empieza a partir de FD = 3. El 0, 1 y 2 estan reservados por convencion.
	uint32_t offsetArchivo;
	char* flag;
	uint32_t globalFD;
} archivoProceso;

typedef struct {
	char* pathArchivo;
	uint32_t cantAperturas;
} archivoGlobal;

typedef struct {
	char* nombreVariableGlobal;
	uint32_t valorVariableGlobal;
} variableGlobal;

typedef struct {
	char* nombreSemaforo;
	uint32_t valorSemaforo;
} semaforo;


int pidAFinalizar;
int ultimoPID=0;
int socketConMemoria;
int socketConFS;
int ultimoFD = 3;
double TamanioPagina;




//Variables archivo de configuracion
int PUERTO_PROG;
int PUERTO_CPU;
char* IP_MEMORIA;
int PUERTO_MEMORIA;
char* IP_FS;
int PUERTO_FS;
int QUANTUM;
int QUANTUM_SLEEP;
char* ALGORITMO;
int GRADO_MULTIPROG;
char* SEM_IDS[4];
int SEM_INIT[100];
char* SHARED_VARS[100];
int STACK_SIZE;
char* IP_PROG;
uint32_t PidAComparar;
t_list* Nuevos;
t_list* Finalizados;
t_list* Bloqueados;
t_list* Ejecutando;
t_list* Listos;
t_list* Estados;
t_list* ListaPCB;
t_list* EstadosConProgramasFinalizables;
t_list* PaginasPorProceso;
t_list* Paginas;
extern t_list* ArchivosGlobales;
t_list* ArchivosProcesos;
t_list* VariablesGlobales;
t_list* Semaforos;


int RecorrerHastaEncontrarUnMetadataUsed(void* datosPagina);
BloqueControlProceso* FinalizarPrograma(t_list* lista,int pid,int tipoFinalizacion, int index, int socketFD) ;
uint32_t ActualizarMetadata(uint32_t PID, uint32_t nroPagina, uint32_t cantAReservar, int socketFD);
uint32_t SolicitarHeap(uint32_t PID,uint32_t cantAReservar,int socket);
void SolicitudLiberacionDeBloque(int socketFD,uint32_t pid,PosicionDeMemoria pos);
char* ObtenerTextoDeArchivoSinCorchetes(FILE* f);
void ObtenerTamanioPagina(int socketFD);
void MostrarProcesosDeUnaLista(t_list* lista,char* discriminator);
void obtenerValoresArchivoConfiguracion();
void imprimirArchivoConfiguracion();
void CrearListas();
void LimpiarListas();
void ConsultarEstado(int pidAConsultar);
void syscallWrite(int socketFD);
void MostrarTodosLosProcesos();
void KillProgramDeUnaLista(t_list* lista,BloqueControlProceso* pcb,int tipoFinalizacion, int index, int socket);
bool KillProgram(int pidAFinalizar,int tipoFinalizacion, int socket);
void PonerElProgramaComoListo(BloqueControlProceso* pcb,Paquete* paquete,int socketFD,double tamanioTotalPaginas);
void accion(Paquete* paquete, int socketConectado);
void RecibirHandshake_KernelDeMemoria(int socketFD, char emisor[11]);
void userInterfaceHandler(uint32_t* socketFD);
int main(void);

#endif /* KERNEL_H_ */
