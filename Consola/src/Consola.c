#include "SocketsL.h"
#include "Helper.h"

#define TAMANIOMAXIMOFIJO 20
#define true 1
#define false 0
#ifndef NULL
#define NULL   ((void *) 0)
#endif

char* IP_KERNEL;
int PUERTO_KERNEL;
bool fin = false;

typedef struct {
	int socket;
	uint32_t pid;
	pthread_t hilo;
	char* pathCodigo;
} structProceso;
t_list * listaProcesos;
pthread_mutex_t mutexListaProcesos = PTHREAD_MUTEX_INITIALIZER;

bool existeArchivo(char* path) {
	return access( path , F_OK ) != -1;
}

void obtenerValoresArchivoConfiguracion() {
	t_config* arch = config_create("ArchivoConfiguracion.txt");
	IP_KERNEL = string_duplicate(config_get_string_value(arch, "IP_KERNEL"));
	PUERTO_KERNEL = config_get_int_value(arch, "PUERTO_KERNEL");
	config_destroy(arch);
}

void sendSignalOpenFile(char* programPath, int socketFD) {
	FILE* fileForSend = fopen(programPath, "r");
	char * buffer = 0;
	long length;

	if (fileForSend) {
		fseek(fileForSend, 0, SEEK_END);
		length = ftell(fileForSend);
		fseek(fileForSend, 0, SEEK_SET);
		buffer = malloc(length);
		if (buffer) {
			fread(buffer, 1, length, fileForSend);
		}
		fclose(fileForSend);
		EnviarMensaje(socketFD, buffer, CONSOLA);
	}
	else
		printf("No se encontro el archivo");
}
void programHandler(void* structProc) {
	structProceso* procesoActual = structProc;
	bool finPrograma = false;
	time_t tiempoDeInicio = time(NULL);
	char* sTiempoDeInicio = temporal_get_string_time();
	int socketFD = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CONSOLA, RecibirHandshake);
	procesoActual->socket = socketFD;
	uint32_t pid;
	sendSignalOpenFile(procesoActual->pathCodigo, socketFD);
	free(procesoActual->pathCodigo);
	Paquete paquete;
	while(!fin && !finPrograma) {
		while(RecibirPaqueteCliente(socketFD, CONSOLA, &paquete)<0);
		switch (paquete.header.tipoMensaje) {
			case ESDATOS:
				if(strcmp(paquete.header.emisor,KERNEL)==0){
					pid = *((uint32_t*) paquete.Payload);
					if(pid >= 1){
						procesoActual->pid = pid;
						printf("El pid del nuevo programa es %d \n",pid);
					}
				}
			break;
			case ESSTRING:
				printf("%s\n",(char*)paquete.Payload);
				if(strcmp(paquete.header.emisor,KERNEL)==0){

					if (strcmp((char*)paquete.Payload, "KILLEADO") == 0) {
						time_t tiempoFinalizacion = time(NULL);
						char* sTiempoDeFin = temporal_get_string_time();
						printf("%s\n", sTiempoDeInicio);
						printf("%s\n", sTiempoDeFin);
						free(sTiempoDeFin);
						free(sTiempoDeInicio);
						double diferencia = difftime(tiempoFinalizacion, tiempoDeInicio);
						printf("La duracion del programa en segundos es de %f\n", diferencia);
						pthread_mutex_lock( &mutexListaProcesos );
						list_remove_and_destroy_by_condition(listaProcesos,
							LAMBDA(bool _(void* elem) {
								//printf("pid: %u\n", ((structProceso*)elem)->pid);
								return ((structProceso*)elem)->socket == socketFD;
							}) , free);
						pthread_mutex_unlock( &mutexListaProcesos );

						finPrograma = true;
					}
					else if (strcmp(string_substring_until(paquete.Payload,8),"imprimir") == 0) {
						printf("%s\n", string_substring_from((char*)paquete.Payload,8));
					}
					else {
						  printf("%s\n", (char*)paquete.Payload);
					}
				}
			break;
		}
		free(paquete.Payload);
	}
	printf("programa finalizado\n");

}
void programDestroyer(void* p){
	structProceso* sp = p;
	//pthread_join(sp->hilo, NULL);

	free(sp);
}
int startProgram(char* programPath) {
	if (existeArchivo(programPath)) {
		structProceso* stProg =  malloc(sizeof(structProceso));

		stProg->pathCodigo = string_duplicate(programPath);
		//pthread_t* program = malloc(sizeof(pthread_t));
		pthread_mutex_lock( &mutexListaProcesos );
		list_add(listaProcesos, stProg);
		pthread_mutex_unlock( &mutexListaProcesos );
		pthread_create(&(stProg->hilo), NULL, (void*)programHandler, stProg);
	}
	else {
		printf("No existe el archivo\n");
	}
	return 0;
}

