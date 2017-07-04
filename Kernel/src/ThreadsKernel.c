#include "ThreadsKernel.h"

BloqueControlProceso* removerPidDeListas(int pid, int* index)
{

	//Nuevos
	pthread_mutex_lock(&mutexQueueNuevos);
	BloqueControlProceso* pcb = (BloqueControlProceso*)list_remove_by_condition(Nuevos->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));

	if (pcb != NULL)
	{
		*index = INDEX_NUEVOS;
		pthread_mutex_unlock(&mutexQueueNuevos);
		return pcb;
	}
	pthread_mutex_unlock(&mutexQueueNuevos);

	//Ejecutando
	pthread_mutex_lock(&mutexQueueEjecutando);
	if(queue_size(Ejecutando)>0){
		pthread_mutex_unlock(&mutexQueueEjecutando);

		while(ProcesoEstaEjecutandoseActualmente(pid)==true);
		pthread_mutex_lock(&mutexQueueEjecutando);
		pcb = (BloqueControlProceso*)list_remove_by_condition(Ejecutando->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));
		if (pcb != NULL)
		{
			*index = INDEX_EJECUTANDO;
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
		*index = INDEX_BLOQUEADOS;
		pthread_mutex_unlock(&mutexQueueBloqueados);
		return pcb;
	}
	pthread_mutex_unlock(&mutexQueueBloqueados);
	//Listos
	pthread_mutex_lock(&mutexQueueListos);
	pcb = (BloqueControlProceso*)list_remove_by_condition(Listos->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));

	if (pcb != NULL)
	{
		*index = INDEX_LISTOS;
		pthread_mutex_unlock(&mutexQueueListos);
		Evento_ListosRemove();
		return pcb;
	}
	pthread_mutex_unlock(&mutexQueueListos);


	*index=-1;
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
	int i = 0;

	for(i=0;i<metaProgram->instrucciones_size; i++)
	{
		RegIndiceCodigo* registroIndice = malloc(sizeof(RegIndiceCodigo));

		registroIndice->start= metaProgram->instrucciones_serializado[i].start;
		registroIndice->offset= metaProgram->instrucciones_serializado[i].offset;

		list_add(pcb->IndiceDeCodigo,registroIndice);
	}
	//Inicializar indice de etiquetas
	i = 0;

	// Leer metadataprogram.c a ver como desarrollaron esto
	if(metaProgram->etiquetas_size>0){
		char **etiquetasEncontradas = string_n_split(metaProgram->etiquetas,metaProgram->cantidad_de_etiquetas+metaProgram->cantidad_de_funciones,(char*)metaProgram->instrucciones_size);
		string_iterate_lines(etiquetasEncontradas,
				LAMBDA(void _(char* item) {
			t_puntero_instruccion pointer = metadata_buscar_etiqueta(item,metaProgram->etiquetas, metaProgram->etiquetas_size);
			dictionary_put(pcb->IndiceDeEtiquetas, item, &pointer);

		}));
		pcb->cantidad_de_etiquetas = metaProgram->cantidad_de_etiquetas;
		pcb->cantidad_de_funciones = metaProgram->cantidad_de_funciones;
		pcb->etiquetas_size= metaProgram->etiquetas_size;
		strcpy(pcb->etiquetas,metaProgram->etiquetas);
	}

}


