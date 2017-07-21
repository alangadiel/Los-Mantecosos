#include "ThreadsKernel.h"

BloqueControlProceso* removerPidDeListas(int pid, int* indice)
{

	//Nuevos
	pthread_mutex_lock(&mutexQueueNuevos);
	BloqueControlProceso* pcb = (BloqueControlProceso*)list_remove_by_condition(Nuevos->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));

	if (pcb != NULL)
	{
		*indice = INDEX_NUEVOS;
		pthread_mutex_unlock(&mutexQueueNuevos);
		return pcb;
	}
	pthread_mutex_unlock(&mutexQueueNuevos);

	//Ejecutando
	pthread_mutex_lock(&mutexQueueEjecutando);

	if(queue_size(Ejecutando)>0){
		while(ProcesoEstaEjecutandoseActualmente(pid)==true);
		pcb = (BloqueControlProceso*)list_remove_by_condition(Ejecutando->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));
		if (pcb != NULL)
		{
			*indice = INDEX_EJECUTANDO;
			sem_post(&semDispatcherCpus);
			pthread_mutex_unlock(&mutexQueueEjecutando);
			return pcb;
		}
	}
	pthread_mutex_unlock(&mutexQueueEjecutando);

	//Bloqueados
	pthread_mutex_lock(&mutexQueueBloqueados);
	pcb = (BloqueControlProceso*)list_remove_by_condition(Bloqueados->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));

	if (pcb != NULL)
	{
		*indice = INDEX_BLOQUEADOS;
		pthread_mutex_unlock(&mutexQueueBloqueados);
		return pcb;
	}
	pthread_mutex_unlock(&mutexQueueBloqueados);
	//Listos
	pthread_mutex_lock(&mutexQueueListos);
	pcb = (BloqueControlProceso*)list_remove_by_condition(Listos->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));

	if (pcb != NULL)
	{
		*indice = INDEX_LISTOS;
		pthread_mutex_unlock(&mutexQueueListos);
		//Evento_ListosRemove();
		return pcb;
	}
	pthread_mutex_unlock(&mutexQueueListos);


	*indice=-1;
	return NULL;
}


void GuardarCodigoDelProgramaEnLaMemoria(BloqueControlProceso* bcp, Paquete* paquete){
	int i = 0, j = 0;
	bool salioTodoBien = true;

	while(i < bcp->PaginasDeCodigo && salioTodoBien == true)
	{
		char* str = string_new();

		if(i+1 != bcp->PaginasDeCodigo)
		{
			str = string_substring((char*)paquete->Payload, j, TamanioPagina-1); //Si la pagina es de 512, iria de 0 a 511
			j += TamanioPagina;
		}
		else
		{
			//Ultima pagina de codigo
			 str = string_substring_from((char*)paquete->Payload, j);
		}

		salioTodoBien = IM_GuardarDatos(socketConMemoria, KERNEL, bcp->PID, i, 0, string_length(str), str);

		if(salioTodoBien == false)
		{
			FinalizarPrograma(bcp->PID, EXCEPCIONDEMEMORIA);
		}
		free(str);
		i++;
	}
}


void CargarInformacionDelCodigoDelPrograma(BloqueControlProceso* pcb,Paquete* paquete){
	t_metadata_program* metaProgram = metadata_desde_literal((char*)paquete->Payload);
	int i;

	for(i=0;i<metaProgram->instrucciones_size; i++)
	{
		RegIndiceCodigo* registroIndice = malloc(sizeof(RegIndiceCodigo));

		registroIndice->start= metaProgram->instrucciones_serializado[i].start;
		registroIndice->offset= metaProgram->instrucciones_serializado[i].offset;

		list_add(pcb->IndiceDeCodigo,registroIndice);
	}

	//if(metaProgram->etiquetas_size>0){
	pcb->cantidad_de_etiquetas = metaProgram->cantidad_de_etiquetas;
	pcb->cantidad_de_funciones = metaProgram->cantidad_de_funciones;
	pcb->etiquetas_size= metaProgram->etiquetas_size;
	pcb->IndiceDeEtiquetas = malloc(metaProgram->etiquetas_size);
	memcpy(pcb->IndiceDeEtiquetas,metaProgram->etiquetas, metaProgram->etiquetas_size);


}


