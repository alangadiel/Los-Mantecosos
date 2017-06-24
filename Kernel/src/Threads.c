#include "Threads.h"

//Tipos de ExitCode
#define FINALIZACIONNORMAL 0
#define NOSEPUDIERONRESERVARRECURSOS -1
#define DESCONECTADODESDECOMANDOCONSOLA -7
#define SOLICITUDMASGRANDEQUETAMANIOPAGINA -8


int ultimoPID = 0;


BloqueControlProceso* removerPidDeListas(int pid, int* index)
{
	pthread_mutex_lock(&mutexQueueNuevos);
	BloqueControlProceso* pcb = (BloqueControlProceso*)list_remove_by_condition(Nuevos->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));

	if (pcb != NULL)
	{
		*index = INDEX_NUEVOS;

		return pcb;
	}
	pthread_mutex_unlock(&mutexQueueNuevos);

	pthread_mutex_lock(&mutexQueueEjecutando);
	if (ProcesoNoEstaEjecutandoseActualmente(pid)){
		pcb = (BloqueControlProceso*)list_remove_by_condition(Ejecutando->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));

		if (pcb != NULL)
		{
			*index = INDEX_EJECUTANDO;
			return pcb;
		}
	}
	else{
		*index = INDEX_EJECUTANDO;
		return NULL;
	}
	pthread_mutex_unlock(&mutexQueueEjecutando);


	pthread_mutex_lock(&mutexQueueBloqueados);
	pcb = (BloqueControlProceso*)list_remove_by_condition(Bloqueados->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));

	if (pcb != NULL)
	{
		*index = INDEX_BLOQUEADOS;

		return pcb;
	}
	pthread_mutex_unlock(&mutexQueueBloqueados);

	pthread_mutex_lock(&mutexQueueListos);
	pcb = (BloqueControlProceso*)list_remove_by_condition(Listos->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));

	if (pcb != NULL)
	{
		*index = INDEX_LISTOS;

		return pcb;
	}
	pthread_mutex_unlock(&mutexQueueListos);


	*index=-1;
	return NULL;
}


void GuardarCodigoDelProgramaEnLaMemoria(BloqueControlProceso* bcp, Paquete* paquete){
	int i = 0, j = 0;
	bool salioTodoBien = true;

	while(i < bcp->PaginasDeCodigo && salioTodoBien == false)
	{
		char* str;

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
			FinalizarPrograma(bcp->PID, EXCEPCIONDEMEMORIA, socketConMemoria);
		}
	}
}


void CargarInformacionDelCodigoDelPrograma(BloqueControlProceso* pcb,Paquete* paquete){
	t_metadata_program* metaProgram = metadata_desde_literal((char*)paquete->Payload);
	int i = 0;

	while(i<string_length(metaProgram->etiquetas))
	{
		int *registroIndice = malloc(sizeof(uint32_t)*2);

		registroIndice[0]= metaProgram->instrucciones_serializado[i].start;
		registroIndice[1]= metaProgram->instrucciones_serializado[i].offset;

		list_add(pcb->IndiceDeCodigo,registroIndice);

		i++;
	}

	//Inicializar indice de etiquetas
	i = 0;

	// Leer metadataprogram.c a ver como desarrollaron esto
	char **etiquetasEncontradas = string_n_split(metaProgram->etiquetas,metaProgram->cantidad_de_etiquetas+metaProgram->cantidad_de_funciones,(char*)metaProgram->instrucciones_size);
	string_iterate_lines(etiquetasEncontradas,
			LAMBDA(void _(char* item) {
		t_puntero_instruccion pointer = metadata_buscar_etiqueta(item,metaProgram->etiquetas, metaProgram->etiquetas_size);
		dictionary_put(pcb->IndiceDeEtiquetas, item, &pointer);

	}));
}


BloqueControlProceso* FinalizarPrograma(int PID, int tipoFinalizacion, int socketFD)
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
	}
	else{
		if(index==INDEX_EJECUTANDO)
			FinalizarPrograma(PID,tipoFinalizacion,socketFD);
		else if(index==-1){
			pthread_mutex_unlock(&mutexFinalizarPrograma);
			return pcbRemovido;
		}
	}
	pthread_mutex_unlock(&mutexFinalizarPrograma);
}


