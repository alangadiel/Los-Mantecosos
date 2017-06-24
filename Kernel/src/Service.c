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
t_list* Estados;
t_list* EstadosConProgramasFinalizables;

bool end;
uint32_t TamanioPagina;
int pidAFinalizar;

int socketConMemoria;
int socketConFS;

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

int QUANTUM;
int QUANTUM_SLEEP;
char* ALGORITMO;
int GRADO_MULTIPROG;
char* SEM_IDS[4];
int SEM_INIT[100];
char* SHARED_VARS[100];
int STACK_SIZE;

char* ObtenerTextoDeArchivoSinCorchetes(FILE* f) //Para obtener los valores de los arrays del archivo de configuracion
{
	char buffer[10000];
	char *line = fgets(buffer,sizeof buffer,f);
	int length = string_length(line)-3;
	char *texto = string_substring(line,1,length);

	texto  = strtok(texto,",");

	return texto;
}



void imprimirArchivoConfiguracion()
{
	int c;
	FILE *file;

	file = fopen("ArchivoConfiguracion.txt", "r");

	if (file)
	{
		while ((c = getc(file)) != EOF)
		{
			putchar(c);
		}

		fclose(file);
	}
}

void CrearListas() {
	Nuevos = queue_create();
	Finalizados= queue_create();
	Bloqueados= queue_create();
	Ejecutando= queue_create();
	Listos= queue_create();
	//Creo una lista de listas
	Estados = list_create();
	EstadosConProgramasFinalizables = list_create();
	ArchivosGlobales = list_create();
	VariablesCompartidas = list_create();
	Semaforos = list_create();
	PIDsPorSocketConsola = list_create();
	CPUsConectadas = list_create();

	list_add(EstadosConProgramasFinalizables,Nuevos);
	list_add(EstadosConProgramasFinalizables,Listos);
	list_add(EstadosConProgramasFinalizables,Ejecutando);
	list_add(EstadosConProgramasFinalizables,Bloqueados);
	list_add_all(Estados,EstadosConProgramasFinalizables);
	list_add(Estados,Finalizados);
}
void LimpiarListas() {
	queue_destroy_and_destroy_elements(Nuevos,free);
	queue_destroy_and_destroy_elements(Listos,free);
	queue_destroy_and_destroy_elements(Ejecutando,free);
	queue_destroy_and_destroy_elements(Bloqueados,free);
	queue_destroy_and_destroy_elements(Finalizados,free);
	list_destroy_and_destroy_elements(Estados,free);
	list_destroy_and_destroy_elements(EstadosConProgramasFinalizables,free);
	list_destroy_and_destroy_elements(ArchivosGlobales,free);
	list_destroy_and_destroy_elements(ArchivosProcesos,free);
	list_destroy_and_destroy_elements(VariablesCompartidas,free);
	list_destroy_and_destroy_elements(Semaforos,free);
	list_destroy_and_destroy_elements(PIDsPorSocketConsola, free);
	list_destroy_and_destroy_elements(CPUsConectadas, free);
}

void CrearNuevoProceso(BloqueControlProceso* pcb,int* ultimoPid,t_queue* nuevos){
	//Creo el pcb y lo guardo en la lista de nuevos
	pcb_Create(pcb, *ultimoPid+1);
	(*ultimoPid)++;
	pthread_mutex_lock(&mutexQueueNuevos);
	queue_push(nuevos,pcb);
	pthread_mutex_unlock(&mutexQueueNuevos);s
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
