#include "Kernel.h"

int ultimoPID = 0;
int pidAFinalizar;
int socketConMemoria;
int socketConFS;
uint32_t TamanioPagina;


int cantProcesadores = 0;


//Variables archivo de configuracion
int PUERTO_PROG;
int PUERTO_CPU;
char* IP_MEMORIA;
int PUERTO_MEMORIA;
char* IP_FS;
int PUERTO_FS;
int QUANTUM;
int QUANTUM_SLEEP;
char* ALGORITMO;
int GRADO_MULTIPROG;
char* SEM_IDS[4];
int SEM_INIT[100];
char* SHARED_VARS[100];
int STACK_SIZE;
char* IP_PROG;

uint32_t PidAComparar;

t_list* Nuevos;
t_list* Finalizados;
t_list* Bloqueados;
t_list* Ejecutando;
t_list* Listos;
t_list* Estados;
t_list* ListaPCB;
t_list* EstadosConProgramasFinalizables;
t_list* PaginasPorProceso;
t_list* Paginas;
t_list* ArchivosGlobales;
t_list* ArchivosProcesos;
t_list* VariablesGlobales;
t_list* Semaforos;
t_list* PIDsPorSocketConsola;


int RecorrerHastaEncontrarUnMetadataUsed(void* datosPagina)
{
	bool encontroOcupado=false;
	uint32_t offsetOcupado=0;
	//Recorro hasta encontrar el primer bloque ocupado
		while(offsetOcupado<TamanioPagina-sizeof(HeapMetadata) && encontroOcupado == false){
			//Recorro el buffer obtenido
			HeapMetadata heapMD = *(HeapMetadata*)(datosPagina + offsetOcupado);
			if(heapMD.isFree==false){
				//Si encuentra un metadata free, freno
				encontroOcupado = true;
			}
			else{
				//Aumento el puntero de acuerdo al tamaño correspondiente al bloque existente
				offsetOcupado+=(sizeof(HeapMetadata)+ heapMD.size);
			}
		}
	return encontroOcupado;
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
	list_destroy_and_destroy_elements(PIDsPorSocketConsola, free);
}
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
	PIDsPorSocketConsola = list_create();
	list_add(EstadosConProgramasFinalizables,Nuevos);
	list_add(EstadosConProgramasFinalizables,Listos);
	list_add(EstadosConProgramasFinalizables,Ejecutando);
	list_add(EstadosConProgramasFinalizables,Bloqueados);
	list_add_all(Estados,EstadosConProgramasFinalizables);
	list_add(Estados,Finalizados);
}
BloqueControlProceso* FinalizarPrograma(t_list* lista,int pid,int tipoFinalizacion, int index, int socketFD)
{
	BloqueControlProceso* pcbRemovido = NULL;
	bool hayEstructurasNoLiberadas=false;
	pcbRemovido = (BloqueControlProceso*)list_remove_by_condition(lista, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid; }));
	if(pcbRemovido!=NULL) {
		pcbRemovido->ExitCode=tipoFinalizacion;
		list_add(Finalizados,pcbRemovido);
		//Analizo si el proceso tiene Memory Leaks o no
		t_list* pagesProcess= list_filter(PaginasPorProceso,
							LAMBDA(bool _(void* item) {return ((PaginaDelProceso*)item)->pid == pid;}));
		if(list_size(pagesProcess)>0){
			int i=0;
			while(i < list_size(pagesProcess) && hayEstructurasNoLiberadas==false){
				PaginaDelProceso* elem = (PaginaDelProceso*)list_get(PaginasPorProceso,i);
				//Me fijo si hay metadatas en estado "used" en cada pagina
				void* datosPagina = IM_LeerDatos(socketFD,KERNEL,elem->pid,elem->nroPagina,0,TamanioPagina);
				int result = RecorrerHastaEncontrarUnMetadataUsed(datosPagina);
				if(result>=0){
					//Hay algun metadata que no se libero
					hayEstructurasNoLiberadas=true;
				}
			}
			if(hayEstructurasNoLiberadas==true)
				printf("MEMORY LEAKS: El proceso %d no liberó todas las estructuras de memoria dinámica que solicitó",pid);
			else
				printf("El proceso %d liberó todas las estructuras de memoria dinamica",pid);
		}

		if(index == INDEX_LISTOS) {
			IM_FinalizarPrograma(socketFD, KERNEL, pid);
		}
	}
	return pcbRemovido;
}


