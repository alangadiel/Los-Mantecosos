#include "Service.h"

t_list* ArchivosGlobales;
t_list* ArchivosProcesos;
t_list* PIDsPorSocketConsola;
t_list* CPUsConectadas;
t_list* Semaforos;
//t_list* Paginas;
t_list* VariablesCompartidas;
t_list* PaginasPorProceso;
t_queue* Nuevos;
t_queue* Finalizados;
t_queue* Bloqueados;
t_queue* Ejecutando;
t_queue* Listos;
bool planificacion_detenida;

bool end;
uint32_t TamanioPagina;
int pidAFinalizar;

int socketConMemoria = -1;
int socketConFS = -1;

pthread_mutex_t mutexQueueNuevos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexQueueListos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexQueueEjecutando = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexQueueBloqueados = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexQueueFinalizados = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexFinalizarPrograma = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexQueuesProcesos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCPUsConectadas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSemaforos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexVariablesCompartidas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPaginasPorProceso = PTHREAD_MUTEX_INITIALIZER;

//Variables archivo de configuracion
char* IP_PROG;
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
char** SEM_IDS;
char** SEM_INIT;
char** SHARED_VARS;
int STACK_SIZE;

void CrearListas() {
	Nuevos = queue_create();
	Finalizados= queue_create();
	Bloqueados= queue_create();
	Ejecutando= queue_create();
	Listos= queue_create();

	ArchivosGlobales = list_create();
	ArchivosProcesos = list_create();
	VariablesCompartidas = list_create();
	PaginasPorProceso = list_create();
	Semaforos = list_create();
	PIDsPorSocketConsola = list_create();
	CPUsConectadas = list_create();
}
void LiberarVariablesYListas() {
	queue_destroy_and_destroy_elements(Nuevos,free);
	queue_destroy_and_destroy_elements(Listos,free);
	queue_destroy_and_destroy_elements(Ejecutando,free);
	queue_destroy_and_destroy_elements(Bloqueados,free);
	queue_destroy_and_destroy_elements(Finalizados,free);
	list_destroy_and_destroy_elements(ArchivosGlobales,free);
	list_destroy_and_destroy_elements(ArchivosProcesos,free);
	list_destroy_and_destroy_elements(VariablesCompartidas,free);
	list_destroy_and_destroy_elements(Semaforos,free);
	list_destroy_and_destroy_elements(PIDsPorSocketConsola, free);
	list_destroy_and_destroy_elements(CPUsConectadas, free);
	list_destroy_and_destroy_elements(PaginasPorProceso, free);

	free(IP_PROG);
	free(IP_MEMORIA);
	free(IP_FS);
	free(ALGORITMO);
	free(SEM_IDS);
	free(SEM_INIT);
	free(SHARED_VARS);
}

void CrearNuevoProceso(BloqueControlProceso* pcb,int* ultimoPid,t_queue* nuevos){
	//Creo el pcb y lo guardo en la lista de nuevos
	pcb_Create(pcb, *ultimoPid+1);
	(*ultimoPid)++;
	pcb->cantidadDeRafagasEjecutadasHistorica = 0;
	pthread_mutex_lock(&mutexQueueNuevos);
	queue_push(nuevos,pcb);
	pthread_mutex_unlock(&mutexQueueNuevos);
	//list_add(ListaPCB,pcb);
}

void obtenerError(int exitCode){
	switch(exitCode){
		case FINALIZACIONNORMAL:
			printf(" (El programa finalizó correctamente)\n");
		break;

		case NOSEPUDIERONRESERVARRECURSOS:
			printf(" (No se pudieron reservar recursos para ejecutar el programa)\n");
		break;

		case ACCEDERAARCHIVOINEXISTENTE:
			printf(" (El programa intentó acceder a un archivo que no existe)\n");
		break;

		case LEERARCHIVOSINPERMISOS:
			printf(" (El programa intentó leer un archivo sin permisos)\n");
		break;

		case ESCRIBIRARCHIVOSINPERMISOS:
			printf(" (El programa intentó escribir un archivo sin permisos)\n");
		break;

		case EXCEPCIONDEMEMORIA:
			printf(" (Excepción de memoria)\n");
		break;

		case DESCONEXIONDECONSOLA:
			printf(" (Finalizado a través de desconexión de consola)\n");
		break;

		case DESCONECTADODESDECOMANDOCONSOLA:
			printf(" (Finalizado a través del comando Finalizar Programa de la consola)\n");
		break;

		case SOLICITUDMASGRANDEQUETAMANIOPAGINA:
			printf(" (Se intentó reservar más memoria que el tamaño de una página)\n");
		break;

		case NOSEPUEDENASIGNARMASPAGINAS:
			printf(" (No se pueden asignar más páginas al proceso)\n");
		break;

		case ERRORSINDEFINIR:
			printf(" (Error sin definición)\n");
		break;
	}
}