bool ProcesoNoEstaEjecutandoseActualmente(int pidAFinalizar)
{
	int i;
	pthread_mutex_lock(&mutexCPUsConectadas);
	for (i = 0; i < list_size(CPUsConectadas); i++)
	{
		DatosCPU* cpu = (DatosCPU*) list_get(CPUsConectadas, i);
		Paquete* paquete = malloc(sizeof(Paquete));

		paquete->header.tipoMensaje = ESTAEJECUTANDO;

		EnviarPaquete(cpu->socketCPU, paquete);

		free(paquete);

		Paquete* paquete2 = malloc(sizeof(Paquete));

		RecibirPaqueteCliente(cpu->socketCPU, KERNEL, paquete2);

		uint32_t terminoDeEjecutar = ((uint32_t*) paquete2->Payload)[0];
		uint32_t PID = ((uint32_t*) paquete2->Payload)[1];

		if (PID == pidAFinalizar && terminoDeEjecutar == 1)
		{
			return true;
		}
	}
	pthread_mutex_unlock(&mutexCPUsConectadas);
	return false;
}


bool KillProgram(int pidAFinalizar, int tipoFinalizacion, int socket)
{
	void* result = NULL;
	result = FinalizarPrograma(pidAFinalizar, tipoFinalizacion, socket);

	if(result == NULL)
	{
		printf("No se encontro el programa finalizar");

		return false;
	}

	return true;
}


void PonerElProgramaComoListo(BloqueControlProceso* pcb, Paquete* paquete, int socketFD, double tamanioTotalPaginas)
{
	pcb->PaginasDeCodigo = (uint32_t)tamanioTotalPaginas;
	printf("Cant paginas asignadas para el codigo: %d \n",pcb->PaginasDeCodigo);
	pthread_mutex_lock(&mutexQueueNuevos);
	//Saco el programa de la lista de NEW y  agrego el programa a la lista de READY
	list_remove_by_condition((t_list*)Nuevos, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pcb->PID; }));
	pthread_mutex_unlock(&mutexQueueNuevos);
	pthread_mutex_lock(&mutexQueueListos);
	queue_push(Listos, pcb);
	pthread_mutex_unlock(&mutexQueueListos);
	printf("El programa %d se cargo en memoria \n",pcb->PID);
}


