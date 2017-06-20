#include "Threads.h"

//Tipos de ExitCode
#define FINALIZACIONNORMAL 0
#define NOSEPUDIERONRESERVARRECURSOS -1
#define DESCONECTADODESDECOMANDOCONSOLA -7
#define SOLICITUDMASGRANDEQUETAMANIOPAGINA -8

int ultimoPID = 0;

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

	while(i<string_length(metaProgram->etiquetas)){
		int *registroIndice = malloc(sizeof(uint32_t)*2);
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

BloqueControlProceso* FinalizarPrograma(int PIDAFinalizar, int tipoFinalizacion, int index, int socketFD)
{
	BloqueControlProceso* pcbRemovido = NULL;
	bool hayEstructurasNoLiberadas = false;

	finalizarProgramaCapaFS(PIDAFinalizar);

	//Aca hace la liberacion de memoria uri, fijate si podes hacer una funcion finalizarProgramaCapaMemoria, sino hace aca normal


	pcbRemovido = (BloqueControlProceso*)list_remove_by_condition(lista, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid; }));

	if(pcbRemovido!=NULL) {
		pcbRemovido->ExitCode = tipoFinalizacion;

		list_add(Finalizados, pcbRemovido);

		//Analizo si el proceso tiene Memory Leaks o no
		t_list* pagesProcess= list_filter(PaginasPorProceso,
							LAMBDA(bool _(void* item) {return ((PaginaDelProceso*)item)->pid == PIDAFinalizar;}));

		if(list_size(pagesProcess) > 0)
		{
			int i = 0;

			while(i < list_size(pagesProcess) && hayEstructurasNoLiberadas == false)
			{
				PaginaDelProceso* elem = (PaginaDelProceso*)list_get(PaginasPorProceso, i);

				//Me fijo si hay metadatas en estado "used" en cada pagina
				void* datosPagina = IM_LeerDatos(socketFD, KERNEL, elem->pid, elem->nroPagina, 0, TamanioPagina);
				int result = RecorrerHastaEncontrarUnMetadataUsed(datosPagina);

				if(result >= 0)
				{
					//Hay algun metadata que no se libero
					hayEstructurasNoLiberadas=true;
				}
			}

			if(hayEstructurasNoLiberadas == true)
			{
				printf("MEMORY LEAKS: El proceso %d no liberó todas las estructuras de memoria dinámica que solicitó", PIDAFinalizar);
			}
			else
			{
				printf("El proceso %d liberó todas las estructuras de memoria dinamica", PIDAFinalizar);
			}
		}

		if(index == INDEX_LISTOS)
		{
			IM_FinalizarPrograma(socketFD, KERNEL, PIDAFinalizar);
		}
	}
	return pcbRemovido;
}