char* ObtenerTextoDeArchivoSinCorchetes(FILE* f) //Para obtener los valores de los arrays del archivo de configuracion
{
	char buffer[10000];
	char *line = fgets(buffer,sizeof buffer,f);
	int length = string_length(line)-3;
	char *texto = string_substring(line,1,length);

	texto  = strtok(texto,",");

	return texto;
}


void ObtenerTamanioPagina(int socketFD)
{
	Paquete* datosInicialesMemoria = malloc(sizeof(Paquete));
	uint32_t datosRecibidos = RecibirPaqueteCliente(socketFD,KERNEL,datosInicialesMemoria);
	if(datosRecibidos>0){
		TamanioPagina = *((uint32_t*)datosInicialesMemoria->Payload);
	}
	free(datosInicialesMemoria->Payload+1);
	free(datosInicialesMemoria);

}

void MostrarProcesosDeUnaLista(t_list* lista,char* discriminator)
{
	printf("Procesos de la lista: %s \n",discriminator);
	int index=0;
	for (index = 0; index < list_size(lista); index++) {
		BloqueControlProceso* proceso = (BloqueControlProceso*)list_get(lista,index);

		if (strcmp(discriminator, FINALIZADOS) == 0) {
			printf("\tProceso N°: %d",proceso->PID);
			obtenerError(proceso->ExitCode);
		}
		else {
			printf("\tProceso N°: %d\n",proceso->PID);
		}
	}
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
		//printf("Indice de codigo: %d \n",proceso->IndiceDeCodigo);
		//printf("Tamaño del stack: %d \n",proceso->IndiceStack);
		printf("Paginas de codigo: %d \n",proceso->PaginasDeCodigo);
		printf("Contador de programa: %d \n",proceso->ProgramCounter);
	}
}
void obtenerValoresArchivoConfiguracion()
{
	int contadorDeVariables = 0;
	int c;
	FILE *file;

	file = fopen("ArchivoConfiguracion.txt", "r");

	if (file)
	{
		while ((c = getc(file)) != EOF)
		{
			if (c == '=')
			{
				if (contadorDeVariables == 14)
				{
					char buffer[10000];

					IP_PROG = fgets(buffer, sizeof buffer, file);
					strtok(IP_PROG, "\n");
					contadorDeVariables++;
				}

				if (contadorDeVariables == 13)
				{
					fscanf(file, "%i", &STACK_SIZE);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 12)
				{
					char* texto = ObtenerTextoDeArchivoSinCorchetes(file);
					int i = 0;

					while (texto != NULL)
					{
						SHARED_VARS[i] = texto;
						texto = strtok (NULL, ",");
						i++;
					}

					contadorDeVariables++;
				}

				if (contadorDeVariables == 11)
				{
					char* texto = ObtenerTextoDeArchivoSinCorchetes(file);
					int i = 0;

					while (texto != NULL)
					{
						SEM_INIT[i++] = atoi(texto);
						texto = strtok (NULL, ",");
					}

					contadorDeVariables++;
				}

				if (contadorDeVariables == 10){
					char * texto = ObtenerTextoDeArchivoSinCorchetes(file);
					int i = 0;

					while (texto != NULL)
					{
						SEM_IDS[i++] = texto;
						texto = strtok (NULL, ",");
					}

					contadorDeVariables++;
				}

				if (contadorDeVariables == 9)
				{
					fscanf(file, "%i", &GRADO_MULTIPROG);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 8)
				{
					char buffer[10000];

					ALGORITMO = fgets(buffer, sizeof buffer, file);
					strtok(ALGORITMO, "\n");

					contadorDeVariables++;
				}

				if (contadorDeVariables == 7)
				{
					fscanf(file, "%i", &QUANTUM_SLEEP);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 6)
				{
					fscanf(file, "%i", &QUANTUM);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 5)
				{
					fscanf(file, "%i", &PUERTO_FS);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 4)
				{
					char buffer[10000];

					IP_FS = fgets(buffer, sizeof buffer, file);
					strtok(IP_FS, "\n");

					contadorDeVariables++;
				}

				if (contadorDeVariables == 3)
				{
					fscanf(file, "%i", &PUERTO_MEMORIA);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 2)
				{
					char buffer[10000];

					IP_MEMORIA = fgets(buffer, sizeof buffer, file);
					strtok(IP_MEMORIA, "\n");

					contadorDeVariables++;
				}

				if (contadorDeVariables == 1)
				{
					fscanf(file, "%i", &PUERTO_CPU);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 0)
				{
					fscanf(file, "%i", &PUERTO_PROG);
					contadorDeVariables++;
				}
			}
		}

		fclose(file);
	}
}
void syscallWrite(int socketFD)
{
	if(socketFD == 1) {
		char orden[100];
		char texto[100];

		printf("\n\nIngrese una orden privilegiada: \n");
		scanf("%s", orden);
		char* command = getWord(orden, 0);

		if(strcmp(command,"write") == 0) {
			printf("\n\nIngrese texto a imprimir: \n");
			scanf("%s", texto);
			printf("%s", texto);
			char* imprimir;
			strcpy(imprimir,"imprimir ");
			string_append(&imprimir,texto);
			EnviarMensaje(socketFD,imprimir,KERNEL);
			free(imprimir);
		}
		else{
			printf("System Call no definida\n");
		}
	}
}
void MostrarTodosLosProcesos()
{
	MostrarProcesosDeUnaLista(Nuevos,NUEVOS);
	MostrarProcesosDeUnaLista(Listos,LISTOS);
	MostrarProcesosDeUnaLista(Ejecutando,EJECUTANDO);
	MostrarProcesosDeUnaLista(Bloqueados,BLOQUEADOS);
	MostrarProcesosDeUnaLista(Finalizados,FINALIZADOS);
}

