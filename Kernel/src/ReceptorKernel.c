#include "ReceptorKernel.h"
int socketConsola = -1;
int index_semaforo_wait=0;

void receptorKernel(Paquete* paquete, int socketConectado){
	int32_t valorAAsignar;
	char* variableCompartida;
	uint32_t PID;
	uint32_t FD;
	uint32_t tamanioArchivo;
	uint32_t punteroArchivo;
	uint32_t punteroALiberar;
	char* path;
	void* result;
	void* datos;
	uint32_t abrir =0;
	int tamDatos;
	char* nombreSemaf;
	char* nombreSemaforoWait;
	uint32_t tamanioAReservar;
	BanderasPermisos* bandera;
	VariableCompartida* sharedvar;

	switch(paquete->header.tipoMensaje)	{
	case ESSTRING:
		if(strcmp(paquete->header.emisor, CONSOLA) == 0)
		{
			int tamaniCodigoEnPaginas = paquete->header.tamPayload / TamanioPagina + 1;
			int tamanioCodigoYStackEnPaginas = tamaniCodigoEnPaginas + STACK_SIZE;
			BloqueControlProceso* pcb = malloc(sizeof(BloqueControlProceso));


			CrearNuevoProceso(pcb,&ultimoPID,Nuevos);

			if (list_find(PIDsPorSocketConsola, LAMBDA(bool _(void* item){	return ((PIDporSocketConsola*)item)->PID == pcb->PID;})) == NULL)
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
				PID = ((uint32_t*)paquete->Payload)[1];
				printf("\n[PID %u] Tipo operacion : %u\n", PID, tipoOperacion);

				//Chequeo que el proceso no haya sido finalizado
				pthread_mutex_lock(&mutexFinalizarPrograma);
				pthread_mutex_lock(&mutexQueueFinalizados);
				if (list_any_satisfy(Finalizados->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == PID; })))
				{
					pthread_mutex_unlock(&mutexQueueFinalizados);
					pthread_mutex_unlock(&mutexFinalizarPrograma);
					break; //salir del switch
				}
				pthread_mutex_unlock(&mutexQueueFinalizados);
				pthread_mutex_unlock(&mutexFinalizarPrograma);

				switch (tipoOperacion)
				{
					case PEDIRSHAREDVAR:
						variableCompartida = string_duplicate(paquete->Payload + sizeof(uint32_t) * 2);
						printf("Variable compartida solicitada :%s\n",variableCompartida);

						//Busco la variable compartida
						result = NULL;
						bool buscarSharedVar(void *item){
							VariableCompartida* var = item;
							char* nombreVarEnCuestion = string_substring_from(var->nombreVariableGlobal,1);
							return strcmp(nombreVarEnCuestion, variableCompartida) == 0;
						}
						sharedvar = list_find(VariablesCompartidas,buscarSharedVar);
						if(sharedvar!=NULL){
							pthread_mutex_lock(&mutexVariablesCompartidas);
							printf("Variable compartida encontrada: %s y su valor es: %i \n",sharedvar->nombreVariableGlobal,sharedvar->valorVariableGlobal);
							//Devuelvo a la cpu el valor de la variable compartida
							tamDatos = sizeof(int32_t);
							datos = malloc(tamDatos);
							((int32_t*) datos)[0] = sharedvar->valorVariableGlobal;
							pthread_mutex_unlock(&mutexVariablesCompartidas);

							EnviarDatos(socketConectado, KERNEL, datos, tamDatos);
							free(datos);
						}
						else{
							FinalizarPrograma(PID,ERRORSINDEFINIR);
						}

					break;

					case ASIGNARSHAREDVAR:
						valorAAsignar = ((int32_t*)paquete->Payload)[2];
						variableCompartida =string_duplicate(paquete->Payload + sizeof(uint32_t) * 3);
						//Busco la variable compartida
						result = NULL;
						sharedvar = list_find(VariablesCompartidas,buscarSharedVar);
						if(sharedvar!=NULL){
							pthread_mutex_lock(&mutexVariablesCompartidas);
							sharedvar->valorVariableGlobal = valorAAsignar;
							printf("Se le asigno a %s el valor %u\n", sharedvar->nombreVariableGlobal, sharedvar->valorVariableGlobal);
							//Devuelvo a la cpu el valor de la variable compartida, el cual asigne
							tamDatos = sizeof(int32_t);
							datos = malloc(tamDatos);
							((int32_t*) datos)[0] = sharedvar->valorVariableGlobal;
							pthread_mutex_unlock(&mutexVariablesCompartidas);
							EnviarDatos(socketConectado, KERNEL, datos, tamDatos);
							free(datos);
						}

					break;

					case WAITSEM:
						nombreSemaforoWait = string_duplicate(paquete->Payload + sizeof(uint32_t)*2);

						Semaforo* sem;
						pthread_mutex_lock(&mutexIndiceSemaforoWait);
						index_semaforo_wait = 0;
						while(index_semaforo_wait<list_size(Semaforos)){
							sem = list_get(Semaforos,index_semaforo_wait);
							if(strcmp(sem->nombreSemaforo,nombreSemaforoWait)==0)
								break;
							index_semaforo_wait++;
						}
						if(sem==NULL){
							FinalizarPrograma(PID,ERRORSINDEFINIR);
						}
					break;

					case SIGNALSEM:
						nombreSemaf = string_duplicate(paquete->Payload + sizeof(uint32_t)*2);
						result = NULL;
						bool despertar = false;
						uint32_t* pidDesbloqueadoDeLaColaDelSem;
						BloqueControlProceso* pcbDesbloqueado;
						bool searchSemaforo(void* item){
							Semaforo *sem = item;
							return strcmp(sem->nombreSemaforo,nombreSemaf)==0;
						}
						result = list_find(Semaforos,searchSemaforo);
						if(result != NULL)
						{

							Semaforo* semaf = (Semaforo*)result;
							pthread_mutex_lock(&mutexSemaforos);

							if (semaf->valorSemaforo < 0 && queue_size(semaf->listaDeProcesos)>0)
							{
								//pthread_mutex_lock(&mutexSemaforos);
								pidDesbloqueadoDeLaColaDelSem = queue_pop(semaf->listaDeProcesos);
								printf("pid desbloqueado de la cola del semaforo: %u\n",*pidDesbloqueadoDeLaColaDelSem);
								//pthread_mutex_unlock(&mutexSemaforos);

								pthread_mutex_lock(&mutexQueueBloqueados);
								printf("Tamanio de la cola de bloqueados: %u\n",queue_size(Bloqueados));
								pcbDesbloqueado = list_remove_by_condition(Bloqueados->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*) item)->PID == *pidDesbloqueadoDeLaColaDelSem; }));
								pthread_mutex_unlock(&mutexQueueBloqueados);

								free(pidDesbloqueadoDeLaColaDelSem);
								despertar = true;

							}

							/*else{
								//El signal no desbloqueo ningun proceso
								pthread_mutex_unlock(&mutexSemaforos);
							}*/
							semaf->valorSemaforo++;
							printf("[PID %u] Valor semaforo despues de hacer signal es %i\n", PID, semaf->valorSemaforo);
							pthread_mutex_unlock(&mutexSemaforos);
							if(despertar==true){
								pthread_mutex_lock(&mutexQueueListos);
								queue_push(Listos, pcbDesbloqueado);
								pthread_mutex_unlock(&mutexQueueListos);
								printf("Se desbloqueo el proceso N° %u\n",pcbDesbloqueado->PID);
								Evento_ListosAdd();
							}

						}
						else
							//No se encontro el semaforo
						{
							//pthread_mutex_unlock(&mutexSemaforos);
							FinalizarPrograma(PID, ERRORSINDEFINIR);
						}

					break;

					case RESERVARHEAP:
						tamanioAReservar = ((uint32_t*)paquete->Payload)[2];
						printf("[PID %u] Se solicita reservar en el heap %u bytes.\n", PID, tamanioAReservar);
						int32_t tipoError=0;
						uint32_t punteroADevolver = SolicitarHeap(PID, tamanioAReservar,&tipoError);
						if(tipoError==0){
							//printf("entra al if despues de solicitar heap");
							tamDatos = sizeof(uint32_t);
							void *data = malloc(tamDatos);
							//(uint32_t*) data) = punteroADevolver;
							memcpy(data,&punteroADevolver,sizeof(tamDatos));
							printf("[PID %u] Puntero a devolver a la cpu :%u\n", PID, *(uint32_t*)data);
							EnviarDatos(socketConectado, KERNEL, data, tamDatos);
							free(data);
						}
						else{
							printf("[PID %u] Hubo un error al reservar heap\n", PID);
							EnviarDatosTipo(socketConectado,KERNEL,&tipoError,sizeof(int32_t),ESERROR);

						}
					break;

					case LIBERARHEAP:
						punteroALiberar = ((uint32_t*)paquete->Payload)[2];

						SolicitudLiberacionDeBloque(PID, punteroALiberar,&tipoError);

					break;

					case ABRIRARCHIVO:
						bandera = paquete->Payload + sizeof(uint32_t) * 2;
						path = string_duplicate(paquete->Payload + sizeof(uint32_t) * 2 + sizeof(BanderasPermisos)); //no libero path xq esta en una lista
						string_trim(&path);
						abrir = abrirArchivo(path, PID, *bandera, socketConectado, &tipoError);
						if(abrir == 0)
						{
							printf("[PID %u] El archivo no pudo ser abierto por falta de permisos de creacion\n", PID);
						}
						else if(abrir == 1)
						{
							printf("[PID %u] El archivo no pudo ser abierto por falta de bloques disponibles\n", PID);
						}
						else
						{
							printf("[PID %u] El archivo fue abierto\n", PID);
						}
					break;

					case BORRARARCHIVO:
						FD = ((uint32_t*)paquete->Payload)[2];
						int tipoErrorBorrar = 0;
						tipoErrorBorrar = borrarArchivo(FD, PID, socketConFS);
						if(tipoErrorBorrar == 0)
						{
							uint32_t r = 1;

							EnviarDatosTipo(socketConectado,KERNEL,&r,sizeof(int32_t),ESDATOS);
						}
						else
						{
							EnviarDatosTipo(socketConectado,KERNEL,&tipoErrorBorrar,sizeof(int32_t),ESERROR);
						}
					break;

					case CERRARARCHIVO:
						FD = ((uint32_t*)paquete->Payload)[2];
						int tipoErrorCerrar = 0;
						tipoErrorCerrar = cerrarArchivo(FD, PID);
						if(tipoErrorCerrar == 0)
						{
							uint32_t r = 1;

							EnviarDatosTipo(socketConectado,KERNEL,&r,sizeof(int32_t),ESDATOS);
						}
						else
						{
							EnviarDatosTipo(socketConectado,KERNEL,&tipoErrorCerrar,sizeof(int32_t),ESERROR);
						}
					break;

					case MOVERCURSOSARCHIVO:
						FD = ((uint32_t*)paquete->Payload)[2];
						uint32_t posicion = ((uint32_t*)paquete->Payload)[3];
						int tipoErrorMover = 0;
						tipoErrorMover = moverCursor(FD, PID, posicion);
						if(tipoErrorMover == 0)
						{
							uint32_t r = 1;

							EnviarDatosTipo(socketConectado,KERNEL,&r,sizeof(int32_t),ESDATOS);
						}
						else
						{
							EnviarDatosTipo(socketConectado,KERNEL,&tipoErrorMover,sizeof(int32_t),ESERROR);
						}
					break;

					case ESCRIBIRARCHIVO:
						FD = ((uint32_t*)paquete->Payload)[2];
						tamanioArchivo = ((uint32_t*)paquete->Payload)[3];
						//Si el FD es 1, hay que mostrarlo por pantalla
						if(FD == 1)
						{
							char* mensaje = ((char*)paquete->Payload+sizeof(uint32_t) * 4);
							printf("[PID %u] mensaje a mandar a la consola %s\n", PID, mensaje);
							pthread_mutex_lock(&mutexConsolaFD1);
							void* datos = malloc(sizeof(uint32_t) * (string_length(mensaje) +1));
							((uint32_t*) datos)[0] = 0;
							memcpy(datos + sizeof(uint32_t), mensaje, string_length(mensaje) + 1);

							PIDporSocketConsola* PIDxSocket = list_find(PIDsPorSocketConsola, LAMBDA(bool _(void* item){	return ((PIDporSocketConsola*)item)->PID == ((uint32_t*)paquete->Payload)[1];}));

							uint32_t r = 1;

							EnviarDatosTipo(socketConectado,KERNEL,&r,sizeof(int32_t),ESDATOS);

							EnviarDatos(PIDxSocket->socketConsola, KERNEL, datos, sizeof(uint32_t) * string_length(mensaje) + 1);

							//printf("Escribiendo en el FD N°1 la informacion siguiente: %s\n",((char*)paquete->Payload+sizeof(uint32_t) * 4));
							pthread_mutex_unlock(&mutexConsolaFD1);
						}
						else
						{
							int tipoError = 0;

							tipoError = escribirArchivo(FD, PID, tamanioArchivo, ((void*)paquete->Payload+sizeof(uint32_t) * 4));

							if(tipoError == 0)
							{
								uint32_t r = 1;

								EnviarDatosTipo(socketConectado,KERNEL,&r,sizeof(int32_t),ESDATOS);
							}
							else
							{
								EnviarDatosTipo(socketConectado,KERNEL,&tipoError,sizeof(int32_t),ESERROR);
							}
						}
					break;

					case LEERARCHIVO:
						FD = ((uint32_t*)paquete->Payload)[2];
						tamanioArchivo = ((uint32_t*)paquete->Payload)[3];
						punteroArchivo = ((uint32_t*)paquete->Payload)[4];
						int tipoErrorLeer = 0;
						char* datosLeidos = leerArchivo(FD, PID, tamanioArchivo, punteroArchivo, &tipoErrorLeer);
						if(datosLeidos != NULL)
						{
							printf("[PID %u] Se leyo %s\n", PID, datosLeidos);
							uint32_t pagAGuardar = punteroArchivo/TamanioPagina;
							uint32_t offsetAGuardar = punteroArchivo % TamanioPagina;

							if(string_length(datosLeidos) < tamanioArchivo)
							{	/*
								int i;

								for(i = string_length(datosLeidos); i < (tamanioArchivo); i++)
								{
									datosLeidos[i]=' ';
								}*/

								datosLeidos[string_length(datosLeidos)]='\0';
							}

							//Guardo los datos leidos en el puntero que me piden
							IM_GuardarDatos(socketConMemoria,KERNEL,PID,pagAGuardar,offsetAGuardar,tamanioArchivo,datosLeidos);

							uint32_t r = 1;

							EnviarDatosTipo(socketConectado,KERNEL,&r,sizeof(int32_t),ESDATOS);
						}
						else
						{
							if(tipoErrorLeer == 0)
							{
								uint32_t r = 1;

								EnviarDatosTipo(socketConectado,KERNEL,&r,sizeof(int32_t),ESDATOS);
							}
							else
							{
								EnviarDatosTipo(socketConectado,KERNEL,&tipoErrorLeer,sizeof(int32_t),ESERROR);
							}
						}
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
				list_remove_and_destroy_by_condition(CPUsConectadas,LAMBDA(bool _(void* item){
					return ((DatosCPU*)item)->socketCPU==socketConectado;
				}), free);
				pthread_mutex_unlock(&mutexCPUsConectadas);
				break;
			case KILLPROGRAM:
				if(strcmp(paquete->header.emisor, CONSOLA) == 0)
				{
					pidAFinalizar = *(uint32_t*)paquete->Payload;
					bool finalizadoConExito = KillProgram(pidAFinalizar, DESCONECTADODESDECOMANDOCONSOLA);

					if(finalizadoConExito == true)
					{
						printf("El programa %d fue finalizado con exito\n", pidAFinalizar);

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
					printf("index semaforo encontrado: %i\n",index_semaforo_wait);

					Semaforo *semaforoEncontrado = list_get(Semaforos,index_semaforo_wait);
					pthread_mutex_unlock(&mutexIndiceSemaforoWait);

					pthread_mutex_lock(&mutexCPUsConectadas);
					DatosCPU* cpuActual = list_find(CPUsConectadas, LAMBDA(bool _(void* item) { return ((DatosCPU*) item)->socketCPU == socketConectado; }));
					cpuActual->isFree = true;
					pthread_mutex_unlock(&mutexCPUsConectadas);

					BloqueControlProceso* pcbRecibir = malloc(sizeof(BloqueControlProceso));
					RecibirPCB(pcbRecibir, paquete->Payload, paquete->header.tamPayload,KERNEL);
					pthread_mutex_lock(&mutexQueueEjecutando);

					BloqueControlProceso* pcbEliminadoDeEjecutando = list_remove_by_condition(Ejecutando->elements,  LAMBDA(bool _(void* item) {
						return ((BloqueControlProceso*)item)->PID == pcbRecibir->PID;
					}));

					pthread_mutex_unlock(&mutexQueueEjecutando);

					if(semaforoEncontrado != NULL)
					{
						pthread_mutex_lock(&mutexSemaforos);
						semaforoEncontrado->valorSemaforo--;
						//pthread_mutex_unlock(&mutexSemaforos);

						printf("Semaforo encontrado para wait: %s\n",semaforoEncontrado->nombreSemaforo);
						//pthread_mutex_unlock(&mutexSemaforos);


						printf("Valor semaforo dsps de decrementar %i\n",semaforoEncontrado->valorSemaforo);
						if (semaforoEncontrado->valorSemaforo < 0)
						{ //se bloquea
							printf("Se va a bloquear el proceso N° %u\n",pcbRecibir->PID);

							//Lo meto en la cola de los procesos bloqueados por el semaforo en cuestion
							uint32_t* pid = malloc(sizeof(uint32_t));
							*pid=pcbRecibir->PID;
							queue_push(semaforoEncontrado->listaDeProcesos, pid);

							if(pcbEliminadoDeEjecutando!=NULL) {
								//Lo meto en la cola de bloqueados
								pthread_mutex_lock(&mutexQueueBloqueados);
								queue_push(Bloqueados, pcbRecibir);
								pcb_Destroy(pcbEliminadoDeEjecutando);
								pthread_mutex_unlock(&mutexQueueBloqueados);
							}

							printf("Despues de bloquearse, el program counter del proceso N° %u es %u\n",pcbRecibir->PID,pcbRecibir->ProgramCounter);
							pthread_mutex_unlock(&mutexSemaforos);

							sem_post(&semDispatcherCpus);
							//Evento_ListosAdd();

						}
						else
						{

							pthread_mutex_unlock(&mutexSemaforos);

							/*pthread_mutex_lock(&mutexCPUsConectadas);
							DatosCPU* cpuActual = list_find(CPUsConectadas, LAMBDA(bool _(void* item) { return ((DatosCPU*) item)->socketCPU == socketConectado; }));
							cpuActual->isFree = true;
							pthread_mutex_unlock(&mutexCPUsConectadas);*/

							pthread_mutex_lock(&mutexQueueListos);
							list_add_in_index(Listos->elements, 0, pcbRecibir);
							pthread_mutex_unlock(&mutexQueueListos);
							sem_post(&semDispatcherCpus);
							Evento_ListosAdd();
						}

					}


				}
			break;

			case ESPCB:
				//termino la rafaga normalmente sin bloquearse
				if(strcmp(paquete->header.emisor, CPU) == 0)
				{
					pthread_mutex_lock(&mutexCPUsConectadas);
					DatosCPU* cpuActual = list_find(CPUsConectadas, LAMBDA(bool _(void* item) { return ((DatosCPU*) item)->socketCPU == socketConectado; }));
					pthread_mutex_unlock(&mutexCPUsConectadas);
					cpuActual->isFree = true;

					pthread_mutex_lock(&mutexQueueEjecutando);
					BloqueControlProceso* pcb = list_find(Ejecutando->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*) item)->PID == cpuActual->pid; }));
					pthread_mutex_unlock(&mutexQueueEjecutando);
					if(pcb!=NULL){
						RecibirPCB(pcb, paquete->Payload, paquete->header.tamPayload,KERNEL);

						if (pcb->ExitCode<= 0)
						{
							FinalizarPrograma(pcb->PID,pcb->ExitCode);
							//sem_post(&semDispatcherCpus);
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
								pcb_Destroy(pcbEjecutado);
							}
						}
					} else
						printf("Error al finalizar ejecucion");
				}
				break;
	}
}
