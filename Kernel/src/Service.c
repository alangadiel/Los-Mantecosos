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
t_list* ListaPCB;
t_list* EstadosConProgramasFinalizables;

bool end;
uint32_t TamanioPagina;
int pidAFinalizar;

int socketConMemoria;
int socketConFS;

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

void CrearNuevoProceso(BloqueControlProceso* pcb,int* ultimoPid,t_list* nuevos){
	//Creo el pcb y lo guardo en la lista de nuevos
	pcb->PID = *ultimoPid+1;
	//pcb->IndiceStack = 0;
	pcb->PaginasDeCodigo=0;
	pcb->ProgramCounter = 0;
	(*ultimoPid)++;
	queue_push(nuevos,pcb);
	//list_add(ListaPCB,pcb);
}

void obtenerError(int exitCode){
	switch(exitCode){
		case 0:
			printf(" (El programa finalizó correctamente)\n");
		break;

		case -1:
			printf(" (No se pudieron reservar recursos para ejecutar el programa)\n");
		break;

		case -2:
			printf(" (El programa intentó acceder a un archivo que no existe)\n");
		break;

		case -3:
			printf(" (El programa intentó leer un archivo sin permisos)\n");
		break;

		case -4:
			printf(" (El programa intentó escribir un archivo sin permisos)\n");
		break;

		case -5:
			printf(" (Excepción de memoria)\n");
		break;

		case -6:
			printf(" (Finalizado a través de desconexión de consola)\n");
		break;

		case -7:
			printf(" (Finalizado a través del comando Finalizar Programa de la consola)\n");
		break;

		case -8:
			printf(" (Se intentó reservar más memoria que el tamaño de una página)\n");
		break;

		case -9:
			printf(" (No se pueden asignar más páginas al proceso)\n");
		break;

		case -20:
			printf(" (Error sin definición)\n");
		break;
	}
}