void KillProgramDeUnaLista(t_list* lista,BloqueControlProceso* pcb,int tipoFinalizacion, int index, int socket)
{
	FinalizarPrograma(lista,pcb->PID,tipoFinalizacion, index, socket);
}
bool KillProgram(int pidAFinalizar,int tipoFinalizacion, int socket)
{
	int i =0;
	void* result = NULL;
	while(i<list_size(EstadosConProgramasFinalizables) && result==NULL){
		t_list* lista = list_get(EstadosConProgramasFinalizables,i);
		//TODO: Ver como eliminar un programa en estado ejecutando
		if(i!=2)  //2 == EJECUTANDO
			result = FinalizarPrograma(lista,pidAFinalizar,tipoFinalizacion, i, socket);
		i++;
	}
	if(result==NULL){
		printf("No se encontro el programa finalizar");
		return false;
	}
	return true;

}
void PonerElProgramaComoListo(BloqueControlProceso* pcb,Paquete* paquete,int socketFD,double tamanioTotalPaginas){

		pcb->PaginasDeCodigo = (uint32_t)tamanioTotalPaginas;
		printf("Cant paginas asignadas para el codigo: %d \n",pcb->PaginasDeCodigo);
		//Saco el programa de la lista de NEW y  agrego el programa a la lista de READY
		list_remove_by_condition(Nuevos, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pcb->PID; }));
		list_add(Listos, pcb);
		printf("El programa %d se cargo en memoria \n",pcb->PID);
}
void GuardarCodigoDelProgramaEnLaMemoria(BloqueControlProceso* bcp,Paquete* paquete){
	int i,j=0;
	for (i = 0; i < bcp->PaginasDeCodigo; i++) {
		char* str;
		if(i+1 != bcp->PaginasDeCodigo){
			str = string_substring((char*)paquete->Payload,j,TamanioPagina);
			j+=TamanioPagina;
		}
		else
		{
			//Ultima pagina de codigo
			 str = string_substring_from((char*)paquete->Payload,j);
		}
		IM_GuardarDatos(socketConMemoria, KERNEL, bcp->PID, i, 0, string_length(str)+1, str);

	}
}
void CargarInformacionDelCodigoDelPrograma(BloqueControlProceso* pcb,Paquete* paquete){
	t_metadata_program* metaProgram = metadata_desde_literal((char*)paquete->Payload);
	int i=0;

	while(i<metaProgram->instrucciones_size){
		int *registroIndice;
		registroIndice[0]= metaProgram->instrucciones_serializado[i].start;
		registroIndice[1]= metaProgram->instrucciones_serializado[i].offset;
		list_add(pcb->IndiceDeCodigo,registroIndice);
		i++;
	}
	//Inicializar indice de etiquetas
	i=0;
	// Leer metadataprogram.c a ver como desarrollaron esto
	while(i<metaProgram->etiquetas_size){
		char* etiquetaABuscar;
		while(metaProgram->etiquetas[i]!=metaProgram->instrucciones_size){
			etiquetaABuscar[i]=metaProgram->etiquetas[i];
			i++;
		}
		t_puntero_instruccion pointer = metadata_buscar_etiqueta(etiquetaABuscar,metaProgram->etiquetas,metaProgram->etiquetas_size);
		dictionary_put(pcb->IndiceDeEtiquetas,etiquetaABuscar,&pointer);
	}
}

