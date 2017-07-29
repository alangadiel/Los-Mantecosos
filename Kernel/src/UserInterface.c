#include "UserInterface.h"


void MostrarProcesosDeUnaCola(t_queue* cola,char* discriminator)
{
	printf("Procesos de la cola: %s \n",discriminator);
	int index=0;
	for (index = 0; index < list_size(cola->elements); index++) {
		BloqueControlProceso* proceso = (BloqueControlProceso*)list_get(cola->elements,index);

		if (strcmp(discriminator, FINALIZADOS) == 0) {
			printf("\tProceso N°: %d",proceso->PID);
			obtenerError(proceso->ExitCode);
		}
		else {
			printf("\tProceso N°: %d\n",proceso->PID);
		}
	}
}

void MostrarTodosLosProcesos()
{
	pthread_mutex_lock(&mutexQueueNuevos);
	MostrarProcesosDeUnaCola(Nuevos,NUEVOS);
	pthread_mutex_unlock(&mutexQueueNuevos);
	pthread_mutex_lock(&mutexQueueListos);
	MostrarProcesosDeUnaCola(Listos,LISTOS);
	pthread_mutex_unlock(&mutexQueueListos);
	pthread_mutex_lock(&mutexQueueEjecutando);
	MostrarProcesosDeUnaCola(Ejecutando,EJECUTANDO);
	pthread_mutex_unlock(&mutexQueueEjecutando);
	pthread_mutex_lock(&mutexQueueBloqueados);
	MostrarProcesosDeUnaCola(Bloqueados,BLOQUEADOS);
	pthread_mutex_unlock(&mutexQueueBloqueados);
	pthread_mutex_lock(&mutexQueueFinalizados);
	MostrarProcesosDeUnaCola(Finalizados,FINALIZADOS);
	pthread_mutex_unlock(&mutexQueueFinalizados);
}

void ConsultarEstado(int pidAConsultar)
{
	BloqueControlProceso* result=NULL;
	bool termino = false;
	//Busco el proceso en todas las listas
	pthread_mutex_lock(&mutexQueueNuevos);
	result = list_find(Nuevos->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pidAConsultar; }));
	pthread_mutex_unlock(&mutexQueueNuevos);
	if(result==NULL) {
		pthread_mutex_lock(&mutexQueueListos);
		result = list_find(Listos->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pidAConsultar; }));
		pthread_mutex_unlock(&mutexQueueListos);
		if(result==NULL) {
			pthread_mutex_lock(&mutexQueueEjecutando);
			result = list_find(Ejecutando->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pidAConsultar; }));
			pthread_mutex_unlock(&mutexQueueEjecutando);
			if(result==NULL) {
				pthread_mutex_lock(&mutexQueueBloqueados);
				result = list_find(Bloqueados->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pidAConsultar; }));
				pthread_mutex_unlock(&mutexQueueBloqueados);
				if(result==NULL) {
					pthread_mutex_lock(&mutexQueueFinalizados);
					result = list_find(Finalizados->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pidAConsultar; }));
					pthread_mutex_unlock(&mutexQueueFinalizados);
					termino = true;
				}
			}
		}
	}

	if(result==NULL)
		printf("No se encontro el proceso a consultar. Intente nuevamente");
	else{

		printf("Proceso N°: %u \n",result->PID);
		printf("Paginas de codigo: %u \n",result->PaginasDeCodigo);
		printf("Contador de programa: %u \n",result->ProgramCounter);
		printf("Rafagas ejecutadas: %u \n",result->cantidadDeRafagasEjecutadasHistorica);
		printf("Syscall ejecutadas: %u \n",result->cantidadSyscallEjecutadas);
		if(termino) obtenerError(result->ExitCode);
	}
}

