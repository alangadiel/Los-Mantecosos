#include "ReceptorKernel.h"
Semaforo semaforoAVerificar;
void receptorKernel(Paquete* paquete, int socketConectado){
	int32_t valorAAsignar;
	char* variableCompartida;
	uint32_t PID;
	uint32_t FD;
	uint32_t tamanioArchivo;
	uint32_t punteroArchivo;
	char* path;
	void* result;
	void* datos;
	uint32_t abrir =0;
	int tamDatos;

	char* nombreSem;
	uint32_t tamanioAReservar;

	switch(paquete->header.tipoMensaje)	{
	case ESSTRING:
		if(strcmp(paquete->header.emisor, CONSOLA) == 0)
		{
			int tamaniCodigoEnPaginas = paquete->header.tamPayload / TamanioPagina + 1;
			int tamanioCodigoYStackEnPaginas = tamaniCodigoEnPaginas + STACK_SIZE;
			BloqueControlProceso* pcb = malloc(sizeof(BloqueControlProceso));


			CrearNuevoProceso(pcb,&ultimoPID,Nuevos);

			AgregarAListadePidsPorSocket(pcb->PID, socketConectado);

			//Manejo la multiprogramacion
			if(GRADO_MULTIPROG - queue_size(Ejecutando) - queue_size(Listos) > 0 && queue_size(Nuevos) >= 1)
			{
				//Pregunta a la memoria si me puede guardar estas paginas
				bool pudoCargarse = IM_InicializarPrograma(socketConMemoria, KERNEL, pcb->PID, tamanioCodigoYStackEnPaginas);

				if(pudoCargarse == true)
				{
					//Ejecuto el metadata program
					CargarInformacionDelCodigoDelPrograma(pcb, paquete);
					pcb->PaginasDeCodigo = tamaniCodigoEnPaginas;
					//Solicito a la memoria que me guarde el codigo del programa(dependiendo cuantas paginas se requiere para el codigo
					GuardarCodigoDelProgramaEnLaMemoria(pcb, paquete);
					PonerElProgramaComoListo(pcb,paquete, socketConectado, tamaniCodigoEnPaginas);
					EnviarDatos(socketConectado, KERNEL, &(pcb->PID), sizeof(uint32_t));
				}
				else
				{
					//Sacar programa de la lista de nuevos y meterlo en la lista de finalizado con su respectivo codigo de error
					FinalizarPrograma(pcb->PID,NOSEPUDIERONRESERVARRECURSOS);
					EnviarMensaje(socketConectado, "No se pudo guardar el programa porque no hay espacio suficiente", KERNEL);
				}
			}
			else
			{
				//El grado de multiprogramacion no te deja agregar otro proceso
				FinalizarPrograma(pcb->PID,NOSEPUDIERONRESERVARRECURSOS);
				EnviarMensaje(socketConectado, "No se pudo guardar el programa porque se alcanzo el grado de multiprogramacion", KERNEL);
			}
		}
		break;

		case ESDATOS:
			if(strcmp(paquete->header.emisor, CPU) == 0)
			{
				uint32_t tipoOperacion = ((uint32_t*)paquete->Payload)[0];
				printf("\nTipo operacion : %u\n",tipoOperacion);
				printf("Pid : %u\n",((uint32_t*)paquete->Payload)[1]);

				switch (tipoOperacion)
				{
					case PEDIRSHAREDVAR:
						PID = ((uint32_t*)paquete->Payload)[1];
						variableCompartida = string_duplicate(paquete->Payload + sizeof(uint32_t) * 2);
						printf("Variable compartida solicitada :%s\n",variableCompartida);

						//Busco la variable compartida
						result = NULL;
						pthread_mutex_lock(&mutexVariablesCompartidas);
						bool buscarSharedVar(void *item){
							VariableCompartida* var = item;
							char* nombreVarEnCuestion = string_substring_from(var->nombreVariableGlobal,1);
							return strcmp(nombreVarEnCuestion, variableCompartida) == 0;
						}
						VariableCompartida* sharedVar = list_find(VariablesCompartidas,buscarSharedVar);
						pthread_mutex_unlock(&mutexVariablesCompartidas);
						printf("Variable compartida encontrada: %s y su valor es: %i \n",sharedVar->nombreVariableGlobal,sharedVar->valorVariableGlobal);

						//Devuelvo a la cpu el valor de la variable compartida
						tamDatos = sizeof(int32_t);
						datos = malloc(tamDatos);
						((int32_t*) datos)[0] = sharedVar->valorVariableGlobal;
						EnviarDatos(socketConectado, KERNEL, datos, tamDatos);
						free(datos);
					break;

					case ASIGNARSHAREDVAR:
						PID = ((uint32_t*)paquete->Payload)[1];
						valorAAsignar = ((int32_t*)paquete->Payload)[2];
						variableCompartida =string_duplicate(paquete->Payload + sizeof(uint32_t) * 3);
						//Busco la variable compartida
						result = NULL;
						pthread_mutex_lock(&mutexVariablesCompartidas);
						VariableCompartida* sharedvar = list_find(VariablesCompartidas,buscarSharedVar);
						sharedvar->valorVariableGlobal = valorAAsignar;
						printf("Se le asigno a %s el valor %u\n", sharedvar->nombreVariableGlobal, sharedvar->valorVariableGlobal);
						pthread_mutex_unlock(&mutexVariablesCompartidas);

						//Devuelvo a la cpu el valor de la variable compartida, el cual asigne
						tamDatos = sizeof(int32_t);
						datos = malloc(tamDatos);
						((int32_t*) datos)[0] = sharedvar->valorVariableGlobal;
						EnviarDatos(socketConectado, KERNEL, datos, tamDatos);
						free(datos);
					break;

					case WAITSEM:
						PID = ((uint32_t*)paquete->Payload)[1];
						nombreSem = string_duplicate(paquete->Payload + sizeof(uint32_t)*2);

						result = NULL;
						pthread_mutex_lock(&mutexSemaforos);
						bool buscarSemaforo(void* item){
							Semaforo *sem = item;
							return strcmp(sem->nombreSemaforo,nombreSem)==0;
						}
						Semaforo *semaforoEncontrado = list_find(Semaforos,buscarSemaforo);
						pthread_mutex_unlock(&mutexSemaforos);

						if(semaforoEncontrado != NULL)
						{
							semaforoEncontrado->valorSemaforo--;
							//semaforoAVerificar = malloc(sizeof(Semaforo));
							semaforoAVerificar = *semaforoEncontrado;
							printf("Semaforo: %s\n",semaforoAVerificar.nombreSemaforo);
						}
						else
						{
							FinalizarPrograma(PID, ERRORSINDEFINIR);
						}
					break;

					case SIGNALSEM:
						PID = ((uint32_t*)paquete->Payload)[1];
						nombreSem = string_duplicate(paquete->Payload + sizeof(uint32_t)*2);
						result = NULL;
						pthread_mutex_lock(&mutexSemaforos);
						result = list_find(Semaforos,buscarSemaforo);
						pthread_mutex_unlock(&mutexSemaforos);
						if(result != NULL)
						{
							Semaforo* semaf = (Semaforo*)result;
							printf("Semaforo encontrado para signal : %s\n",semaf->nombreSemaforo);

							semaf->valorSemaforo++;
							printf("Valor semaforo despues de signal: %i\n",semaf->valorSemaforo);
							printf("Cant procesos bloqueados por el semaforo: %u\n",queue_size(semaf->listaDeProcesos));
							if (semaf->valorSemaforo >= 0 && queue_size(semaf->listaDeProcesos)>0)
							{
								pthread_mutex_lock(&mutexSemaforos);
								uint32_t* pidDesbloqueadoDeLaColaDelSem = queue_pop(semaf->listaDeProcesos);
								pthread_mutex_unlock(&mutexSemaforos);
								pthread_mutex_lock(&mutexQueueBloqueados);
								BloqueControlProceso* pcbDesbloqueado = list_remove_by_condition(Bloqueados->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*) item)->PID == *pidDesbloqueadoDeLaColaDelSem; }));
								//pcbDesbloqueado->ProgramCounter++;
								printf("Pcb con %u desbloqueado \n",pcbDesbloqueado->PID);
								pthread_mutex_unlock(&mutexQueueBloqueados);

								pthread_mutex_lock(&mutexQueueListos);
								queue_push(Listos, pcbDesbloqueado);
								pthread_mutex_unlock(&mutexQueueListos);

							}
							sem_post(&semDispatcherCpus);
							Evento_ListosAdd();
						}
						else
						{
							FinalizarPrograma(PID, ERRORSINDEFINIR);
						}

					break;

					case RESERVARHEAP:
						PID = ((uint32_t*)paquete->Payload)[1];
						tamanioAReservar = ((uint32_t*)paquete->Payload)[2];
						printf("Se solicita reservar en el heap %u bytes para el proceso %u\n",tamanioAReservar,PID);
						int32_t tipoError=0;
						uint32_t punteroADevolver = SolicitarHeap(PID, tamanioAReservar,&tipoError);
						if(tipoError==0){
							printf("entra al if despues de solicitar heap");
							tamDatos = sizeof(uint32_t);
							void *data = malloc(tamDatos);
							//(uint32_t*) data) = punteroADevolver;
							memcpy(data,&punteroADevolver,sizeof(tamDatos));
							printf("Puntero a devolver a la cpu :%u\n",*(uint32_t*)data);
							EnviarDatos(socketConectado, KERNEL, data, tamDatos);
							free(data);
						}
						else{
							EnviarDatosTipo(socketConectado,KERNEL,&tipoError,sizeof(int32_t),ESERROR);

						}
					break;

					case LIBERARHEAP:
						PID = ((uint32_t*)paquete->Payload)[1];
						uint32_t punteroALiberar = ((uint32_t*)paquete->Payload)[2];

						SolicitudLiberacionDeBloque(PID, punteroALiberar,&tipoError);
					break;

					case ABRIRARCHIVO:
						PID = ((uint32_t*)paquete->Payload)[1];

						BanderasPermisos* bandera = paquete->Payload + sizeof(uint32_t) * 2;

						path = string_duplicate(paquete->Payload + sizeof(uint32_t) * 2 + sizeof(BanderasPermisos));

						abrir = abrirArchivo(path, PID, *bandera, socketConectado, &tipoError);

						if(abrir == 0)
						{
							printf("El archivo no pudo ser abierto por falta de permisos de creacion\n");
						}
						else
						{
							printf("El archivo fue abierto\n");
						}
					break;

					case BORRARARCHIVO:
						PID = ((uint32_t*)paquete->Payload)[1];
						FD = ((uint32_t*)paquete->Payload)[2];

						borrarArchivo(FD, PID);
					break;

					case CERRARARCHIVO:
						PID = ((uint32_t*)paquete->Payload)[1];
						FD = ((uint32_t*)paquete->Payload)[2];

						cerrarArchivo(FD, PID);
					break;

					case MOVERCURSOSARCHIVO:
						PID = ((uint32_t*)paquete->Payload)[1];
						FD = ((uint32_t*)paquete->Payload)[2];
						uint32_t posicion = ((uint32_t*)paquete->Payload)[3];

						moverCursor(FD, PID, posicion);
					break;

					case ESCRIBIRARCHIVO:
						PID = ((uint32_t*)paquete->Payload)[1];
						FD = ((uint32_t*)paquete->Payload)[2];
						tamanioArchivo = ((uint32_t*)paquete->Payload)[3];

						//Si el FD es 1, hay que mostrarlo por pantalla
						if(FD == 1)
						{
							pthread_mutex_lock(&mutexConsolaFD1);
							printf("Escribiendo en el FD N°1 la informacion siguiente: %s\n",((char*)paquete->Payload+sizeof(uint32_t) * 4));
							pthread_mutex_unlock(&mutexConsolaFD1);
						}
						else
						{
							escribirArchivo(FD, PID, tamanioArchivo, ((void*)paquete->Payload+sizeof(uint32_t) * 4));

							printf("El archivo fue escrito con %s \n", ((void*)paquete->Payload+sizeof(uint32_t) * 4));
						}
					break;

					case LEERARCHIVO:
						PID = ((uint32_t*)paquete->Payload)[1];
						FD = ((uint32_t*)paquete->Payload)[2];
						tamanioArchivo = ((uint32_t*)paquete->Payload)[3];
						punteroArchivo = ((uint32_t*)paquete->Payload)[4];

						void* datosLeidos = leerArchivo(FD, PID, tamanioArchivo, punteroArchivo);

						printf("Se leyo %s", datosLeidos);
					break;
					/*
					case FINEJECUCIONPROGRAMA:
						PID = ((uint32_t*)paquete->Payload)[1];
						//Finalizar programa
						FinalizarPrograma(PID, FINALIZACIONNORMAL);
					break;
					*/
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
				if(strcmp(paquete->header.emisor, CONSOLA) == 0)
				{
					pidAFinalizar = *(uint32_t*)paquete->Payload;
					bool finalizadoConExito = KillProgram(pidAFinalizar, DESCONECTADODESDECOMANDOCONSOLA);

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
			case ESPCBWAIT:
				if(strcmp(paquete->header.emisor, CPU) == 0)
				{
					pthread_mutex_lock(&mutexCPUsConectadas);
					DatosCPU* cpuActual = list_find(CPUsConectadas, LAMBDA(bool _(void* item) { return ((DatosCPU*) item)->socketCPU == socketConectado; }));
					cpuActual->isFree = true;
					pthread_mutex_unlock(&mutexCPUsConectadas);
					pthread_mutex_lock(&mutexQueueEjecutando);
					BloqueControlProceso* pcb = list_find(Ejecutando->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*) item)->PID == cpuActual->pid; }));
					pthread_mutex_unlock(&mutexQueueEjecutando);
					if(pcb!=NULL){
						RecibirPCB(pcb, paquete->Payload, paquete->header.tamPayload,KERNEL);
						printf("Valor semaforo dsps de decrementar %i\n",semaforoAVerificar.valorSemaforo);
						if (semaforoAVerificar.valorSemaforo < 0)
						{ //se bloquea
							printf("Se va a bloquear el proceso\n");

							pthread_mutex_lock(&mutexSemaforos);
							uint32_t* pid = malloc(sizeof(uint32_t));
							*pid=pcb->PID;
							queue_push(semaforoAVerificar.listaDeProcesos, pid);
							pthread_mutex_unlock(&mutexSemaforos);
							pthread_mutex_lock(&mutexQueueEjecutando);
							list_remove_by_condition(Ejecutando->elements,  LAMBDA(bool _(void* item) {
								return ((BloqueControlProceso*)item)->PID == pcb->PID;
							}));
							pthread_mutex_unlock(&mutexQueueEjecutando);
							pthread_mutex_lock(&mutexQueueBloqueados);
							queue_push(Bloqueados, pcb);
							pthread_mutex_unlock(&mutexQueueBloqueados);
							sem_post(&semDispatcherCpus);

						}

						else
						{
							//si hay un wait, se mete otra vez en la cola de listos
							//pcb->ProgramCounter++;
							pthread_mutex_lock(&mutexQueueListos);
							list_add_in_index(Listos->elements, 0, pcb);
							pthread_mutex_unlock(&mutexQueueListos);
							sem_post(&semDispatcherCpus);
							Evento_ListosAdd();

						}


						//free(semaforoAVerificar);
					} else
						printf("Error al finalizar ejecucion");
				}
			break;

			case ESPCB:
				//termino la rafaga normalmente sin bloquearse
				if(strcmp(paquete->header.emisor, CPU) == 0)
				{
					pthread_mutex_lock(&mutexCPUsConectadas);
					DatosCPU* cpuActual = list_find(CPUsConectadas, LAMBDA(bool _(void* item) { return ((DatosCPU*) item)->socketCPU == socketConectado; }));
					cpuActual->isFree = true;
					pthread_mutex_unlock(&mutexCPUsConectadas);
					pthread_mutex_lock(&mutexQueueEjecutando);
					BloqueControlProceso* pcb = list_find(Ejecutando->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*) item)->PID == cpuActual->pid; }));
					pthread_mutex_unlock(&mutexQueueEjecutando);
					if(pcb!=NULL){
						RecibirPCB(pcb, paquete->Payload, paquete->header.tamPayload,KERNEL);

						if (pcb->ExitCode<= 0)
						{
							FinalizarPrograma(pcb->PID,pcb->ExitCode);
							sem_post(&semDispatcherCpus);

						}
						else
						{
							pthread_mutex_lock(&mutexQueueEjecutando);
							BloqueControlProceso* pcbEjecutado = list_remove_by_condition(Ejecutando->elements,  LAMBDA(bool _(void* item) {
								return ((BloqueControlProceso*)item)->PID == pcb->PID;
							}));
							pthread_mutex_unlock(&mutexQueueEjecutando);
							if(pcbEjecutado!=NULL){
								pthread_mutex_lock(&mutexQueueListos);
								queue_push(Listos, pcb);
								pthread_mutex_unlock(&mutexQueueListos);
								sem_post(&semDispatcherCpus);
								Evento_ListosAdd();
							}
						}
					} else
						printf("Error al finalizar ejecucion");
				}
				break;
	}
}