void AgregarAListadePidsPorSocket(uint32_t PID, int socket)
{
	PIDporSocketConsola* PIDxSocket = malloc(sizeof(uint32_t));

	PIDxSocket->PID = PID;
	PIDxSocket->socketConsola = socket;

	list_add(PIDsPorSocketConsola, PIDxSocket);
}

void accion(Paquete* paquete, int socketConectado){
	/*pthread_t hiloSyscallWrite;
	pthread_create(&hiloSyscallWrite,NULL, (void*)syscallWrite, &socketConectado);*/
	switch(paquete->header.tipoMensaje) {

		case ESSTRING:

				if(strcmp(paquete->header.emisor, CONSOLA)==0)
				{
					double tamaniCodigoEnPaginas = paquete->header.tamPayload/TamanioPagina;
					double tamanioCodigoYStackEnPaginas = ceil(tamaniCodigoEnPaginas)+STACK_SIZE;
					BloqueControlProceso* pcb = malloc(sizeof(BloqueControlProceso));
					//TODO: Falta free, pero OJO, hay que ver la forma de ponerlo y que siga andando
					CrearNuevoProceso(pcb,&ultimoPID,Nuevos);

					AgregarAListadePidsPorSocket(pcb->PID, socketConectado);

					//Manejo la multiprogramacion
					if(GRADO_MULTIPROG - list_size(Ejecutando) - list_size(Listos) > 0 && list_size(Nuevos) >= 1){
						//Pregunta a la memoria si me puede guardar estas paginas
						bool pudoCargarse = IM_InicializarPrograma(socketConMemoria,KERNEL,pcb->PID,tamanioCodigoYStackEnPaginas);
						if(pudoCargarse == true)
						{
							PonerElProgramaComoListo(pcb,paquete,socketConectado,tamaniCodigoEnPaginas);
							//Ejecuto el metadata program
							CargarInformacionDelCodigoDelPrograma(pcb,paquete);

							//Solicito a la memoria que me guarde el codigo del programa(dependiendo cuantas paginas se requiere para el codigo
							GuardarCodigoDelProgramaEnLaMemoria(pcb,paquete);
							EnviarDatos(socketConectado,KERNEL,&(pcb->PID),sizeof(uint32_t));
							//TODO: Ejecutar en alguna CPU(Enviar PCB)

						}
						else
						{
							//Sacar programa de la lista de nuevos y meterlo en la lista de finalizado con su respectivo codigo de error
							KillProgramDeUnaLista(Nuevos,pcb,NOSEPUDIERONRESERVARRECURSOS, INDEX_NUEVOS, socketConectado);
							EnviarMensaje(socketConectado,"No se pudo guardar el programa porque no hay espacio suficiente",KERNEL);
						}
					}
					else
					{
						//El grado de multiprogramacion no te deja agregar otro proceso
						KillProgramDeUnaLista(Nuevos,pcb,NOSEPUDIERONRESERVARRECURSOS, INDEX_NUEVOS, socketConectado);
						EnviarMensaje(socketConectado,"No se pudo guardar el programa porque se alcanzo el grado de multiprogramacion",KERNEL);

					}

				}

				if(strcmp(paquete->header.emisor, CPU)==0)
				{
					switch (paquete->header.tipoMensaje)
					{
						case ESDATOS:
							switch ((*(uint32_t*)paquete->Payload))
							{
								uint32_t valorADevolver;
								uint32_t valorAAsignar;
								char* variableCompartida;
								uint32_t PID;
								void* result;
								char* nombreSem;
								uint32_t tamanioAReservar;
								case PEDIRSHAREDVAR:

								break;

								case ASIGNARSHAREDVAR:
									PID = ((uint32_t*)paquete->Payload)[1];
									valorAAsignar = ((uint32_t*)paquete->Payload)[2];
									strcpy(variableCompartida, (char*)(paquete->Payload+sizeof(uint32_t) * 2));

									/*char* contenidoVariableCompartida;
									strcpy(contenidoVariableCompartida, SHARED_VARS[0]);

									int i = 0;

									while(strcmp(contenidoVariableCompartida, "") != 0)
									{
										i++;
										strcpy(contenidoVariableCompartida, SHARED_VARS[i]);
									}

									strcpy(SHARED_VARS[i], variableCompartida);*/


								break;

								case WAITSEM:
									PID = ((uint32_t*)paquete->Payload)[sizeof(uint32_t)];
									strcpy(nombreSem, (char*)(paquete->Payload+sizeof(uint32_t) * 2));

									result = NULL;

									result = list_find(Semaforos, LAMBDA(bool _(void* item) { return ((semaforo*) item)->nombreSemaforo == nombreSem; }));

									if(result == NULL) //No hay semaforo con ese nombre
									{
										int tamDatos = sizeof(uint32_t) * 2;
										void* datos = malloc(tamDatos);

										((uint32_t*) datos)[0] = WAITSEM;
										((uint32_t*) datos)[1] = -1;

										EnviarDatos(socketConectado, KERNEL, datos, tamDatos);

										free(datos);
									}
									else
									{
										semaforo* semaf = (semaforo*)result;

										if(semaf->valorSemaforo >= 1)
										{
											semaf->valorSemaforo--;

											int tamDatos = sizeof(uint32_t) * 2;
											void* datos = malloc(tamDatos);

											((uint32_t*) datos)[0] = WAITSEM;
											((uint32_t*) datos)[1] = semaf->valorSemaforo;

											EnviarDatos(socketConectado, KERNEL, datos, tamDatos);

											free(datos);
										}
										else
										{
											int tamDatos = sizeof(uint32_t) * 2;
											void* datos = malloc(tamDatos);

											((uint32_t*) datos)[0] = WAITSEM;
											((uint32_t*) datos)[1] = -2;

											EnviarDatos(socketConectado, KERNEL, datos, tamDatos);

											free(datos);
										}
									}


								break;

								case SIGNALSEM:
									PID = ((uint32_t*)paquete->Payload)[1];
									strcpy(nombreSem, (char*)(paquete->Payload+sizeof(uint32_t)));
									 result = NULL;

									result = (semaforo*) list_find(Semaforos, LAMBDA(bool _(void* item) { return ((semaforo*) item)->nombreSemaforo == nombreSem; }));

									if(result == NULL) //No hay semaforo con ese nombre
									{
										int tamDatos = sizeof(uint32_t) * 2;
										void* datos = malloc(tamDatos);

										((uint32_t*) datos)[0] = SIGNALSEM;
										((uint32_t*) datos)[1] = -1;

										EnviarDatos(socketConectado, KERNEL, datos, tamDatos);

										free(datos);
									}
									else
									{
										semaforo* semaf = (semaforo*)result;

										semaf->valorSemaforo++;

										int tamDatos = sizeof(uint32_t) * 2;
										void* datos = malloc(tamDatos);

										((uint32_t*) datos)[0] = WAITSEM;
										((uint32_t*) datos)[1] = semaf->valorSemaforo;

										EnviarDatos(socketConectado, KERNEL, datos, tamDatos);

										free(datos);
									}


								break;

								case RESERVARHEAP:
									PID = ((uint32_t*)paquete->Payload)[1];
									tamanioAReservar = ((uint32_t*)paquete->Payload)[2];

									uint32_t punteroADevolver = SolicitarHeap(PID, tamanioAReservar, socketConectado);

									int tamDatos = sizeof(uint32_t) * 2;
									void* datos = malloc(tamDatos);

									((uint32_t*) datos)[0] = RESERVARHEAP;
									((uint32_t*) datos)[1] = punteroADevolver;

									EnviarDatos(socketConectado, KERNEL, datos, tamDatos);

									free(datos);


								break;

								case LIBERARHEAP:
									 PID = ((uint32_t*)paquete->Payload)[1];
									uint32_t punteroALiberar = ((uint32_t*)paquete->Payload)[2];
									PosicionDeMemoria pos;
									pos.NumeroDePagina = punteroALiberar/TamanioPagina;
									pos.Offset = punteroALiberar % TamanioPagina;
									SolicitudLiberacionDeBloque(socketConectado, PID, pos);

									//SolicitudLiberacion no hace ningun return ni validacion, habria que hacer algo ahi
									//asi puedo puedo ponerle un valor a valorADevolver

									 tamDatos = sizeof(uint32_t) * 2;
									 datos = malloc(tamDatos);

									((uint32_t*) datos)[0] = RESERVARHEAP;
									((uint32_t*) datos)[1] = valorADevolver;

									EnviarDatos(socketConectado, KERNEL, datos, tamDatos);


								break;

								case ABRIRARCHIVO:
									PID = ((uint32_t*)paquete->Payload)[1];
									bool* flagCreacion = ((bool*)(paquete->Payload+sizeof(uint32_t) * 2));
									//Hacer que los permisos sean char[3], hablar con uri
									char* path = ((char**)paquete->Payload)[sizeof(uint32_t) * 2 + sizeof(bool)];

									abrirArchivo(path, PID, flagCreacion);




								break;

								case BORRARARCHIVO:
									PID = ((int*)paquete->Payload)[1];

								break;

								case CERRARARCHIVO:

								break;

								case MOVERCURSOSARCHIVO:

								break;

								case ESCRIBIRARCHIVO:

								break;

								case LEERARCHIVO:

								break;

								case FINEJECUCIONPROGRAMA:

								break;
							}
						break;
					}
				}
		break;

		case KILLPROGRAM:
			if(strcmp(paquete->header.emisor, CONSOLA) == 0)
			{
				pidAFinalizar = *(uint32_t*)paquete->Payload;
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
	}
//	pthread_join(hiloSyscallWrite, NULL);

}

void RecibirHandshake_KernelDeMemoria(int socketFD, char emisor[11]) {
	Paquete* paquete =  malloc(sizeof(Paquete));
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0 && paquete->header.tipoMensaje == ESHANDSHAKE) { //si no hubo error y es un handshake
		if (strcmp(paquete->header.emisor, emisor) == 0) {
				printf("\nConectado con el servidor!\n");
				if(strcmp(paquete->header.emisor, MEMORIA) == 0){
					paquete->Payload = malloc(paquete->header.tamPayload);
					resul = RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
					TamanioPagina = *((uint32_t*)paquete->Payload);
					free(paquete->Payload);
				}
		} else
			perror("Error, no se recibio un handshake del servidor esperado\n");
	} else
	perror("Error de Conexion, no se recibio un handshake\n");


	free(paquete);
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
				MostrarProcesosDeUnaLista(Nuevos,NUEVOS);
			else if(strcmp(lista,LISTOS)==0)
				MostrarProcesosDeUnaLista(Listos,LISTOS);
			else if(strcmp(lista,EJECUTANDO)==0)
				MostrarProcesosDeUnaLista(Ejecutando,EJECUTANDO);
			else if(strcmp(lista,BLOQUEADOS)==0)
				MostrarProcesosDeUnaLista(Bloqueados,BLOQUEADOS);
			else if(strcmp(lista,FINALIZADOS)==0)
				MostrarProcesosDeUnaLista(Finalizados,FINALIZADOS);

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
			bool finalizadoOK = KillProgram(pidConsulta, DESCONECTADODESDECOMANDOCONSOLA, *socketFD);
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
		else {
			printf("No se conoce el mensaje %s\n", orden);
		}
	}
}