void userInterfaceHandler(uint32_t* socketFD) {
	int end = 1;
	while (end) {
		char* command = malloc(50);
		int pidConsulta=0;
		int nuevoGradoMP=0;

		char lista[100];
		printf("\nIngrese una orden: \n");
		scanf("%s", command);
		if(strcmp(command,"exit")==0)
				exit(1);
		else if (strcmp(command, "disconnect") == 0) {
				end = 0;
		}
		//Para cambiar el grado de multiprogramacion
		else if(strcmp(command,"cambiar_gradomp")==0){
			command[0] = '\0'; //lo vacio
			scanf("%s", command);
			nuevoGradoMP = strtol(command, NULL, 10);
			if (nuevoGradoMP != 0 && nuevoGradoMP <= (2^32)){
				GRADO_MULTIPROG = nuevoGradoMP;
				printf("El nuevo grado de multiprogramacion es: %d\n",GRADO_MULTIPROG);
			} else
				printf("Numero invalido\n");
		}
		else if (strcmp(command, "mostrar_procesos") == 0) {
			scanf("%s", lista);
			if(strcmp(lista,NUEVOS)==0)
				MostrarProcesosDeUnaCola(Nuevos,NUEVOS);
			else if(strcmp(lista,LISTOS)==0)
				MostrarProcesosDeUnaCola(Listos,LISTOS);
			else if(strcmp(lista,EJECUTANDO)==0)
				MostrarProcesosDeUnaCola(Ejecutando,EJECUTANDO);
			else if(strcmp(lista,BLOQUEADOS)==0)
				MostrarProcesosDeUnaCola(Bloqueados,BLOQUEADOS);
			else if(strcmp(lista,FINALIZADOS)==0)
				MostrarProcesosDeUnaCola(Finalizados,FINALIZADOS);

			else if(strcmp(lista,"TODAS")==0)
				MostrarTodosLosProcesos();
			else
				printf("No se reconoce la lista ingresada");
			}

		else if (strcmp(command, "consultar_programa") == 0) {
			command[0] = '\0'; //lo vacio
			scanf("%s", command);
			pidConsulta = strtol(command, NULL, 10);
			if (pidConsulta != 0 && pidConsulta <= (2^32)){
				ConsultarEstado(pidConsulta);
			} else
				printf("Numero invalido\n");
		}
		else if (strcmp(command, "kill_program") == 0){
			command[0] = '\0'; //lo vacio
			scanf("%s", command);
			pidConsulta = strtol(command, NULL, 10);
			if (pidConsulta != 0 && pidConsulta <= (2^32)){
				bool finalizadoOK = KillProgram(pidConsulta, DESCONECTADODESDECOMANDOCONSOLA);
				if(finalizadoOK == true)
				{
					printf("El programa %d fue finalizado\n", pidAFinalizar);
					EnviarMensaje(*socketFD,"KILLEADO",KERNEL);
				}
				else
				{
					printf("Error al finalizar programa %d\n", pidAFinalizar);
					EnviarMensaje(*socketFD, "Error al finalizar programa", KERNEL);
				}
			} else
				printf("Numero invalido\n");
		}
		else if (strcmp(command, "detener_planificacion") == 0){
			planificacion_detenida = true;
		}
		else if (strcmp(command, "reanudar_planificacion") == 0){
			planificacion_detenida = false;
		}
		else if (strcmp(command, "mostrar_tabla_de_archivos_de_proceso") == 0) {
			command[0] = '\0'; //lo vacio
			scanf("%s", command);
			pidConsulta = strtol(command, NULL, 10);
			if (pidConsulta != 0 && pidConsulta <= (2^32)){
				ListaArchivosProceso* tablaProceso = obtenerTablaArchivosDeUnProceso(pidConsulta);

				if(tablaProceso != NULL)
				{
					int i;

					printf("\nProceso: %d\n", pidConsulta);

					for(i = 0; i < list_size(tablaProceso->listaArchivo); i++)
					{
						archivoProceso* archProceso = list_get(tablaProceso->listaArchivo, i);

						printf("FD: %d\n", archProceso->FD);
						printf("Flag Creacion: %d\n", archProceso->flags.creacion);
						printf("Flag Escritura: %d\n", archProceso->flags.escritura);
						printf("Flag Lectura: %d\n", archProceso->flags.lectura);
						printf("FD Global: %d\n", archProceso->globalFD);
						printf("Offset del archivo: %d\n", archProceso->offsetArchivo);
					}
				}
				else
					printf("El proceso %d no tiene archivos abiertos\n", pidConsulta);
			} else
				printf("Numero invalido\n");
		}
		else if (strcmp(command, "mostrar_tabla_de_archivos_global") == 0) {
			t_list* tablaGlobal = obtenerTablaArchivosGlobales();

			if(list_size(tablaGlobal) != 0)
			{
				int i;

				for(i = 0; i < list_size(tablaGlobal); i++)
				{
					archivoGlobal* archGlobal = list_get(tablaGlobal, i);

					printf("\nPath: %s\n", archGlobal->pathArchivo);
					printf("Cantidad de aperturas: %d\n", archGlobal->cantAperturas);
				}
			}
			else
			{
				printf("No hay una tabla global de archivos creada\n");
			}
		}
		else {
			printf("No se conoce el mensaje %s\n", command);
		}
	}
}