bool KillProgram(int pidAFinalizar,int tipoFinalizacion, int socket)
{ //TODO: hacer de nuevo. sin usar una lista EstadosConProgramasFinalizables
	int i =0;
	void* result = NULL;
	while(i<list_size(EstadosConProgramasFinalizables) && result == NULL){
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

int RecibirPaqueteServidorKernel(int socketFD, char receptor[11], Paquete* paquete) {
	paquete->Payload = malloc(1);
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0) { //si no hubo error
		if (paquete->header.tipoMensaje == ESHANDSHAKE) { //vemos si es un handshake
			printf("Se establecio conexion con %s\n", paquete->header.emisor);

			if(strcmp(paquete->header.emisor, CPU) == 0)
			{
				DatosCPU* disp = malloc(sizeof(DatosCPU));

				disp->isFree = true;
				disp->socketCPU = socketFD;

				list_add(CPUsConectadas, disp);
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

void mandarAEjecutar(uint32_t cantidadDeRafagas) {


}

void dispatcher() {
	/*Bloqueados;
	Ejecutando;
	Listos;
	CPUSDisponibles;
	disp->isFree = true;
	disp->socketCPU = socketFD;*/
	while (true) {
		t_list listCPUsLibres = list_filter(CPUsConectadas, LAMBDA(bool _(void* item) { return ((DatosCPU*)item)->isFree == true;}));
		int i;
		for (i = 0; i < list_size(listCPUsLibres); i++) {
			BloqueControlProceso* PCBAMandar = (BloqueControlProceso*)queue_pop(Listos);
			DatosCPU* cpuAUsar = (DatosCPU*)list_get(listCPUsLibres, 0);
			uint32_t cantidadDeRafagas;
			if (strcmp(ALGORITMO, "FIFO") == 0) {
				cantidadDeRafagas = PCBAMandar->IndiceDeCodigo->elements_count - PCBAMandar->cantidadDeRafagasEjecutadas;
			}
			else if (strcmp(ALGORITMO, "RR") == 0){
				cantidadDeRafagas = QUANTUM;
			}
			pcb_Send(cpuAUsar, KERNEL, PCBAMandar, cantidadDeRafagas);
			cpuAUsar->isFree = false;
			queue_push(Ejecutando, PCBAMandar);
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

void* accion(void* socket){
	int socketConectado = *(int*)socket;
	Paquete paquete;
	while (RecibirPaqueteServidorKernel(socketConectado, KERNEL, &paquete) > 0) {
		switch(paquete.header.tipoMensaje) {

			case ESSTRING:

					if(strcmp(paquete.header.emisor, CONSOLA)==0)
					{
						double tamaniCodigoEnPaginas = paquete.header.tamPayload/TamanioPagina;
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
								PonerElProgramaComoListo(pcb,&paquete,socketConectado,tamaniCodigoEnPaginas);
								//Ejecuto el metadata program
								CargarInformacionDelCodigoDelPrograma(pcb,&paquete);

								//Solicito a la memoria que me guarde el codigo del programa(dependiendo cuantas paginas se requiere para el codigo
								GuardarCodigoDelProgramaEnLaMemoria(pcb,&paquete);
								EnviarDatos(socketConectado,KERNEL,&(pcb->PID),sizeof(uint32_t));
								//TODO: Ejecutar en alguna CPU(Enviar PCB)

							}
							else
							{
								//Sacar programa de la lista de nuevos y meterlo en la lista de finalizado con su respectivo codigo de error
								FinalizarPrograma(Nuevos,pcb->PID,NOSEPUDIERONRESERVARRECURSOS, INDEX_NUEVOS, socketConectado);
								EnviarMensaje(socketConectado,"No se pudo guardar el programa porque no hay espacio suficiente",KERNEL);
							}
						}
						else
						{
							//El grado de multiprogramacion no te deja agregar otro proceso
							FinalizarPrograma(Nuevos,pcb->PID,NOSEPUDIERONRESERVARRECURSOS, INDEX_NUEVOS, socketConectado);
							EnviarMensaje(socketConectado,"No se pudo guardar el programa porque se alcanzo el grado de multiprogramacion",KERNEL);

						}

					}
					break;

					case ESDATOS:
						if(strcmp(paquete.header.emisor, CPU) == 0)
						{
							switch ((*(uint32_t*)paquete.Payload))
							{
								int32_t valorADevolver;
								int32_t valorAAsignar;
								char* variableCompartida;
								uint32_t PID;
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
									result = list_find(VariablesCompartidas,
										LAMBDA(bool _(void* item) {
											return strcmp(((VariableCompartida*)item)->nombreVariableGlobal, variableCompartida) == 0;
									}));

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
									result = list_find(VariablesCompartidas,
										LAMBDA(bool _(void* item) {
											return strcmp(((VariableCompartida*)item)->nombreVariableGlobal, variableCompartida) == 0; }));

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

									result = list_find(Semaforos, LAMBDA(bool _(void* item) { return ((Semaforo*) item)->nombreSemaforo == nombreSem; }));

									if(result != NULL)
									{
										Semaforo* semaf = (Semaforo*)result;
										semaf->valorSemaforo--;
										list_add(semaf->listaDeProcesos, &PID);
										BloqueControlProceso* pcb = malloc(sizeof(BloqueControlProceso));
										DatosCPU* cpu = list_find(CPUsConectadas, LAMBDA(bool _(void* item) {
											return ((DatosCPU*)item)->socketCPU == socketConectado;
										}));
										pcb_Receive(pcb, socketConectado, cpu);
										list_remove_by_condition(Ejecutando,  LAMBDA(bool _(void* item) {
											return ((BloqueControlProceso*)item)->PID == pcb->PID;
										}));
										queue_push(Bloqueados, pcb);
									}
									else
									{
										FinalizarPrograma(PID, ERRORSINDEFINIR, INDEX_EJECUTANDO, socketConMemoria);
									}

								break;

								case SIGNALSEM:
									PID = ((uint32_t*)paquete.Payload)[1];
									strcpy(nombreSem, (char*)(paquete.Payload+sizeof(uint32_t)*2));
									result = NULL;

									result = (Semaforo*) list_find(Semaforos, LAMBDA(bool _(void* item) { return ((Semaforo*) item)->nombreSemaforo == nombreSem; }));

									if(result != NULL)
									{
										Semaforo* semaf = (Semaforo*)result;
										semaf->valorSemaforo++;
										list_remove_by_condition(semaf->listaDeProcesos, LAMBDA(bool _(void* item) { return ((uint32_t*) item) == PID; }));
										list_remove_by_condition(Bloqueados, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*) item)->PID == PID; }));
									}
									else
									{
										FinalizarPrograma(PID, ERRORSINDEFINIR, INDEX_EJECUTANDO, socketConMemoria);
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

									permisos.creacion = ((bool*)paquete.Payload)[sizeof(uint32_t) * 2];
									permisos.escritura = ((bool*)paquete.Payload)[sizeof(uint32_t) * 3];
									permisos.lectura = ((bool*)paquete.Payload)[sizeof(uint32_t) * 4];
									char* path = ((char**)paquete.Payload)[sizeof(uint32_t) * 2 + sizeof(bool) * 3];

									abrirArchivo(path, PID, permisos);
								break;

								case BORRARARCHIVO:
									PID = ((uint32_t*)paquete.Payload)[1];

									uint32_t FD = ((uint32_t*)paquete.Payload)[sizeof(uint32_t) * 2];

									borrarArchivo(FD, PID);
								break;

								case CERRARARCHIVO:
									PID = ((uint32_t*)paquete.Payload)[1];

									uint32_t FD = ((uint32_t*)paquete.Payload)[sizeof(uint32_t) * 2];

									cerrarArchivo(FD, PID);
								break;

								case MOVERCURSOSARCHIVO:
									PID = ((uint32_t*)paquete.Payload)[1];

									uint32_t FD = ((uint32_t*)paquete.Payload)[sizeof(uint32_t) * 2];

									uint32_t posicion = ((uint32_t*)paquete.Payload)[sizeof(uint32_t) * 3];

									moverCursor(FD, PID, posicion);
								break;

								case ESCRIBIRARCHIVO:
									PID = ((uint32_t*)paquete.Payload)[1];

									uint32_t FD = ((uint32_t*)paquete.Payload)[sizeof(uint32_t) * 2];

									uint32_t tamanioArchivo = ((uint32_t*)paquete.Payload)[sizeof(uint32_t) * 3];

									char* datosAEscribir = ((char**)paquete.Payload)[sizeof(uint32_t) * 4];

									escribirArchivo(FD, PID, tamanioArchivo, datosAEscribir);
								break;

								case LEERARCHIVO:
									PID = ((uint32_t*)paquete.Payload)[1];

									uint32_t FD = ((uint32_t*)paquete.Payload)[sizeof(uint32_t) * 2];

									uint32_t tamanioArchivo = ((uint32_t*)paquete.Payload)[sizeof(uint32_t) * 3];

									leerArchivo(FD, PID, tamanioArchivo);
								break;

								case FINEJECUCIONPROGRAMA:
									PID = ((uint32_t*)paquete.Payload)[1];

									//Finalizar programa
								break;
							}
						}
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
						DatosCPU* cpu = list_find(CPUsConectadas, LAMBDA(bool _(void* item) {
							return ((DatosCPU*)item)->socketCPU == socketConectado;
						}));
						pcb_Receive(pcb, socketConectado, cpu);
						list_remove_by_condition(Ejecutando,  LAMBDA(bool _(void* item) {
							return ((BloqueControlProceso*)item)->PID == pcb->PID;
						}));
						if (pcb->IndiceDeCodigo->elements_count == pcb->cantidadDeRafagasEjecutadas) {
							FinalizarPrograma(pcb->PID, FINALIZACIONNORMAL, INDEX_EJECUTANDO, socketConMemoria);
						}
						else {
							queue_push(Listos, pcb);
						}

					}
		}

		free(paquete.Payload);
	}

	close(socketConectado);

	return NULL;

}