int RecibirPaqueteServidorKernel(int socketFD, char receptor[11], Paquete* paquete) {
	paquete->Payload = malloc(1);
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0) { //si no hubo error
		if (paquete->header.tipoMensaje == ESHANDSHAKE) { //vemos si es un handshake
			printf("Se establecio conexion con %s\n", paquete->header.emisor);

			if(strcmp(paquete->header.emisor, CPU) == 0)
			{
				cantProcesadores++;
			}

			EnviarHandshake(socketFD, receptor); // paquete->header.emisor
		} else { //recibimos un payload y lo procesamos (por ej, puede mostrarlo)
			paquete->Payload = realloc(paquete->Payload,
					paquete->header.tamPayload);
			resul = RecibirDatos(paquete->Payload, socketFD,
					paquete->header.tamPayload);
		}
	}

	return resul;
}


int main(void)
{
	CrearListas(); //TODO: Cargar listas de semaforos y variables globales
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	while((socketConMemoria = ConectarAServidor(PUERTO_MEMORIA,IP_MEMORIA,MEMORIA,KERNEL, RecibirHandshake_KernelDeMemoria))<0);
	while((socketConFS = ConectarAServidor(PUERTO_FS,IP_FS,FS,KERNEL, RecibirHandshake))<0);

	//pthread_t hiloSyscallWrite;
	//pthread_create(&hiloSyscallWrite, NULL, (void*)syscallWrite, 2); //socket 2 hardcodeado
	//pthread_join(hiloSyscallWrite, NULL);

	pthread_t hiloConsola;
	pthread_create(&hiloConsola, NULL, (void*)userInterfaceHandler, &socketConMemoria);
	Servidor(IP_PROG, PUERTO_PROG, KERNEL, accion, RecibirPaqueteServidorKernel);
	pthread_join(hiloConsola, NULL);
	LimpiarListas();

	return 0;
}