int RecibirPaqueteServidorKernel(int socketFD, char receptor[11], Paquete* paquete)
{
	paquete->Payload = malloc(1);

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


void dispatcher()
{
	while (true && !planificacion_detenida)
	{
		pthread_mutex_lock(&mutexCPUsConectadas);
		t_list* listCPUsLibres = list_filter(CPUsConectadas, LAMBDA(bool _(void* item) { return ((DatosCPU*)item)->isFree == true;}));
		pthread_mutex_unlock(&mutexCPUsConectadas);
		int i;

		for (i = 0; i < list_size(listCPUsLibres); i++)
		{
			BloqueControlProceso* PCBAMandar = (BloqueControlProceso*)queue_pop(Listos);
			DatosCPU* cpuAUsar = (DatosCPU*)list_get(listCPUsLibres, 0);

			uint32_t cantidadDeRafagas;
			uint32_t cantidadDeRafagasRestantes = PCBAMandar->IndiceDeCodigo->elements_count - PCBAMandar->cantidadDeRafagasEjecutadas;

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
			pthread_mutex_lock(&mutexQueueEjecutando);
			queue_push(Ejecutando, PCBAMandar);
			pthread_mutex_unlock(&mutexQueueEjecutando);
			//cuando se hace el pcb_receive, cpuAUsar->isFree se cambia a true.
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
		switch(paquete.header.tipoMensaje)
		{
				case ESSTRING:
				if(strcmp(paquete.header.emisor, CONSOLA) == 0)
				{
					double tamaniCodigoEnPaginas = paquete.header.tamPayload / TamanioPagina;
					double tamanioCodigoYStackEnPaginas = ceil(tamaniCodigoEnPaginas) + STACK_SIZE;

					BloqueControlProceso* pcb = malloc(sizeof(BloqueControlProceso));
					//TODO: Falta free, pero OJO, hay que ver la forma de ponerlo y que siga andando
					CrearNuevoProceso(pcb,&ultimoPID,Nuevos);

					AgregarAListadePidsPorSocket(pcb->PID, socketConectado);

					//Manejo la multiprogramacion
					if(GRADO_MULTIPROG - queue_size(Ejecutando) - queue_size(Listos) > 0 && queue_size(Nuevos) >= 1)
					{
						//Pregunta a la memoria si me puede guardar estas paginas
						bool pudoCargarse = IM_InicializarPrograma(socketConMemoria, KERNEL, pcb->PID, tamanioCodigoYStackEnPaginas);

						if(pudoCargarse == true)
						{
							PonerElProgramaComoListo(pcb,&paquete, socketConectado, tamaniCodigoEnPaginas);
							//Ejecuto el metadata program
							CargarInformacionDelCodigoDelPrograma(pcb, &paquete);

							//Solicito a la memoria que me guarde el codigo del programa(dependiendo cuantas paginas se requiere para el codigo
							GuardarCodigoDelProgramaEnLaMemoria(pcb, &paquete);
							EnviarDatos(socketConectado, KERNEL, &(pcb->PID), sizeof(uint32_t));
						}
						else
						{
							//Sacar programa de la lista de nuevos y meterlo en la lista de finalizado con su respectivo codigo de error
							FinalizarPrograma(pcb->PID,NOSEPUDIERONRESERVARRECURSOS, socketConMemoria);
							EnviarMensaje(socketConectado, "No se pudo guardar el programa porque no hay espacio suficiente", KERNEL);
						}
					}
					else
					{
						//El grado de multiprogramacion no te deja agregar otro proceso
						FinalizarPrograma(pcb->PID,NOSEPUDIERONRESERVARRECURSOS, socketConMemoria);
						EnviarMensaje(socketConectado, "No se pudo guardar el programa porque se alcanzo el grado de multiprogramacion", KERNEL);
					}
				}
			break;

			case ESDATOS:
				if(strcmp(paquete.header.emisor, CPU) == 0)
				{
					switch ((*(uint32_t*)paquete.Payload))
					{
						int32_t valorAAsignar;
						char* variableCompartida;
						uint32_t PID;
						uint32_t FD;
						uint32_t tamanioArchivo;
						void* result;
						void* datos;
						VariableCompartida var;
						int tamDatos;

						char* nombreSem;
						uint32_t tamanioAReservar;

						case PEDIRSHAREDVAR:
							PID = ((uint32_t*)paquete.Payload)[1];
							strcpy(variableCompartida, (char*)(paquete.Payload + sizeof(uint32_t) * 2));

							//Busco la variable compartida
							result = NULL;
							pthread_mutex_lock(&mutexVariablesCompartidas);
							result = list_find(VariablesCompartidas,
								LAMBDA(bool _(void* item) {
									return strcmp(((VariableCompartida*)item)->nombreVariableGlobal, variableCompartida) == 0;
							}));
							pthread_mutex_unlock(&mutexVariablesCompartidas);
							 var = *(VariableCompartida*)result;

							//Devuelvo a la cpu el valor de la variable compartida
							tamDatos = sizeof(uint32_t) * 2;
							datos = malloc(tamDatos);

							((uint32_t*) datos)[0] = PEDIRSHAREDVAR;
							((int32_t*) datos)[1] = var.valorVariableGlobal;

							EnviarDatos(socketConectado, KERNEL, datos, tamDatos);

							free(datos);
						break;

						case ASIGNARSHAREDVAR:
							PID = ((uint32_t*)paquete.Payload)[1];
							valorAAsignar = ((int32_t*)paquete.Payload)[2];
							strcpy(variableCompartida, (char*)(paquete.Payload + sizeof(uint32_t) * 3));

							//Busco la variable compartida
							result = NULL;
							pthread_mutex_lock(&mutexVariablesCompartidas);
							result = list_find(VariablesCompartidas,
								LAMBDA(bool _(void* item) {
									return strcmp(((VariableCompartida*)item)->nombreVariableGlobal, variableCompartida) == 0; }));
							pthread_mutex_unlock(&mutexVariablesCompartidas);
							var = *(VariableCompartida*)result;
							var.valorVariableGlobal = valorAAsignar;

							//Devuelvo a la cpu el valor de la variable compartida, el cual asigne
							tamDatos = sizeof(uint32_t) * 2;
							datos = malloc(tamDatos);

							((uint32_t*) datos)[0] = ASIGNARSHAREDVAR;
							((uint32_t*) datos)[1] = var.valorVariableGlobal;

							EnviarDatos(socketConectado, KERNEL, datos, tamDatos);

							free(datos);
						break;

						case WAITSEM:
							PID = ((uint32_t*)paquete.Payload)[1];
							strcpy(nombreSem, (char*)(paquete.Payload+sizeof(uint32_t) * 2));

							result = NULL;
							pthread_mutex_lock(&mutexSemaforos);
							result = list_find(Semaforos, LAMBDA(bool _(void* item) { return ((Semaforo*) item)->nombreSemaforo == nombreSem; }));
							pthread_mutex_unlock(&mutexSemaforos);
							if(result != NULL)
							{
								Semaforo* semaf = (Semaforo*)result;
								semaf->valorSemaforo--;

								BloqueControlProceso* pcb = malloc(sizeof(BloqueControlProceso));
								RecibirPCB(pcb, socketConectado, KERNEL);

								if (semaf->valorSemaforo < 0)
								{ //se bloquea
									pthread_mutex_lock(&mutexSemaforos);
									list_add(semaf->listaDeProcesos, &PID);
									pthread_mutex_unlock(&mutexSemaforos);
									pthread_mutex_lock(&mutexQueueEjecutando);
									list_remove_by_condition(Ejecutando->elements,  LAMBDA(bool _(void* item) {
										return ((BloqueControlProceso*)item)->PID == pcb->PID;
									}));
									pthread_mutex_unlock(&mutexQueueEjecutando);
									pthread_mutex_lock(&mutexQueueBloqueados);
									queue_push(Bloqueados, pcb);
									pthread_mutex_unlock(&mutexQueueBloqueados);
								}

								else
								{
									//si hay un wait, se mete otra vez en la cola de listos
									pcb->ProgramCounter++;
									pthread_mutex_lock(&mutexQueueListos);
									list_add_in_index(Listos->elements, 0, pcb);
									pthread_mutex_unlock(&mutexQueueListos);
								}
							}
							else
							{
								FinalizarPrograma(PID, ERRORSINDEFINIR, socketConMemoria);
							}
						break;

						case SIGNALSEM:
							PID = ((uint32_t*)paquete.Payload)[1];
							strcpy(nombreSem, (char*)(paquete.Payload+sizeof(uint32_t)*2));
							result = NULL;
							pthread_mutex_lock(&mutexSemaforos);
							result = (Semaforo*) list_find(Semaforos, LAMBDA(bool _(void* item) { return ((Semaforo*) item)->nombreSemaforo == nombreSem; }));
							pthread_mutex_unlock(&mutexSemaforos);
							if(result != NULL)
							{
								Semaforo* semaf = (Semaforo*)result;
								semaf->valorSemaforo++;

								if (semaf->valorSemaforo >= 0)
								{
									pthread_mutex_lock(&mutexSemaforos);
									uint32_t pid = *(uint32_t*) list_get(semaf->listaDeProcesos, 0);
									list_remove_by_condition(semaf->listaDeProcesos, LAMBDA(bool _(void* item) { return *((uint32_t*) item) == pid; }));
									pthread_mutex_unlock(&mutexSemaforos);
									pthread_mutex_lock(&mutexQueueBloqueados);
									BloqueControlProceso* pcbDesbloqueado = list_remove_by_condition(Bloqueados->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*) item)->PID == pid; }));
									pcbDesbloqueado->ProgramCounter++;
									pthread_mutex_unlock(&mutexQueueBloqueados);
									pthread_mutex_lock(&mutexQueueListos);
									queue_push(Listos, pcbDesbloqueado);
									pthread_mutex_unlock(&mutexQueueListos);

								}
							}
							else
							{
								FinalizarPrograma(PID, ERRORSINDEFINIR, socketConMemoria);
							}

						break;

						case RESERVARHEAP:
							PID = ((uint32_t*)paquete.Payload)[1];
							tamanioAReservar = ((uint32_t*)paquete.Payload)[2];

							uint32_t punteroADevolver = SolicitarHeap(PID, tamanioAReservar, socketConectado);

							tamDatos = sizeof(uint32_t) * 2;
							datos = malloc(tamDatos);

							((uint32_t*) datos)[0] = RESERVARHEAP;
							((uint32_t*) datos)[1] = punteroADevolver;

							EnviarDatos(socketConectado, KERNEL, datos, tamDatos);

							free(datos);
						break;

						case LIBERARHEAP:
							PID = ((uint32_t*)paquete.Payload)[1];
							uint32_t punteroALiberar = ((uint32_t*)paquete.Payload)[2];

							PosicionDeMemoria pos;

							pos.NumeroDePagina = punteroALiberar / TamanioPagina;
							pos.Offset = punteroALiberar % TamanioPagina;

							SolicitudLiberacionDeBloque(socketConectado, PID, pos);
						break;

						case ABRIRARCHIVO:
							PID = ((uint32_t*)paquete.Payload)[1];

							permisosArchivo permisos;
							permisos.creacion = *((bool*)paquete.Payload+sizeof(uint32_t) * 2);
							permisos.escritura = *((bool*)paquete.Payload+sizeof(uint32_t) * 3);
							permisos.lectura = *((bool*)paquete.Payload+sizeof(uint32_t) * 4);

							abrirArchivo(((char*)paquete.Payload+sizeof(uint32_t) * 2 + sizeof(bool) * 3), PID, permisos);
						break;

						case BORRARARCHIVO:
							PID = ((uint32_t*)paquete.Payload)[1];
							FD = ((uint32_t*)paquete.Payload)[2];

							borrarArchivo(FD, PID);
						break;

						case CERRARARCHIVO:
							PID = ((uint32_t*)paquete.Payload)[1];
							FD = ((uint32_t*)paquete.Payload)[2];

							cerrarArchivo(FD, PID);
						break;

						case MOVERCURSOSARCHIVO:
							PID = ((uint32_t*)paquete.Payload)[1];
							FD = ((uint32_t*)paquete.Payload)[2];
							uint32_t posicion = ((uint32_t*)paquete.Payload)[3];

							moverCursor(FD, PID, posicion);
						break;

						case ESCRIBIRARCHIVO:
							PID = ((uint32_t*)paquete.Payload)[1];
							FD = ((uint32_t*)paquete.Payload)[2];
							tamanioArchivo = ((uint32_t*)paquete.Payload)[3];
							escribirArchivo(FD, PID, tamanioArchivo, ((char*)paquete.Payload+sizeof(uint32_t) * 4));
						break;

						case LEERARCHIVO:
							PID = ((uint32_t*)paquete.Payload)[1];
							FD = ((uint32_t*)paquete.Payload)[2];
							tamanioArchivo = ((uint32_t*)paquete.Payload)[3];

							leerArchivo(FD, PID, tamanioArchivo);
						break;

						case FINEJECUCIONPROGRAMA:
							PID = ((uint32_t*)paquete.Payload)[1];
							//Finalizar programa
							FinalizarPrograma(PID, FINALIZACIONNORMAL, socketConectado);
						break;
					}
				}
			break;
			case ESDESCONEXIONCPU:
				pthread_mutex_lock(&mutexCPUsConectadas);
				list_remove_by_condition(CPUsConectadas,LAMBDA(bool _(void* item){
					return ((DatosCPU*)item)->socketCPU==socketConectado;}
				));
				pthread_mutex_unlock(&mutexCPUsConectadas);
			break;
			case KILLPROGRAM:
				if(strcmp(paquete.header.emisor, CONSOLA) == 0)
				{
					pidAFinalizar = *(uint32_t*)paquete.Payload;
					bool finalizadoConExito = KillProgram(pidAFinalizar, DESCONECTADODESDECOMANDOCONSOLA, socketConectado);

					if(finalizadoConExito == true)
					{
						printf("El programa %d fue finalizado\n", pidAFinalizar);
						EnviarMensaje(socketConectado,"KILLEADO",KERNEL);
					}
					else
					{
						printf("Error al finalizar programa %d\n", pidAFinalizar);
						EnviarMensaje(socketConectado, "Error al finalizar programa", KERNEL);
					}
				}
			break;

			case ESPCB:
				//termino la rafaga normalmente sin bloquearse
				if(strcmp(paquete.header.emisor, CPU) == 0)
				{
					BloqueControlProceso* pcb = malloc(sizeof(BloqueControlProceso));
					RecibirPCB(pcb, socketConectado, KERNEL);
					pthread_mutex_lock(&mutexQueueEjecutando);
					list_remove_by_condition((t_list*)Ejecutando,  LAMBDA(bool _(void* item) {
						return ((BloqueControlProceso*)item)->PID == pcb->PID;
					}));
					pthread_mutex_unlock(&mutexQueueEjecutando);
					if (pcb->IndiceDeCodigo->elements_count == pcb->cantidadDeRafagasEjecutadas)
					{
						FinalizarPrograma(pcb->PID, FINALIZACIONNORMAL, socketConMemoria);
					}
					else
					{
						pthread_mutex_lock(&mutexQueueListos);
						queue_push(Listos, pcb);
						pthread_mutex_unlock(&mutexQueueListos);
					}

				}
			break;
		}

		free(paquete.Payload);
	}

	close(socketConectado);

}
