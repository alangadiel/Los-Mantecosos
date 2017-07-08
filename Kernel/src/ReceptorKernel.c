#include "ReceptorKernel.h"

void receptorKernel(Paquete* paquete, int socketConectado){
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
				printf("Tipo operacion : %u\n",*(uint32_t*)paquete->Payload);
				switch ((*(uint32_t*)paquete->Payload))
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
						PID = ((uint32_t*)paquete->Payload)[1];
						variableCompartida =paquete->Payload + sizeof(uint32_t) * 2;
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
						PID = ((uint32_t*)paquete->Payload)[1];
						valorAAsignar = ((int32_t*)paquete->Payload)[2];
						variableCompartida =paquete->Payload + sizeof(uint32_t) * 3;
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
						PID = ((uint32_t*)paquete->Payload)[1];
						nombreSem = paquete->Payload + sizeof(uint32_t)*2;
						result = NULL;
						pthread_mutex_lock(&mutexSemaforos);
						result = list_find(Semaforos,
							LAMBDA(bool _(void* item) {
							char* nombreActual = ((Semaforo*) item)->nombreSemaforo;
							return strcmp(nombreActual, nombreSem) == 0;
							}));
						pthread_mutex_unlock(&mutexSemaforos);
						if(result != NULL)
						{
							printf("nombre semaforo %s", ((Semaforo*)result)->nombreSemaforo);
							Semaforo* semaf = (Semaforo*)result;
							semaf->valorSemaforo--;

							BloqueControlProceso* pcb = list_find(Ejecutando->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*) item)->PID == PID; }));
							RecibirPaqueteServidorKernel(socketConectado, KERNEL, paquete);
							RecibirPCB(pcb, paquete->Payload, paquete->header.tamPayload,KERNEL);

							if (semaf->valorSemaforo < 0)
							{ //se bloquea
								pthread_mutex_lock(&mutexSemaforos);
								uint32_t* pid = malloc(sizeof(uint32_t));
								*pid=PID;
								list_add(semaf->listaDeProcesos, pid);
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
								//pcb->ProgramCounter++;
								Evento_ListosAdd();
								pthread_mutex_lock(&mutexQueueListos);
								list_add_in_index(Listos->elements, 0, pcb);
								pthread_mutex_unlock(&mutexQueueListos);
							}
						}
						else
						{
							FinalizarPrograma(PID, ERRORSINDEFINIR);
						}
					break;

					case SIGNALSEM:
						PID = ((uint32_t*)paquete->Payload)[1];
						nombreSem = paquete->Payload + sizeof(uint32_t)*2;
						result = NULL;
						pthread_mutex_lock(&mutexSemaforos);
						result = list_find(Semaforos,
							LAMBDA(bool _(void* item) {
							char* nombreActual = ((Semaforo*) item)->nombreSemaforo;
							return strcmp(nombreActual, nombreSem) == 0;
							}));
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
								//pcbDesbloqueado->ProgramCounter++;
								pthread_mutex_unlock(&mutexQueueBloqueados);

								Evento_ListosAdd();
								pthread_mutex_lock(&mutexQueueListos);
								queue_push(Listos, pcbDesbloqueado);
								pthread_mutex_unlock(&mutexQueueListos);

							}
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
						printf("El archivo fue abierto\n");
						permisosArchivo permisos;
						permisos.creacion = *((bool*)paquete->Payload+sizeof(uint32_t) * 2);
						permisos.escritura = *((bool*)paquete->Payload+sizeof(uint32_t) * 3);
						permisos.lectura = *((bool*)paquete->Payload+sizeof(uint32_t) * 4);

						abrirArchivo(((char*)paquete->Payload+sizeof(uint32_t) * 2 + sizeof(bool) * 3), PID, permisos, socketConectado,&tipoError);

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
						if(FD==1){
							printf("Escribiendo en el FD N°1 la informacion siguiente: %s\n",((char*)paquete->Payload+sizeof(uint32_t) * 4));
						}
						else{
							escribirArchivo(FD, PID, tamanioArchivo, ((char*)paquete->Payload+sizeof(uint32_t) * 4));
							printf("El archivo fue escrito con %s \n", ((char*)paquete->Payload+sizeof(uint32_t) * 4));
						}
					break;

					case LEERARCHIVO:
						PID = ((uint32_t*)paquete->Payload)[1];
						FD = ((uint32_t*)paquete->Payload)[2];
						tamanioArchivo = ((uint32_t*)paquete->Payload)[3];
						leerArchivo(FD, PID, tamanioArchivo);
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

			case ESPCB:
				//termino la rafaga normalmente sin bloquearse
				if(strcmp(paquete->header.emisor, CPU) == 0)
				{
					DatosCPU* cpuActual = list_find(CPUsConectadas, LAMBDA(bool _(void* item) { return ((DatosCPU*) item)->socketCPU == socketConectado; }));
					BloqueControlProceso* pcb = list_find(Ejecutando->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*) item)->PID == cpuActual->pid; }));
					if(pcb!=NULL){
						RecibirPCB(pcb, paquete->Payload, paquete->header.tamPayload,KERNEL);

						DatosCPU * datoscpu = list_find(CPUsConectadas,LAMBDA(bool _(void* item) {
							return ((DatosCPU*)item)->socketCPU == socketConectado;
						}));
						datoscpu->isFree = true;
						sem_post(&semDispatcherCpus);
						if (pcb->ExitCode<= 0)
						{
							FinalizarPrograma(pcb->PID,pcb->ExitCode);
						}
						else
						{
							pthread_mutex_lock(&mutexQueueEjecutando);
							list_remove_by_condition(Ejecutando->elements,  LAMBDA(bool _(void* item) {
								return ((BloqueControlProceso*)item)->PID == pcb->PID;
							}));
							pthread_mutex_unlock(&mutexQueueEjecutando);

							Evento_ListosAdd();
							pthread_mutex_lock(&mutexQueueListos);
							queue_push(Listos, pcb);
							pthread_mutex_unlock(&mutexQueueListos);
						}
					} else
						printf("Error al finalizar ejecucion");
				}
				break;
	}
}
