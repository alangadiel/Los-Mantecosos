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
} structProceso;
t_list * listaProcesos;

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
void programHandler(void *programPath) {
	bool fin = false;
	time_t tiempoDeInicio = time(NULL);
	char* sTiempoDeInicio = temporal_get_string_time();
	int socketFD = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CONSOLA, RecibirHandshake);
	uint32_t pid;
	sendSignalOpenFile((char*) programPath, socketFD);
	Paquete paquete;
	while(!fin) {
		while(RecibirPaqueteCliente(socketFD, CONSOLA, &paquete)<0);
		switch (paquete.header.tipoMensaje) {
			case ESDATOS:
				if(strcmp(paquete.header.emisor,KERNEL)==0){
					pid = *((uint32_t*) paquete.Payload);
					if(pid >= 1){
						structProceso* sp = malloc(sizeof(structProceso));

						sp->pid = pid;
						sp->socket = socketFD;
						list_add(listaProcesos,sp);

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
						list_remove_and_destroy_by_condition(listaProcesos,
								LAMBDA(bool _(void* elem) { return ((structProceso*)elem)->socket == socketFD; }), free);
						fin = true;
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
	}
	free(paquete.Payload);


	/*int end = 1;
	int cantMensajes = 0;
	while (end) {
		uint32_t datosRecibidos = RecibirPaqueteCliente(socketFD, CONSOLA,
				&paquete);
		while (datosRecibidos <= 0) {
			datosRecibidos = RecibirPaqueteCliente(socketFD, CONSOLA, &paquete);
		}

		cantMensajes++;
		if (strcmp(paquete.Payload, "kill") == 0) {
			end = 0;
			time_t tiempoFinalizacion = time(NULL);
			printf("%s\n", obtenerTiempoString(tiempoDeInicio));
			printf("%s\n", obtenerTiempoString(tiempoFinalizacion));
			printf("%d\n", cantMensajes);
			double diferencia = difftime(tiempoFinalizacion, tiempoDeInicio);
			printf("La duracion del programa en segundos es de %f\n",
					diferencia);
		} else if (strcmp(paquete.Payload, "imprimir") == 0) {
			printf("%s\n", (char*)paquete.Payload);
		}
	}*/
}

int startProgram(char* programPath) {
	if (existeArchivo(programPath)) {
		pthread_t* program = malloc(sizeof(pthread_t));
		list_add(listaProcesos, program);
		pthread_create(program, NULL, (void*)programHandler, programPath);
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

	Paquete nuevoPaquete;
	while(RecibirPaqueteCliente(sp->socket, CONSOLA, &nuevoPaquete)<0);
	free(sp);

	if(nuevoPaquete.header.tipoMensaje==ESSTRING){
		if(strcmp(nuevoPaquete.header.emisor,KERNEL)==0){
			char* result = (char*)nuevoPaquete.Payload;
			printf("result: %s",result);
			if (strcmp(result, "KILLEADO") == 0) {
				printf("Se finalizo el programa N°: %d\n",sp->pid);
			}
		}
	}
	free(nuevoPaquete.Payload);
}

void clean() {
	system("clear");
}

void killAllPrograms() {
	int i;
	for (i = 0; i < list_size(listaProcesos); i++) {
		structProceso* proceso = (structProceso*) list_get(listaProcesos, i);
		endProgram(proceso);
		free(proceso);
	}
}

void liberarMemoria() {
	list_destroy_and_destroy_elements(listaProcesos, free);
	free(IP_KERNEL);
}

void killConsole() {
	fin = true;
	killAllPrograms();
	liberarMemoria();
}

void userInterfaceHandler(uint32_t* socketGeneral) {

	while (!fin) {
		char command[100];
		char parametro[100];
		printf("\nIngrese una orden: \n");
		scanf("%s", command);
		if (strcmp(command, "start_program") == 0) {
			scanf("%s", parametro);
			startProgram(parametro);
		} else if (strcmp(command, "end_program") == 0) {


			scanf("%s", parametro);
			uint32_t pid = atoi(parametro);
			structProceso* sp = (structProceso*)list_find(listaProcesos,LAMBDA(bool _(void* item) { return ((structProceso*)item)->pid == pid; }));
			if (sp != NULL) {
				printf("pid a eliminar : %d\n",sp->pid);
				endProgram(sp);
			}
			else {
				printf("No se reconoce algun programa con ese PID");
			}
		} else if (strcmp(command, "disconnect") == 0) {
			killConsole();
		} else if (strcmp(command, "clean") == 0) {
			clean();
		} else {
			printf("No se conoce el comando: %s\n", command);
		}
	}
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
