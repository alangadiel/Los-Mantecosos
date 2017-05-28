#include "SocketsL.h"
#include "Helper.h"
#define NUEVOS "NUEVOS"
#define LISTOS "LISTOS"
#define EJECUTANDO "EJECUTANDO"
#define BLOQUEADOS "BLOQUEADOS"
#define FINALIZADOS "FINALIZADOS"


#define BACKLOG 6 //Backlog es el maximo de peticiones pendientes
#define DESCONECTADODESDECOMANDOCONSOLA -7
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
 int isFree;
} HeapMetadata;

typedef struct {
	FILE * file;
	BloqueControlProceso pcb;
} PeticionTamanioStack;


uint32_t PidAComparar;
t_list* Nuevos;
t_list* Finalizados;
t_list* Bloqueados;
t_list* Ejecutando;
t_list* Listos;
t_list* Estados;
t_list* EstadosConProgramasFinalizables;

/*
//replicar aca!!
int j;
for (j = 0; j <= fdmax; j++) {
		// ¡enviar a todo el mundo!
		if (FD_ISSET(j, &master)) {
			// excepto al listener y a nosotros mismos
			if (j != SocketEscucha && j != i) {
				EnviarMensaje(j,(char*)paquete->Payload,KERNEL, ESSTRING);
				//if (send(j, paquete, result, 0) == -1) {
					//perror("send");
				//}
			}
		}
	}
*/
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
	list_add(Estados,Nuevos);
	list_add(EstadosConProgramasFinalizables,Listos);
	list_add(EstadosConProgramasFinalizables,Ejecutando);
	list_add_all(Estados,EstadosConProgramasFinalizables);
	list_add(Estados,Bloqueados);
	list_add(Estados,Finalizados);

}
BloqueControlProceso CrearNuevoProceso(){
	//Creo el pcb y lo guardo en la lista de nuevos
	BloqueControlProceso pcb;
	pcb.PID = ultimoPID+1;
	pcb.ProgramCounter = 0;
	ultimoPID++;
	list_add(Nuevos,&pcb);
	return pcb;
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
				/*if (contadorDeVariables == 15)
				{
					fscanf(file, "%i", &TAM_PAG);
					contadorDeVariables++;

				}*/
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
		printf("Proceso N°: %d \n",proceso->PID);
	}
}
void ConsultarEstado(int pidAConsultar){
	int i =0;
	void* result;
	//Busco el proceso en todas las listas
	while(i<list_size(Estados) && result==NULL){
		t_list* lista = list_get(Estados,i);
		result = list_find(lista,LAMBDA(bool _(void* pcb) { return ((BloqueControlProceso*)pcb)->PID != pidAConsultar; }));
		i++;
	}
	if(result==NULL)
		printf("No se encontro el proceso a consultar. Intente nuevamente");
	else{
		BloqueControlProceso* proceso = (BloqueControlProceso*)proceso;
		printf("Proceso N°: %d \n",proceso->PID);
		//printf("Indice de codigo: %d \n",proceso->IndiceDeCodigo);
		printf("Tamaño del stack: %d \n",proceso->TamanioStack);
		printf("Paginas de codigo: %d \n",proceso->PaginasDeCodigo);
		printf("Contador de programa: %d \n",proceso->ProgramCounter);
	}
}
void MostrarTodosLosProcesos(){
	MostrarProcesosDeUnaLista(Nuevos,NUEVOS);
	MostrarProcesosDeUnaLista(Listos,LISTOS);
	MostrarProcesosDeUnaLista(Ejecutando,EJECUTANDO);
	MostrarProcesosDeUnaLista(Bloqueados,BLOQUEADOS);
	MostrarProcesosDeUnaLista(Finalizados,FINALIZADOS);
}

bool KillProgram(int pidAFinalizar,int tipoFinalizacion){
	int i =0;
	void* result;
	//Busco el proceso en todas las listas
	while(i<list_size(EstadosConProgramasFinalizables) && result==NULL){
		t_list* lista = list_get(EstadosConProgramasFinalizables,i);
		result = list_find(lista,LAMBDA(bool _(void* pcb) { return ((BloqueControlProceso*)pcb)->PID != pidAFinalizar; }));
	}
	if(result==NULL){
		printf("No se encuentro el programa finalizar");
		return false;
	}
	else
	{
		list_remove_by_condition(Listos, LAMBDA(bool _(void* pcb) { return ((BloqueControlProceso*)pcb)->PID != pidAFinalizar; }));
		BloqueControlProceso* pcb = (BloqueControlProceso*)result;
		//Le asigno el codigo de finalización de programa y lo pongo en la lista de Exit
		pcb->ExitCode = tipoFinalizacion;
		list_add(Finalizados,pcb);
		// TODO: Liberar las paginas de memoria asignadas a ese proceso
		return true;
	}
}

