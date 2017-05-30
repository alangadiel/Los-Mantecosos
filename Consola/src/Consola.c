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
t_list* listaHilos;

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

void* programHandler(void *programPath) {
	printf("Starting Program");
	time_t tiempoDeInicio = time(NULL);
	int socketFD = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CONSOLA, RecibirHandshake);
	int pid;
	sendSignalOpenFile((char*) programPath, socketFD);
	Paquete paquete;
	uint32_t datosRecibidos = RecibirPaqueteCliente(socketFD, CONSOLA, &paquete);
	while (datosRecibidos <= 0) {
		datosRecibidos = RecibirPaqueteCliente(socketFD, CONSOLA, &paquete);
	}
	pid = *((uint32_t*) paquete.Payload);
	free(paquete.Payload);
	int end = 1;
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
	}
	return NULL;
}

int startProgram(char* programPath) {
	printf("Start Program");
	pthread_t* program= malloc(sizeof(pthread_t));
	int r = pthread_create(program, NULL, programHandler, (void*)programPath);
	list_add(listaHilos, program);
	return r;
}

void endProgram(int pid, int* socketFD) {
	Paquete paquete;
	strcpy(paquete.header.emisor, CONSOLA);
	paquete.header.tipoMensaje = KILLPROGRAM;
	paquete.header.tamPayload = sizeof(int);
	paquete.Payload = &pid;
	EnviarPaquete(*socketFD, &paquete);
}

void clean() {
	system("clear"); // Clear screen and home cursor
}

void userInterfaceHandler(void* socketFD) {
	int end = 1;
	while (end) {
		char command[100];
		char parametro[100];
		printf("\nIngrese SOLO el comando: \n");
		scanf("%s", command);
		/*char* command = getWord(str, 0);
		char* parameter = getWord(str, 1);*/
		if (!strcmp(command, "start_program")) {
			printf("\nIngrese parametro: \n");
			scanf("%s", parametro);
			startProgram(parametro);
		} else if (!strcmp(command, "end_program")) {
			printf("\nIngrese parametro: \n");
			scanf("%s", parametro);
			int pid = atoi(parametro);
			endProgram(pid, (int*) socketFD);
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
	/*pthread_t userInterface;
	pthread_create(&userInterface, NULL, (void*)userInterfaceHandler, &socketFD);
	pthread_join(userInterface, NULL);*/ //creo que no se necesita un hilo si ya tenes el principal
	userInterfaceHandler(&socketFD);
	printf("fin");
	list_destroy_and_destroy_elements(listaHilos, LAMBDA(void _(void* elem) { pthread_join(*(pthread_t*)elem, NULL); }));
	return 0;
}
