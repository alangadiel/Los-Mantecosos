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
#define EMISORCONSOLA "Consola"

char* IP_KERNEL;
int PUERTO_KERNEL;


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
				if (contadorDeVariables == 1)
				{
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


char* integer_to_string(int x)
{
    char* buffer = malloc(sizeof(char) * sizeof(int) * 4 + 1);

    if (buffer)
    {
         sprintf(buffer, "%d", x);
    }

    return buffer; // caller is expected to invoke free() on this buffer to release memory
}


int main(void)
{
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();

	int socketFD = ConectarServidor(PUERTO_KERNEL, IP_KERNEL);
	EnviarHandshake(socketFD,EMISORCONSOLA);

	Header* header = malloc(TAMANIOHEADER);
	int resul = RecibirPaquete(header, socketFD, TAMANIOHEADER);
		if(resul<0){
			printf("Socket %d: ", socketFD);
			perror("Error de Recepcion, no se pudo leer el mensaje\n");
			close(socketFD); // ¡Hasta luego!
		} else if (resul==0){

			perror("Error de Conexion, no se pudo leer el mensaje porque se cerro la conexion\n");
			close(socketFD); // ¡Hasta luego!

		} else {
		//vemos si es un handshake
			if (header->esHandShake=='1'){
				printf("\nConectado con el servidor!\n");

			} else {
				perror("Error de Conexion, no se recibio un handshake\n");
			}
		}

	char str[100];
	printf("\n\nIngrese un mensaje: \n");
	scanf("%99[^\n]",str);

	EnviarMensaje(socketFD,str,EMISORCONSOLA);

 	close(socketFD);
	return 0;
}