BloqueControlProceso* FinalizarPrograma(int PID, int tipoFinalizacion)
{
	BloqueControlProceso* pcbRemovido = NULL;
	bool hayEstructurasNoLiberadas = false;
	int indice;
	pthread_mutex_lock(&mutexFinalizarPrograma);
	finalizarProgramaCapaFS(PID);
	printf("Pid a finalizar : %u\n",PID);
	obtenerError(tipoFinalizacion);
	pcbRemovido = removerPidDeListas(PID, &indice);
	if(pcbRemovido != NULL)
	{
		printf("\nFinalizando proceso %u \n", pcbRemovido->PID);
		pcbRemovido->ExitCode = tipoFinalizacion;
		//Tengo que sacar a ese proceso de la cola de procesos bloqueados en los semaforos
		int z;
		for(z=0;z< list_size(Semaforos);z++){
			Semaforo* sem = list_get(Semaforos,z);
			if(list_size(Semaforos)> 0){
				bool buscarProcesoBloqueadoEnSemaforo(void * item){
					uint32_t* pid=item;
					return *pid==pcbRemovido->PID;
				}
				list_remove_by_condition(sem->listaDeProcesos->elements,buscarProcesoBloqueadoEnSemaforo);
			}
		}

		list_add(Finalizados->elements, pcbRemovido);
		//Analizo si el proceso tiene Memory Leaks o no
		bool esDelPID(void* item) {return ((PaginaDelProceso*)item)->pid == PID;}
		t_list* pagesProcess = list_filter(PaginasPorProceso, esDelPID);
		printf("Cant. paginas del heap : %u\n",pagesProcess->elements_count);
		if(list_size(pagesProcess) > 0)
		{
			int i = 0;
			while(i < list_size(pagesProcess) && hayEstructurasNoLiberadas == false)
			{
				PaginaDelProceso* elem = list_get(pagesProcess, i);
				//Me fijo si hay metadatas en estado "used" en cada pagina
				void* datosPagina = IM_LeerDatos(socketConMemoria, KERNEL, elem->pid, elem->nroPagina, 0, TamanioPagina);
				if(datosPagina != NULL)
				{
					hayEstructurasNoLiberadas = RecorrerHastaEncontrarUnMetadataUsed(datosPagina);
				}
				i++;
			}
			if(hayEstructurasNoLiberadas == true)
			{
				printf("MEMORY LEAKS: El proceso %d no liberó todas las estructuras de memoria dinámica que solicitó\n", PID);
			}
			else
			{
				printf("El proceso %d liberó todas las estructuras de memoria dinamica. NO hay memory leaks.\n", PID);
			}
		}
		else{
			printf("El proceso %d nunca llegó a reservar bloques de memoria dinámica, por lo tanto, no hay memory leaks.\n",PID);
		}
		if(IM_FinalizarPrograma(socketConMemoria, KERNEL, PID) == false)
		{
			pcbRemovido->ExitCode = EXCEPCIONDEMEMORIA;
		}
		list_remove_by_condition(PaginasPorProceso, esDelPID);
		list_destroy_and_destroy_elements(pagesProcess,free);
	}
	/*else{
		if(index==INDEX_EJECUTANDO)
			pthread_mutex_unlock(&mutexFinalizarPrograma);
			FinalizarPrograma(PID,tipoFinalizacion);
	}*/
	pthread_mutex_unlock(&mutexFinalizarPrograma);
	return pcbRemovido;
}

bool ProcesoEstaEjecutandoseActualmente(int pidAFinalizar)
{
	int i;
	pthread_mutex_lock(&mutexCPUsConectadas);
	int sizeCpusConectadas = list_size(CPUsConectadas);
	for (i = 0; i < sizeCpusConectadas; i++)
	{
		DatosCPU* cpu = (DatosCPU*) list_get(CPUsConectadas, i);
		if (cpu->pid == pidAFinalizar && cpu->isFree == false)
		{
			pthread_mutex_unlock(&mutexCPUsConectadas);
			return true;
		}
	}
	pthread_mutex_unlock(&mutexCPUsConectadas);
	return false;
}


bool KillProgram(int pidAFinalizar, int tipoFinalizacion)
{
	void* result = NULL;
	result = FinalizarPrograma(pidAFinalizar, tipoFinalizacion);

	if(result == NULL)
	{
		printf("No se encontro el programa finalizar");

		return false;
	}

	return true;
}


void PonerElProgramaComoListo(BloqueControlProceso* pcb, Paquete* paquete, int socketFD, double tamanioTotalPaginas)
{
	printf("Cant paginas asignadas para el codigo: %d \n",pcb->PaginasDeCodigo);
	pthread_mutex_lock(&mutexQueueNuevos);
	//Saco el programa de la lista de NEW y  agrego el programa a la lista de READY
	list_remove_by_condition(Nuevos->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pcb->PID; }));

	pthread_mutex_unlock(&mutexQueueNuevos);

	pthread_mutex_lock(&mutexQueueListos);
	queue_push(Listos, pcb);
	pthread_mutex_unlock(&mutexQueueListos);

	Evento_ListosAdd();

	printf("El programa %d se cargo en memoria \n",pcb->PID);
}


