/*
 ============================================================================
 Name        : Consola.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */


#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/txt.h>
#include "SocketsL.h"

#define TAMANIOMAXIMOFIJO 20
char* IP_KERNEL;
int PUERTO_KERNEL;

void obtenerValoresArchivoConfiguracion() {
	int contadorDeVariables = 0;
	int c;
	FILE *file;
	file = fopen("ArchivoConfiguracion.txt", "r");
	if (file) {
		while ((c = getc(file)) != EOF)
			if (c == '=')
			{
				if (contadorDeVariables == 1) {
					fscanf(file, "%i", &PUERTO_KERNEL);
				}
				if (contadorDeVariables == 0)
				{
					char buffer[10000];
					IP_KERNEL = fgets(buffer, sizeof buffer, file);
					strtok(IP_KERNEL, "\n");

					contadorDeVariables++;
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
		while ((c = getc(file)) != EOF)
			putchar(c);
		fclose(file);
	}
}

int ConectarServidor(){
	int socketFD = socket(AF_INET,SOCK_STREAM,0);
	printf("%i\n", socketFD);
	struct sockaddr_in direccionKernel;
	direccionKernel.sin_family = AF_INET;
	direccionKernel.sin_port = htons(PUERTO_KERNEL);
	direccionKernel.sin_addr.s_addr = (int) htonl(IP_KERNEL);
	connect(socketFD,(struct sockaddr *)&direccionKernel, sizeof(struct sockaddr));
	printf("%s", "Se conecto! anda a kernel y apreta\n");
	return socketFD;
}


int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	int socketFD= ConectarServidor();
	EnviarHandshake(socketFD,"Consola");
	char* result = RecibirHandshake(socketFD);
	printf("%s\n",result);
	char* mensajeAEnviar;
	printf("%s\n","Ingrese un texto de hasta %d caracteres",TAMANIOMAXIMOFIJO);
	scanf("%s",mensajeAEnviar);
	mensajeAEnviar = string_substring_until(mensajeAEnviar,20);
	EnviarMensaje(socketFD,mensajeAEnviar,"Consola");
	return 0;
}
