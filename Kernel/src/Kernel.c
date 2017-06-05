#include "SocketsL.h"
#include "Helper.h"
#define NUEVOS "NUEVOS"
#define LISTOS "LISTOS"
#define EJECUTANDO "EJECUTANDO"
#define BLOQUEADOS "BLOQUEADOS"
#define FINALIZADOS "FINALIZADOS"


#define BACKLOG 6 //Backlog es el maximo de peticiones pendientes
//Tipos de ExitCode
#define NOSEPUDIERONRESERVARRECURSOS -1
#define DESCONECTADODESDECOMANDOCONSOLA -7
#define SOLICITUDMASGRANDEQUETAMANIOPAGINA -8

#define SIZEMETADATA 5

#define INDEX_NUEVOS 0
#define INDEX_LISTOS 1
#define INDEX_EJECUTANDO 2
#define INDEX_BLOQUEADOS 3
#define INDEX_FINALIZADOS 4
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


int pidAFinalizar;
int ultimoPID=0;
int socketConMemoria;
double TamanioPagina;
typedef struct  {
 uint32_t size;
 bool isFree;
} HeapMetadata;

typedef struct {
	FILE * file;
	BloqueControlProceso pcb;
} PeticionTamanioStack;

typedef struct {
	uint32_t pid;
	uint32_t nroPagina;
	uint32_t espacioDisponible;
} PaginaDelProceso;

typedef struct {
	uint32_t numero;
	t_list* HeapsMetadata;
} Pagina;


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

BloqueControlProceso* FinalizarPrograma(t_list* lista,int pid,int tipoFinalizacion, int index, int socket) {
	BloqueControlProceso* pcbRemovido = NULL;
	pcbRemovido = (BloqueControlProceso*)list_remove_by_condition(lista, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid; }));
	if(pcbRemovido!=NULL) {
		pcbRemovido->ExitCode=tipoFinalizacion;
		list_add(Finalizados,pcbRemovido);

		/*if(index == INDEX_LISTOS) {
			IM_FinalizarPrograma(socket, KERNEL, pid);
		}*/
	}
	return pcbRemovido;
}
void ActualizarMetadata(uint32_t PID,uint32_t nroPagina,uint32_t cantAReservar,uint32_t socketFD){
	void* datosPagina = IM_LeerDatos(socket,KERNEL,PID,nroPagina,0,TamanioPagina);
	void* metadataOcupado = malloc(sizeof(uint32_t)+sizeof(bool));
	void* metadataLibre = malloc(sizeof(uint32_t)+sizeof(bool));
	uint32_t offset = 0;
	uint32_t sizeBloque;
	bool frenar = false;
	while(offset<TamanioPagina-SIZEMETADATA){
		//Recorro el buffer obtenido
		void* size= malloc(sizeof(uint32_t));
		void* estado= malloc(sizeof(bool));
		memcpy(size,datosPagina,sizeof(uint32_t));
		memcpy(estado,datosPagina+4,sizeof(bool));
		bool isFree = *(bool*)estado;
		sizeBloque = *(uint32_t*)size;
		//Aumento el puntero de acuerdo al tamaño correspondiente al bloque existente
		offset+=(sizeBloque+sizeof(bool));
	}

	//Actualizo el metadata de acuerdo a la cantidad de bytes a reservar
	memcpy(metadataOcupado,cantAReservar,sizeof(uint32_t));
	memcpy(metadataOcupado+sizeof(uint32_t),true,sizeof(bool));
	IM_GuardarDatos(socketFD,KERNEL,PID,nroPagina,offset,sizeof(uint32_t)+sizeof(bool),metadataOcupado);

	uint32_t offsetAGuardarMetadataLibre = offset+sizeof(uint32_t)+sizeof(bool)+cantAReservar;
	//Creo el metadata para lo restante FREE
	memcpy(metadataLibre,sizeBloque-cantAReservar,sizeof(uint32_t));
	memcpy(metadataLibre+sizeof(uint32_t),false,sizeof(bool));
	IM_GuardarDatos(socketFD,KERNEL,PID,nroPagina,offsetAGuardarMetadataLibre,sizeof(uint32_t)+sizeof(bool),metadataLibre);


}