void endProgram(structProceso* sp) {
	//printf("socket: %d\n",sp->socket);
	//printf("pid: %d\n",sp->pid);
	EnviarDatosTipo(sp->socket, CONSOLA, &sp->pid, sizeof(uint32_t), KILLPROGRAM);
/*
	Paquete nuevoPaquete;
	while(RecibirPaqueteCliente(sp->socket, CONSOLA, &nuevoPaquete)<0);

	if(nuevoPaquete.header.tipoMensaje==ESSTRING){
		if(strcmp(nuevoPaquete.header.emisor,KERNEL)==0){
			char* result = nuevoPaquete.Payload;
			printf("result: %s",result);
			if (strcmp(result, "KILLEADO") == 0) {
				printf("Se finalizo el programa N°: %d\n",sp->pid);
			}
		}
	}
	//free(sp);
	free(nuevoPaquete.Payload);
	*/
}

void clean() {
	system("clear");
}

void killAllPrograms() {
	int i;
	pthread_mutex_lock( &mutexListaProcesos );
	for (i = 0; i < list_size(listaProcesos); i++) {
		structProceso* proceso = (structProceso*) list_get(listaProcesos, i);
		endProgram(proceso);
		free(proceso);
	}
	pthread_mutex_unlock( &mutexListaProcesos );
}

void liberarMemoria() {
	pthread_mutex_lock( &mutexListaProcesos );
	list_destroy_and_destroy_elements(listaProcesos, free);
	pthread_mutex_unlock( &mutexListaProcesos );
	free(IP_KERNEL);
	pthread_mutex_destroy( &mutexListaProcesos );
}

void killConsole() {
	fin = true;
	killAllPrograms();
	liberarMemoria();
}

void userInterfaceHandler(uint32_t* socketGeneral) {
	char command[100];

	while (!fin) {

		printf("\nIngrese una orden: \n");
		scanf("%s", command);
		if (strcmp(command, "start_program") == 0) {
			char parametro[500]; //= string_new();
			scanf("%s", parametro);
			startProgram(parametro);
			//free(parametro);
		} else if (strcmp(command, "kill_program") == 0) {
			command[0] = '\0'; //lo vacio
			scanf("%s", command);
			long pid = strtol(command, NULL, 10);
			if (pid != 0 && pid <= (2^32)) {
				//que sea un numero y que no se pase del max del uint32
				pthread_mutex_lock( &mutexListaProcesos );
				structProceso* sp = list_find(listaProcesos,
					LAMBDA(bool _(void* item) {
						return ((structProceso*)item)->pid == pid;
					}));
				pthread_mutex_unlock( &mutexListaProcesos );
				if (sp != NULL) {
					printf("Finalizando programa: %d\n",sp->pid);
					endProgram(sp);
				}
				else {
					printf("No se reconoce algun programa con ese PID");
				}

			} else {
				printf("No se conoce el comando %s\n", command);
			}
		} else if (strcmp(command, "disconnect") == 0) {
			killConsole();
		} else if (strcmp(command, "clean") == 0) {
			clean();
		} else {
			printf("No se conoce el comando: %s\n", command);
		}

	}
	printf("consola finalizada \n");
}

void sigintHandler(int sig_num)
{
	killConsole();
	signal(SIGINT, sigintHandler);
	exit(0);
}

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	int socketFD = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CONSOLA, RecibirHandshake);
	listaProcesos = list_create();
	signal(SIGINT, sigintHandler);
	pthread_t userInterface;
	pthread_create(&userInterface, NULL, (void*)userInterfaceHandler,&socketFD);
	pthread_join(userInterface, NULL);

	list_destroy_and_destroy_elements(listaProcesos, LAMBDA(void _(void* elem) {
				pthread_join(((structProceso*)elem)->hilo, NULL);
				free(elem); }));

	return 0;
}
