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
	int contadorDeVariables = 0;
	int c;
	FILE *file;

	file = fopen("ArchivoConfiguracion.txt", "r");

	if (file) {
		while ((c = getc(file)) != EOF) {
			if (c == '=') {
				if (contadorDeVariables == 1) {
					fscanf(file, "%i", &PUERTO_KERNEL);
				}

				if (contadorDeVariables == 0) {
					char buffer[10000];

					IP_KERNEL = fgets(buffer, sizeof buffer, file);
					strtok(IP_KERNEL, "\n");

					contadorDeVariables++;
				}
			}
		}

		fclose(file);
	}
}

void imprimirArchivoConfiguracion() {
	int c;
	FILE *file;

	file = fopen("ArchivoConfiguracion.txt", "r");

	if (file) {
		while ((c = getc(file)) != EOF) {
			putchar(c);
		}
		fclose(file);
	}
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
	}
	EnviarMensaje(socketFD, buffer, CONSOLA);
}
void programHandler(void *programPath) {
	time_t tiempoDeInicio = time(NULL);
	int socketFD = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CONSOLA, RecibirHandshake);
	uint32_t pid;
	sendSignalOpenFile((char*) programPath, socketFD);
	Paquete paquete;
	uint32_t datosRecibidos = RecibirPaqueteCliente(socketFD, CONSOLA, &paquete);
	/*while (datosRecibidos < 0) {
		datosRecibidos = RecibirPaqueteCliente(socketFD, CONSOLA, &paquete);
	}*/
	switch (paquete.header.tipoMensaje) {
		case ESDATOS:
			if(strcmp(paquete.header.emisor,KERNEL)==0){
				pid = *((uint32_t*) paquete.Payload);
				SocketProceso sp;
				sp.pid = pid;
				sp.socket = socketFD;
				list_add(SocketsProcesos,&sp);
				printf("El pid del nuevo programa es %d \n",pid);
			}
		break;
		case ESSTRING:
			if(strcmp(paquete.header.emisor,KERNEL)==0){
				//printf("%s\n",(char*)paquete.Payload);
				if (strcmp(paquete.Payload, "kill") == 0) {
					time_t tiempoFinalizacion = time(NULL);
					printf("%s\n", obtenerTiempoString(tiempoDeInicio));
					printf("%s\n", obtenerTiempoString(tiempoFinalizacion));
					double diferencia = difftime(tiempoFinalizacion, tiempoDeInicio);
					printf("La duracion del programa en segundos es de %f\n",
							diferencia);
				} else if (strcmp(paquete.Payload, "imprimir") == 0) {
					printf("%s\n", (char*)paquete.Payload);
				}
			}
		break;

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

void endProgram(int pid) {
	Paquete paquete;
	strcpy(paquete.header.emisor, CONSOLA);
	paquete.header.tipoMensaje = KILLPROGRAM;
	paquete.header.tamPayload = sizeof(int);
	paquete.Payload = &pid;
	printf("uri");
	void* sp = list_find(SocketsProcesos,LAMBDA(bool _(void* item) { return ((SocketProceso*)item)->pid != pid; }));
	if(sp!=NULL){
		printf("Pid = %d y socket = %d ",((SocketProceso*)sp)->pid,((SocketProceso*)sp)->socket);
		EnviarPaquete(((SocketProceso*)sp)->socket, &paquete);
	}
	else
		printf("No se reconoce algun programa con ese PID");
}

void clean() {
	system("clear"); // Clear screen and home cursor
}



void userInterfaceHandler() {
	int end = 1;
	while (end) {
		char command[100];
		char parametro[100];
		printf("\n\nIngrese SOLO el comando: \n");
		scanf("%s", command);
		/*char* command = getWord(str, 0);
		char* parameter = getWord(str, 1);*/
		if (strcmp(command, "start_program") == 0) {
			printf("\n\nIngrese nombre de archivo: \n");
			scanf("%s", parametro);
			char* pmtr = getWord(parametro,0);
			startProgram(pmtr);
		}
		if (strcmp(command, "end_program") == 0) {
			printf("\n\nIngrese numero de programa: \n");
			scanf("%s", parametro);
			char* pmtr = getWord(parametro,0);
			int pid = atoi(pmtr);
			endProgram(pid);
		} else if (strcmp(command, "disconnect") == 0) {
			end = 0;
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
	pthread_create(&userInterface, NULL, (void*)userInterfaceHandler,NULL);
	pthread_join(userInterface, NULL);
	//userInterfaceHandler(&socketFD);
	return 0;
}
