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
	int i =0;
	void* result=NULL;
	//Busco el proceso en todas las listas
	while(i<list_size(Estados) && result==NULL){
		t_list* lista = (t_list* )list_get(Estados,i);
		result = list_find(lista,LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pidAConsultar; }));
		i++;
	}
	if(result==NULL)
		printf("No se encontro el proceso a consultar. Intente nuevamente");
	else{
		BloqueControlProceso* proceso = (BloqueControlProceso*)result;
		printf("Proceso N°: %d \n",proceso->PID);
		printf("Paginas de codigo: %d \n",proceso->PaginasDeCodigo);
		printf("Contador de programa: %d \n",proceso->ProgramCounter);
		printf("Rafagas ejecutadas: %d \n",proceso->cantidadDeRafagasEjecutadas);
		printf("Syscall ejecutadas: %d \n",proceso->cantidadSyscallEjecutadas);

	}
}

void userInterfaceHandler(uint32_t* socketFD) {
	int end = 1;
	while (end) {
		char orden[100];
		int pidConsulta=0;
		int nuevoGradoMP=0;

		char lista[100];
		printf("\n\nIngrese una orden: \n");
		scanf("%s", orden);
		char* command = getWord(orden, 0);
		if(strcmp(command,"exit")==0)
				exit(1);
		else if (strcmp(command, "disconnect") == 0) {
				end = 0;
		}
		//Para cambiar el grado de multiprogramacion
		else if(strcmp(command,"cambiar_gradomp")==0){
			scanf("%d", &nuevoGradoMP);
			GRADO_MULTIPROG = nuevoGradoMP;
			printf("El nuevo grado de multiprogramacion es: %d\n",GRADO_MULTIPROG);
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
			scanf("%d", &pidConsulta);
			ConsultarEstado(pidConsulta);
		}
		else if (strcmp(command, "kill_program") == 0){
			scanf("%d", &pidConsulta);
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
		}
		else if (strcmp(command, "detener_planificacion") == 0){
			planificacion_detenida = true;
		}
		else if (strcmp(command, "reanudar_planificacion") == 0){
			planificacion_detenida = false;
		}
		else {
			printf("No se conoce el mensaje %s\n", orden);
		}
	}
}