int RecibirPaqueteServidorKernel(int socketFD, char receptor[11], Paquete* paquete)
{
	paquete->Payload = NULL;
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0)
	{ //si no hubo error
		if (paquete->header.tipoMensaje == ESHANDSHAKE)
		{ //vemos si es un handshake
			printf("Se establecio conexion con %s\n", paquete->header.emisor);
			if(strcmp(paquete->header.emisor, CPU) == 0)
			{
				DatosCPU* disp = malloc(sizeof(DatosCPU));
				disp->isFree = true;
				disp->socketCPU = socketFD;
				pthread_mutex_lock(&mutexCPUsConectadas);
				list_add(CPUsConectadas, disp);
				pthread_mutex_unlock(&mutexCPUsConectadas);
				sem_post(&semDispatcherCpus);

				Paquete paquete;
				paquete.header.tipoMensaje = ESHANDSHAKE;
				paquete.header.tamPayload = sizeof(uint32_t)*2;
				strcpy(paquete.header.emisor, KERNEL);
				void* data = alloca(paquete.header.tamPayload);
				((uint32_t*) data)[0] = STACK_SIZE;
				((uint32_t*) data)[1] = QUANTUM_SLEEP;
				paquete.Payload=data;
				EnviarPaquete(socketFD, &paquete);
			}
			else{
				EnviarHandshake(socketFD, receptor); // paquete->header.emisor
			}
		}
		else if (paquete->header.tamPayload > 0)
		{ //recibimos un payload y lo procesamos (por ej, puede mostrarlo)
			paquete->Payload = malloc(paquete->header.tamPayload);
			resul = RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
		}
	}
	return resul;
}

bool hayCPUsLibres(){
	return list_any_satisfy(CPUsConectadas, LAMBDA(bool _(void* item) { return ((DatosCPU*)item)->isFree == true;}));
}

void dispatcher()
{
	while (!planificacion_detenida)
	{
		sem_wait(&semDispacherListos);
		printf("\nPaso el semaforo DISPATCHER_LISTOS\n");
		sem_wait(&semDispatcherCpus);
		printf("\nPaso el semaforo DISPATCHER_CPUS\n");

		pthread_mutex_lock(&mutexQueueListos);
		BloqueControlProceso* PCBAMandar = (BloqueControlProceso*)queue_pop(Listos);
		pthread_mutex_unlock(&mutexQueueListos);
		if(PCBAMandar!=NULL)
		{
			pthread_mutex_lock(&mutexCPUsConectadas);
			DatosCPU* cpuAUsar = (DatosCPU*) list_find(CPUsConectadas, LAMBDA(bool _(void* item) { return ((DatosCPU*)item)->isFree == true;}));
			pthread_mutex_unlock(&mutexCPUsConectadas);
			if (strcmp(ALGORITMO, "FIFO") == 0)
			{
				PCBAMandar->cantidadDeRafagasAEjecutar = 0;//sin limite
			}
			else if (strcmp(ALGORITMO, "RR") == 0)
			{
				PCBAMandar->cantidadDeRafagasAEjecutar = QUANTUM;
			}

			PCBAMandar->cantidadDeRafagasEjecutadas = 0;
			printf("Despachando proceso %u por socket %i\n", PCBAMandar->PID, cpuAUsar->socketCPU);
			EnviarPCB(cpuAUsar->socketCPU, KERNEL, PCBAMandar,ESPCB);

			cpuAUsar->isFree = false;
			cpuAUsar->pid = PCBAMandar->PID;

			pthread_mutex_lock(&mutexQueueEjecutando);
			BloqueControlProceso* bcp = list_find(Ejecutando->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == PCBAMandar->PID;}));
			if(bcp == NULL)
				queue_push(Ejecutando, PCBAMandar);
			pthread_mutex_unlock(&mutexQueueEjecutando);
			//cuando se hace el pcb_receive, cpuAUsar->isFree se cambia a true.

		}
		else{
			sem_post(&semDispatcherCpus);
		}
	}
}


void AgregarAListadePidsPorSocket(uint32_t PID, int socket)
{
	PIDporSocketConsola* PIDxSocket = malloc(sizeof(uint32_t));

	PIDxSocket->PID = PID;
	PIDxSocket->socketConsola = socket;

	list_add(PIDsPorSocketConsola, PIDxSocket);
}


void accion(void* socket)
{
	int socketConectado = *(int*)socket;
	Paquete paquete;

	while (RecibirPaqueteServidorKernel(socketConectado, KERNEL, &paquete) > 0)
	{
		receptorKernel(&paquete, socketConectado);
		if(paquete.Payload!=NULL)
			free(paquete.Payload);
	}
	close(socketConectado);
}