void SolicitarHeap(uint32_t PID,uint32_t cantAReservar,uint32_t socket){
	uint32_t cantTotal = cantAReservar+SIZEMETADATA;
	//Como maximo, podes solicitar reservar: TamañoPagina -10(correspondiente a los metedatas iniciales)
	if(cantTotal <= TamanioPagina-SIZEMETADATA*2){
		void* result = NULL;
		//Busco en las paginas asignadas a ese proceso, a ver si hay alguna con tamaño suficiente
		result = list_find(PaginasPorProceso,
				LAMBDA(bool _(void* item) {
			return ((PaginaDelProceso*)item)->pid == PID && ((PaginaDelProceso*)item)->espacioDisponible >= cantTotal;
		}));
		if(result!=NULL){ 		//Se encontro la pagina con tamaño disponible
			//Actualizar el tamaño disponible
			PaginaDelProceso* paginaObtenida = (PaginaDelProceso*)result;
			paginaObtenida->espacioDisponible -= cantTotal;
			//Obtengo la pagina en cuestion y actualizo el metadata
			ActualizarMetadata(PID,paginaObtenida->nroPagina,cantTotal,socket);

		}
		else  		//No hay una pagina del proceso utilizable
		{
			//Obtengo el ultimo numero de pagina, de ese PID
			t_list* lista= list_filter(PaginasPorProceso,
					LAMBDA(bool _(void* item) {return ((PaginaDelProceso*)item)->pid == PID;}));
			int i=0;
			int maximoNroPag=0;
			for (i = 0; i < list_size(lista); i++) {
				PaginaDelProceso* elem = (PaginaDelProceso*)list_get(PaginasPorProceso,i);
				if(maximoNroPag< elem->nroPagina)	maximoNroPag = elem->nroPagina;
			}
			PaginaDelProceso nuevaPPP;
			nuevaPPP.nroPagina = maximoNroPag+1;
			nuevaPPP.espacioDisponible = TamanioPagina;
			nuevaPPP.pid = PID;
			IM_AsignarPaginas(socket,KERNEL,PID,1);
			ActualizarMetadata(PID,nuevaPPP.nroPagina,cantTotal,socket);

		}
	}
	else{ //Debe finalizar el programa

		FinalizarPrograma(Ejecutando,PID,SOLICITUDMASGRANDEQUETAMANIOPAGINA,INDEX_EJECUTANDO,socket);
		//TODO: Avisar a la CPU del programa finalizado
	}


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


void ObtenerTamanioPagina(int socketFD){
	Paquete* datosInicialesMemoria = malloc(sizeof(Paquete));
	uint32_t datosRecibidos = RecibirPaqueteCliente(socketFD,KERNEL,datosInicialesMemoria);
	if(datosRecibidos>0){
		TamanioPagina = *((uint32_t*)datosInicialesMemoria->Payload);
	}
	free(datosInicialesMemoria->Payload+1);
	free(datosInicialesMemoria);

}
void CrearListasEstados(){
	Nuevos = list_create();
	Finalizados= list_create();
	Bloqueados= list_create();
	Ejecutando= list_create();
	Listos= list_create();
	//Creo una lista de listas
	Estados = list_create();
	EstadosConProgramasFinalizables = list_create();
	list_add(EstadosConProgramasFinalizables,Nuevos);
	list_add(EstadosConProgramasFinalizables,Listos);
	list_add(EstadosConProgramasFinalizables,Ejecutando);
	list_add(EstadosConProgramasFinalizables,Bloqueados);
	list_add_all(Estados,EstadosConProgramasFinalizables);
	list_add(Estados,Finalizados);
}
void LimpiarListas(){
	list_destroy_and_destroy_elements(Nuevos,free);
	list_destroy_and_destroy_elements(Listos,free);
	list_destroy_and_destroy_elements(Ejecutando,free);
	list_destroy_and_destroy_elements(Bloqueados,free);
	list_destroy_and_destroy_elements(Finalizados,free);
	list_destroy_and_destroy_elements(Estados,free);
	list_destroy_and_destroy_elements(EstadosConProgramasFinalizables,free);
}
void CrearNuevoProceso(BloqueControlProceso* pcb){
	//Creo el pcb y lo guardo en la lista de nuevos

	pcb->PID = ultimoPID+1;
	pcb->IndiceStack = 0;
	pcb->PaginasDeCodigo=0;
	pcb->ProgramCounter = 0;
	ultimoPID++;
	list_add(Nuevos,pcb);
	//list_add(ListaPCB,pcb);
}

void obtenerError(int exitCode){
	switch(exitCode){
		case 0:
			printf(" (El programa finalizó correctamente)\n");
		break;

		case -1:
			printf(" (No se pudieron reservar recursos para ejecutar el programa)\n");
		break;

		case -2:
			printf(" (El programa intentó acceder a un archivo que no existe)\n");
		break;

		case -3:
			printf(" (El programa intentó leer un archivo sin permisos)\n");
		break;

		case -4:
			printf(" (El programa intentó escribir un archivo sin permisos)\n");
		break;

		case -5:
			printf(" (Excepción de memoria)\n");
		break;

		case -6:
			printf(" (Finalizado a través de desconexión de consola)\n");
		break;

		case -7:
			printf(" (Finalizado a través del comando Finalizar Programa de la consola)\n");
		break;

		case -8:
			printf(" (Se intentó reservar más memoria que el tamaño de una página)\n");
		break;

		case -9:
			printf(" (No se pueden asignar más páginas al proceso)\n");
		break;

		case -20:
			printf(" (Error sin definición)\n");
		break;
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
						SHARED_VARS[i++] = texto;
						texto = strtok (NULL, ",");
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
void imprimirArchivoConfiguracion()
{
	int c;
	FILE *file;

	file = fopen("ArchivoConfiguracion.txt", "r");

	if (file)
	{
		while ((c = getc(file)) != EOF)
		{
			putchar(c);
		}

		fclose(file);
	}
}
void MostrarProcesosDeUnaLista(t_list* lista,char* discriminator){
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
void ConsultarEstado(int pidAConsultar){
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
		printf("Tamaño del stack: %d \n",proceso->IndiceStack);
		printf("Paginas de codigo: %d \n",proceso->PaginasDeCodigo);
		printf("Contador de programa: %d \n",proceso->ProgramCounter);
	}
}
void syscallWrite(int socketFD) {
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
void MostrarTodosLosProcesos(){
	MostrarProcesosDeUnaLista(Nuevos,NUEVOS);
	MostrarProcesosDeUnaLista(Listos,LISTOS);
	MostrarProcesosDeUnaLista(Ejecutando,EJECUTANDO);
	MostrarProcesosDeUnaLista(Bloqueados,BLOQUEADOS);
	MostrarProcesosDeUnaLista(Finalizados,FINALIZADOS);
}

void KillProgramDeUnaLista(t_list* lista,BloqueControlProceso* pcb,int tipoFinalizacion, int index, int socket) {
	FinalizarPrograma(lista,pcb->PID,tipoFinalizacion, index, socket);
}
bool KillProgram(int pidAFinalizar,int tipoFinalizacion, int socket){
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

void accion(Paquete* paquete, int socketConectado){
	/*pthread_t hiloSyscallWrite;
	pthread_create(&hiloSyscallWrite,NULL, (void*)syscallWrite, &socketConectado);*/
	switch(paquete->header.tipoMensaje) {

		case ESSTRING:

				if(strcmp(paquete->header.emisor,CONSOLA)==0)
				{
					double tamanioArchivo = paquete->header.tamPayload/TamanioPagina;
					double tamanioTotalPaginas = ceil(tamanioArchivo)+STACK_SIZE;
					BloqueControlProceso* pcb = malloc(sizeof(BloqueControlProceso));
					//TODO: Falta free, pero OJO, hay que ver la forma de ponerlo y que siga andando todo
					CrearNuevoProceso(pcb);

					//Manejo la multiprogramacion
					if(GRADO_MULTIPROG - list_size(Ejecutando) - list_size(Listos) > 0 && list_size(Nuevos) >= 1){
						//Pregunta a la memoria si me puede guardar estas paginas
						uint32_t paginasConfirmadas = IM_InicializarPrograma(socketConMemoria,KERNEL,pcb->PID,tamanioTotalPaginas);
						if(paginasConfirmadas>0 ) // N° negativo significa que la memoria no tiene espacio
						{
							pcb->PaginasDeCodigo = (uint32_t)tamanioTotalPaginas;
							printf("Cant paginas asignadas: %d \n",pcb->PaginasDeCodigo);

							//Saco el programa de la lista de NEW y  agrego el programa a la lista de READY
							list_remove_by_condition(Nuevos, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pcb->PID; }));
							list_add(Listos, pcb);
							printf("El programa %d se cargo en memoria \n",pcb->PID);

							//Solicito a la memoria que me guarde el codigo del programa
							IM_GuardarDatos(socketConMemoria, KERNEL, pcb->PID, 0, 0, paquete->header.tamPayload, paquete->Payload); //TODO: sacar harcodeo
							EnviarDatos(socketConectado,KERNEL,&(pcb->PID),sizeof(uint32_t));
							//TODO: Ejecutar en alguna CPU

							//TODO: Realizar acciones en caso de solicitud de reserva de memoria dinamica
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

		break;
		case KILLPROGRAM:
			if(strcmp(paquete->header.emisor,CONSOLA)==0){
				pidAFinalizar = *(uint32_t*)paquete->Payload;
				bool finalizadoConExito = KillProgram(pidAFinalizar, DESCONECTADODESDECOMANDOCONSOLA, socketConectado);
				if(finalizadoConExito==true){
					printf("El programa %d fue finalizado\n", pidAFinalizar);
					EnviarMensaje(socketConectado,"KILLEADO",KERNEL);
				}
				else {
					printf("Error al finalizar programa\n", pidAFinalizar);
					EnviarMensaje(socketConectado,"Error al finalizar programa",KERNEL);
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
			KillProgram(pidConsulta, DESCONECTADODESDECOMANDOCONSOLA, *socketFD);
		}
		else {
			printf("No se conoce el mensaje %s\n", orden);
		}
	}
}



int main(void)
{
	CrearListasEstados();
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	while((socketConMemoria = ConectarAServidor(PUERTO_MEMORIA,IP_MEMORIA,MEMORIA,KERNEL, RecibirHandshake_KernelDeMemoria))<0);

	pthread_t hiloConsola;
	//pthread_t hiloSyscallWrite;
	//pthread_create(&hiloSyscallWrite, NULL, (void*)syscallWrite, 2); //socket 2 hardcodeado
	//pthread_join(hiloSyscallWrite, NULL);
	pthread_create(&hiloConsola, NULL, (void*)userInterfaceHandler, socketConMemoria);
	Servidor(IP_PROG, PUERTO_PROG, KERNEL, accion, RecibirPaqueteServidor);
	pthread_join(hiloConsola, NULL);
	LimpiarListas();

	return 0;
}
