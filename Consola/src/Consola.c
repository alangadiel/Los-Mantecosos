#include "SocketsL.h"

#define TAMANIOMAXIMOFIJO 20
#define true 1
#define false 0
#ifndef NULL
#define NULL   ((void *) 0)
#endif

char* IP_KERNEL;
int PUERTO_KERNEL;

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


	int socketFD = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CONSOLA);


void disconnect() {

}


void clean() {
	system ( "clear" ); // Clear screen and home cursor
}

void sendSignalOpenFile(char* programPath, int socketFD) {
	FILE* fileForSend = fopen(programPath, "r");
	char * buffer = 0;
	long length;

	if (fileForSend)
	{
	  fseek (fileForSend, 0, SEEK_END);
	  length = ftell (fileForSend);
	  fseek (fileForSend, 0, SEEK_SET);
	  buffer = malloc (length);
	  if (buffer)
	  {
	    fread (buffer, 1, length, fileForSend);
	  }
	  fclose (fileForSend);
	}
	EnviarMensaje(socketFD,buffer, CONSOLA);
}

char* getWord(char* string, int pos) {
	char delimiter[] = " ";
	char *word, *context;
	int inputLength = strlen(string);
	char *inputCopy = (char*) calloc(inputLength + 1, sizeof(char));
	strncpy(inputCopy, string, inputLength);
	if (pos == 0){
		return strtok_r (inputCopy, delimiter, &context);
	}
	else{
		int i;
		for (i = 1; i <= pos; i++) {
			word = strtok_r (NULL, delimiter, &context);
		}
	}
	return word;
}

void userInterfaceHandler(void* socketFD) {
	while (true) {
		char str[100];
		printf("\n\nIngrese un mensaje: \n");
		scanf("%99[^\n]", str);
		char* command = getWord(str, 0);
		char* programPath = getWord(str, 1);
		if (strcmp(command, "start_program") == 0) {
			startProgram(programPath, (int) socketFD);
			endProgram(programPath, (int) socketFD);
		} else if (strcmp(command, "disconnect") == 0) {
			disconnect();
		} else if (strcmp(command, "clean") == 0) {
			clean();
		} else if (strcmp(command, "open_file") == 0) {
			sendSignalOpenFile(programPath, (int) socketFD);
		} else {
			printf("No se conoce el mensaje %s\n", str);
		}
	}
}

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	int socketFD = ConectarServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CONSOLA);
	pthread_t userInterface;
	void* arg = socketFD;
	pthread_create(&userInterface, NULL, userInterfaceHandler, (void*) arg);
	pthread_join(userInterface, NULL);
	close(socketFD);
	return 0;
}