void accion(Paquete* paquete, int socketConectado){
	switch(paquete->header.tipoMensaje) {

		case ESSTRING:

				if(strcmp(paquete->header.emisor,CONSOLA)==0)
				{
					double tamanioArchivo = paquete->header.tamPayload/TamanioPagina;
					double tamanioTotalPaginas = ceil(tamanioArchivo)+STACK_SIZE;

					BloqueControlProceso pcb = CrearNuevoProceso();
					//Manejo la multiprogramacion
					if(GRADO_MULTIPROG - list_size(Ejecutando) - list_size(Listos) > 0 && list_size(Nuevos) >= 1){
						//Pregunta a la memoria si me puede guardar estas paginas
						uint32_t paginasConfirmadas = IM_InicializarPrograma(socketConMemoria,KERNEL,pcb.PID,tamanioTotalPaginas);
						if(paginasConfirmadas == tamanioTotalPaginas) // N° negativo significa que la memoria no tiene espacio
						{
							printf("Cant paginas asignadas: %d \n",paginasConfirmadas);

							pcb.PaginasDeCodigo = tamanioTotalPaginas;
							//Saco el programa de la lista de NEW y lo agrego el programa a la lista de READY
							PidAComparar = pcb.PID;

							list_remove_by_condition(Nuevos, LAMBDA(bool _(void* pcb) { return ((BloqueControlProceso*)pcb)->PID != PidAComparar; }));
							printf("Tamanio de la lista de nuevos programas: %d \n",list_size(Nuevos));
							list_add(Listos,&pcb);
							printf("El programa %d se cargo en memoria \n",pcb.PID);

							//Solicito a la memoria que me guarde el codigo del programa
							IM_GuardarDatos(socketConMemoria, KERNEL, pcb.PID, 0, 0, paquete->header.tamPayload, paquete->Payload); //TODO: sacar harcodeo
							EnviarDatos(socketConectado,KERNEL,&pcb.PID,sizeof(uint32_t));
						}
						else
						{
							EnviarMensaje(socketConectado,"No se pudo guardar el programa porque no hay espacio suficiente",KERNEL);
						}

					}

				}

		break;
		case KILLPROGRAM:
			if(strcmp(paquete->header.emisor,CONSOLA)){
				pidAFinalizar = *(uint32_t*)paquete->Payload;
				bool finalizadoConExito = KillProgram(pidAFinalizar,DESCONECTADODESDECOMANDOCONSOLA);
				if(finalizadoConExito==true){
					EnviarMensaje(socketConectado,"kill",KERNEL);
				}
				else
					EnviarMensaje(socketConectado,"Error al finalizar programa",KERNEL);


			}
		break;
	}
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
void userInterfaceHandler(void* socketFD) {
	int end = 1;
	while (end) {
		char orden[100];
		int pidConsulta=0;
		char lista[100];
		printf("\n\nIngrese una orden: \n");
		scanf("%s", orden);
		char* command = getWord(orden, 0);
		if(strcmp(command,"exit")==0)
				exit(1);
		 else if (strcmp(command, "disconnect") == 0) {
				end = 0;
			}
		 else if (strcmp(command, "MOSTRARPROCESOS") == 0) {
			printf("\n\nIngrese lista en la que buscar: \n");
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

		else if (strcmp(command, "ConsultarPrograma") == 0) {
			printf("\n\nIngrese numero de programa: \n");
			scanf("%d", &pidAFinalizar);
			ConsultarEstado(pidConsulta);
		}  else {
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
	pthread_create(&hiloConsola, NULL, (void*)userInterfaceHandler, NULL);
	Servidor(IP_PROG, PUERTO_PROG, KERNEL, accion, RecibirPaqueteServidor);
	pthread_join(hiloConsola, NULL);

	return 0;
}
