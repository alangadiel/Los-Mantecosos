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
	direccionKernel.sin_addr.s_addr = inet_addr(IP_KERNEL);
	memset(&(direccionKernel.sin_zero), '\0', 8);
	connect(socketFD,(struct sockaddr *)&direccionKernel, sizeof(struct sockaddr));
	//printf("%s", "Se conecto! anda a kernel y apreta\n");
	return socketFD;
}


int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	//Iniciamos el cliente y lo conectamos con el servidor Kernel
	int socketFD= ConectarServidor();
	char* str;

	printf("%s\n","1 -Conectando con el servidor, acepte la conexion y luego ingrese un texto de hasta %d caracteres para enviar el Handshake-",TAMANIOMAXIMOFIJO);
	scanf("%s",str);

	//Intercambiamos Handshakes
	EnviarHandshake(socketFD,"Consola");
	printf("%s\n","2 -Handshake enviado, Ahora recibalo en el servidor, luego ingrese un texto de hasta %d caracteres-",TAMANIOMAXIMOFIJO);
	scanf("%s",str);

	char* result = RecibirHandshake(socketFD);
	printf("%s\n",result);

	char* mensajeAEnviar;
	printf("%s\n","3 -Ingrese el mensaje a enviar, un texto de hasta %d caracteres-",TAMANIOMAXIMOFIJO);
	//fgets(mensajeAEnviar, 100, stdin);
	scanf("%s",mensajeAEnviar);
	//Enviamos el mensaje
	mensajeAEnviar = string_substring_until(mensajeAEnviar,20);
	EnviarMensaje(socketFD,mensajeAEnviar,"Consola");
	printf("%s\n", "4. Mensaje enviado");

	scanf("%s", mensajeAEnviar);
	return 0;
}
