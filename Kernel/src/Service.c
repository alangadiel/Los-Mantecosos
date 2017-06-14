#include "Helper.h"
#include "SocketsL.h"

void CrearListas()
{
	Nuevos = list_create();
	Finalizados= list_create();
	Bloqueados= list_create();
	Ejecutando= list_create();
	Listos= list_create();
	//Creo una lista de listas
	Estados = list_create();
	EstadosConProgramasFinalizables = list_create();
	ArchivosGlobales = list_create();
	VariablesGlobales = list_create();
	Semaforos = list_create();
	list_add(EstadosConProgramasFinalizables,Nuevos);
	list_add(EstadosConProgramasFinalizables,Listos);
	list_add(EstadosConProgramasFinalizables,Ejecutando);
	list_add(EstadosConProgramasFinalizables,Bloqueados);
	list_add_all(Estados,EstadosConProgramasFinalizables);
	list_add(Estados,Finalizados);
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
void LimpiarListas()
{
	list_destroy_and_destroy_elements(Nuevos,free);
	list_destroy_and_destroy_elements(Listos,free);
	list_destroy_and_destroy_elements(Ejecutando,free);
	list_destroy_and_destroy_elements(Bloqueados,free);
	list_destroy_and_destroy_elements(Finalizados,free);
	list_destroy_and_destroy_elements(Estados,free);
	list_destroy_and_destroy_elements(EstadosConProgramasFinalizables,free);
	list_destroy_and_destroy_elements(ArchivosGlobales,free);
	list_destroy_and_destroy_elements(ArchivosProcesos,free);
	list_destroy_and_destroy_elements(VariablesGlobales,free);
	list_destroy_and_destroy_elements(Semaforos,free);
}
void CrearNuevoProceso(BloqueControlProceso* pcb,int* ultimoPid,t_list* nuevos){
	//Creo el pcb y lo guardo en la lista de nuevos
	pcb->PID = *ultimoPid+1;
	//pcb->IndiceStack = 0;
	pcb->PaginasDeCodigo=0;
	pcb->ProgramCounter = 0;
	(*ultimoPid)++;
	list_add(nuevos,pcb);
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
