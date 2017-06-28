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

typedef struct {
	uint32_t socket;
	uint32_t pid;
} SocketProceso;
t_list * SocketsProcesos;

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
						SocketProceso* sp = malloc(sizeof(SocketProceso));     //TODO: Falta free
						sp->pid = pid;
						sp->socket = socketFD;
						list_add(SocketsProcesos,sp);
						SocketProceso* result = (SocketProceso*)list_get(SocketsProcesos,0);
						printf("socket: %d\n",result->socket);
						printf("pid: %d\n",result->pid);
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
						printf("La duracion del programa en segundos es de %f\n",
								diferencia);
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
	pthread_t program;
	pthread_create(&program, NULL, (void*)programHandler, programPath);
	pthread_join(program, NULL);
	return 0;
}

void endProgram(uint32_t pid,uint32_t* socketgeneral) {
	Paquete paquete;
	strcpy(paquete.header.emisor, CONSOLA);
	paquete.header.tipoMensaje = KILLPROGRAM;
	paquete.header.tamPayload = sizeof(uint32_t);
	paquete.Payload = &pid;
	printf("pid a eliminar : %d\n",pid);
	SocketProceso* sp = (SocketProceso*)list_find(SocketsProcesos,LAMBDA(bool _(void* item) { return ((SocketProceso*)item)->pid == pid; }));
	if(sp!=NULL){
		printf("socket: %d\n",sp->socket);
		printf("pid: %d\n",sp->pid);
		EnviarPaquete(sp->socket, &paquete);

		Paquete nuevoPaquete;
		while(RecibirPaqueteCliente(sp->socket, CONSOLA, &nuevoPaquete)<0);
		free(sp);

		if(nuevoPaquete.header.tipoMensaje==ESSTRING){
			if(strcmp(nuevoPaquete.header.emisor,KERNEL)==0){
			char* result = (char*)nuevoPaquete.Payload;
			printf("result: %s",result);

			if (strcmp(result, "KILLEADO") == 0) {
				printf("Se finalizo el programa N°: %d\n",pid);
			}
		}
	 }
	free(nuevoPaquete.Payload);
	}
	else
		printf("No se reconoce algun programa con ese PID");
}

void clean() {
	system("clear"); // Clear screen and home cursor
}



void userInterfaceHandler(uint32_t* socketGeneral) {
	bool fin = false;
	while (!fin) {
		char command[100];
		char parametro[100];
		printf("\n\nIngrese SOLO el comando: \n");
		scanf("%99s", command);
		/*char* command = getWord(str, 0);
		char* parameter = getWord(str, 1);*/
		if (strcmp(command, "start_program") == 0) {
			printf("\n\nIngrese nombre de archivo: \n");
			scanf("%99s", parametro);
			startProgram(parametro);
		}
		if (strcmp(command, "end_program") == 0) {
			printf("\n\nIngrese numero de programa: \n");
			scanf("%99s", parametro);
			uint32_t pid = atoi(parametro);
			endProgram(pid,socketGeneral);
		} else if (strcmp(command, "disconnect") == 0) {
			fin = true;
		} else if (strcmp(command, "clean") == 0) {
			clean();
		} else {
			printf("No se conoce el comando: %s\n", command);
		}
	}
}

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	int socketFD = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CONSOLA, RecibirHandshake);
	SocketsProcesos = list_create();

	pthread_t userInterface;
	pthread_create(&userInterface, NULL, (void*)userInterfaceHandler,&socketFD);
	pthread_join(userInterface, NULL);
	free(IP_KERNEL);

	return 0;
}