BloqueControlProceso* FinalizarPrograma(int PID, int tipoFinalizacion)
{
	BloqueControlProceso* pcbRemovido = NULL;
	bool hayEstructurasNoLiberadas = false;
	int index;
	pthread_mutex_lock(&mutexFinalizarPrograma);

	finalizarProgramaCapaFS(PID);

	//Aca hace la liberacion de memoria uri, fijate si podes hacer una funcion finalizarProgramaCapaMemoria, sino hace aca normal

	pcbRemovido = removerPidDeListas(PID, &index);

	if(pcbRemovido != NULL)
	{
		pcbRemovido->ExitCode = tipoFinalizacion;

		list_add(Finalizados->elements, pcbRemovido);

		//Analizo si el proceso tiene Memory Leaks o no
		t_list* pagesProcess = list_filter(PaginasPorProceso,
							LAMBDA(bool _(void* item) {return ((PaginaDelProceso*)item)->pid == PID;}));

		if(list_size(pagesProcess) > 0)
		{
			int i = 0;

			while(i < list_size(pagesProcess) && hayEstructurasNoLiberadas == false)
			{
				PaginaDelProceso* elem = (PaginaDelProceso*)list_get(PaginasPorProceso, i);

				//Me fijo si hay metadatas en estado "used" en cada pagina
				void* datosPagina = IM_LeerDatos(socketConMemoria, KERNEL, elem->pid, elem->nroPagina, 0, TamanioPagina);

				if(datosPagina != NULL)
				{
					int result = RecorrerHastaEncontrarUnMetadataUsed(datosPagina);

					if(result >= 0)
					{
						//Hay algun metadata que no se libero
						hayEstructurasNoLiberadas = true;
					}
				}

			}

			if(hayEstructurasNoLiberadas == true)
			{
				printf("MEMORY LEAKS: El proceso %d no liberó todas las estructuras de memoria dinámica que solicitó", PID);
			}
			else
			{
				printf("El proceso %d liberó todas las estructuras de memoria dinamica", PID);
			}
		}

		if(index == INDEX_LISTOS)
		{
			if(IM_FinalizarPrograma(socketConMemoria, KERNEL, PID) == false)
			{
				pcbRemovido->ExitCode = EXCEPCIONDEMEMORIA;
			}
		}
		free(pagesProcess);
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
/*
		EnviarDatosTipo(cpu->socketCPU, KERNEL, NULL,0, ESTAEJECUTANDO);
		Paquete* paquete2 = malloc(sizeof(Paquete));
		RecibirPaqueteCliente(cpu->socketCPU, KERNEL, paquete2);
		bool* estaEjecutando = ((bool*)paquete2->Payload)[0];
		uint32_t* PID = (uint32_t*)( paquete2->Payload+sizeof(bool));
*/
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

	Evento_ListosAdd();
	pthread_mutex_lock(&mutexQueueListos);
	queue_push(Listos, pcb);
	pthread_mutex_unlock(&mutexQueueListos);
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
			}

			EnviarHandshake(socketFD, receptor); // paquete->header.emisor
		}
		else
		{ //recibimos un payload y lo procesamos (por ej, puede mostrarlo)
			paquete->Payload = realloc(paquete->Payload, paquete->header.tamPayload);

			resul = RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
		}
	}

	return resul;
}
bool hayCPUsLibres(){
	return list_any_satisfy(CPUsConectadas, LAMBDA(bool _(void* item) { return ((DatosCPU*)item)->isFree == true;}));
}

pthread_mutex_t mutexDispacher = PTHREAD_MUTEX_INITIALIZER;

void dispatcher()
{
	while ( !planificacion_detenida)
	{
		if(queue_size(Listos)==0){
			//pthread_mutex_lock(&mutexDispacher);
		}
		else
		{
			pthread_mutex_lock(&mutexCPUsConectadas);
			t_list* listCPUsLibres = list_filter(CPUsConectadas, LAMBDA(bool _(void* item) { return ((DatosCPU*)item)->isFree == true;}));
			pthread_mutex_unlock(&mutexCPUsConectadas);
			int i;
			for (i = 0; i < list_size(listCPUsLibres); i++)
			{
				pthread_mutex_lock(&mutexQueueListos);
				BloqueControlProceso* PCBAMandar = (BloqueControlProceso*)queue_pop(Listos);
				pthread_mutex_unlock(&mutexQueueListos);
				if(PCBAMandar!=NULL){

					DatosCPU* cpuAUsar = (DatosCPU*)list_get(listCPUsLibres, 0);
					uint32_t cantidadDeRafagas;
					uint32_t cantidadDeRafagasRestantes = PCBAMandar->IndiceDeCodigo->elements_count - PCBAMandar->cantidadDeRafagasEjecutadasHistorica;

					if (strcmp(ALGORITMO, "FIFO") == 0)
					{
						cantidadDeRafagas = cantidadDeRafagasRestantes;
					}
					else if (strcmp(ALGORITMO, "RR") == 0)
					{
						if (cantidadDeRafagasRestantes < QUANTUM)
						{
							cantidadDeRafagas = cantidadDeRafagasRestantes;
						}
						else
						{
							cantidadDeRafagas = QUANTUM;
						}
					}
					PCBAMandar->cantidadDeRafagasAEjecutar = cantidadDeRafagas;
					PCBAMandar->cantidadDeRafagasEjecutadas = 0;

					EnviarPCB(cpuAUsar->socketCPU, KERNEL, PCBAMandar);

					cpuAUsar->isFree = false;
					cpuAUsar->pid = PCBAMandar->PID;
					pthread_mutex_lock(&mutexQueueEjecutando);
					queue_push(Ejecutando, PCBAMandar);
					pthread_mutex_unlock(&mutexQueueEjecutando);
					//cuando se hace el pcb_receive, cpuAUsar->isFree se cambia a true.
				}
			}
			list_destroy(listCPUsLibres);
		}
		//pthread_mutex_unlock(&mutexDispacher);
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
		if(paquete.Payload!=NULL) free(paquete.Payload);
	}

	close(socketConectado);

}
