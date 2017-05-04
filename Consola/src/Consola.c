#include "SocketsL.h"


#define TAMANIOMAXIMOFIJO 20
#define true 1
#define false 0

char* IP_KERNEL;
int PUERTO_KERNEL;
char* IP_MEMORIA;
int PUERTO_MEMORIA;

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

char* integer_to_string(int x) {
	char* buffer = malloc(sizeof(char) * sizeof(int) * 4 + 1);
	if (buffer) {
		sprintf(buffer, "%d", x);
	}
	return buffer; // caller is expected to invoke free() on this buffer to release memory
}

void programHandler(int socketFD) {
	while (true) {
		char str[100];
		printf("\n\nIngrese un mensaje: \n");
		scanf("%99[^\n]", str);

		EnviarMensaje(socketFD, str, CONSOLA);
	}
}

int startProgram(char* programPath, int socketFD) {
	pthread_t program;
	int arg = socketFD;
	int r = pthread_create(&program, NULL, programHandler, (void*) arg);
	pthread_join(program, NULL);
	return 0;
}

int endProgram(char* programPath, int socketFD) {

	/*Mostrar esto cuando termina el proceso:
	  ● Fecha y hora de inicio de ejecución
	  ● Fecha y hora de fin de ejecución
	  ● Cantidad de impresiones por pantalla
	  ● Tiempo total de ejecución (diferencia entre tiempo de inicio y tiempo de fin)*/
	return 0;
}

void disconnect() {

}

void clean() {
	printf( "\e[2J\e[H" ); // Clear screen and home cursor
}

void sendSignalOpenFile(char* programPath, int socketFD) {
	FILE* fileForSend = txt_open_for_append(programPath);
	write()
	EnviarMensaje(socketFD, programPath, CONSOLA);
}

char* getFirstWord(char* string) {
	return string_split(string, ' ')[0];
}

char* getRestOfString(char* string) {
	char* firstWord = string_split(string, ' ')[0];
	char* lengthOfFirstWord = string_length(firstWord);
	return string_substring_from(string, lengthOfFirstWord);
}

void userInterfaceHandler(int socketFD) {
	while (true) {
		char str[100];
		printf("\n\nIngrese un mensaje: \n");
		scanf("%99[^\n]", str);
		char* command = getFirstWord(str);
		char* programPath = getRestOfString(str);
		if (strcmp(command, "start_program")) {
			startProgram(programPath, socketFD);
		} else if (strcmp(command, "end_program")) {
			endProgram(programPath, socketFD);
		} else if (strcmp(command, "disconnect")) {
			disconnect();
		} else if (strcmp(command, "clean")) {
			clean();
		} else if (strcmp(command, "open_file")) {
			sendSignalOpenFile(programPath, socketFD);
		} else {
			printf("No se conoce el mensaje %s\n", str);
		}
		EnviarMensaje(socketFD, str, CONSOLA);
	}
}

int conectarAMemoria(int socketFD) {
	Paquete* paquete = malloc(sizeof(Paquete));
	EnviarMensaje(socketFD, "Mandame la direccion y puerto de la memoria, gato", CONSOLA);
	int result = RecibirPaquete(socketFD, CONSOLA, paquete);
	//buscar en paquete y guardar en variables
	int socketMemory = ConectarServidor(PUERTO_MEMORIA, IP_MEMORIA, MEMORIA, CONSOLA);
	return socketMemory;
}

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	int socketFD = ConectarServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CONSOLA);
	int socketMemory = conectarAMemoria(socketFD);
	pthread_t userInterface;
	int arg[2];
	arg[0] = socketFD;
	arg[1] = socketMemory;
	pthread_create(&userInterface, NULL, userInterfaceHandler, (void*) arg);
	pthread_join(userInterface, NULL);
	close(socketFD);
	return 0;
}
