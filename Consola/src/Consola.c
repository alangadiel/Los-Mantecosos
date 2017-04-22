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

	char str[100];

	printf("\n\nIngrese un mensaje: \n");
	scanf("%99[^\n]",str);

	EnviarHandshakeString(socketFD);
	EnviarMensajeString(socketFD,str);

	/*printf("%s\n","1 -Conectando con el servidor, acepte la conexion y luego ingrese un texto de hasta %d caracteres para enviar el Handshake-",TAMANIOMAXIMOFIJO);
	scanf("%s",str);*/
	//EnviarHandshake(socketFD);

	/*string_append(&mensajeAMandar,largo);
	free(largo);*/
	/*mensajeAMandar = strcat(mensajeAMandar,largo);
	mensajeAMandar = strcat(mensajeAMandar,str);*/
	/*printf("%s\n","2 -Handshake enviado, Ahora recibalo en el servidor, luego ingrese un texto de hasta %d caracteres-",TAMANIOMAXIMOFIJO);
  	scanf("%s",str);*/
	/*char* result = RecibirHandshake(socketFD);
	printf("%s\n",result);*/
	/*char* mensajeAEnviar;
	printf("%s\n","3 -Ingrese el mensaje a enviar, un texto de hasta %d caracteres-",TAMANIOMAXIMOFIJO);
	 //fgets(mensajeAEnviar, 100, stdin);
	  	scanf("%s",mensajeAEnviar);*/

	//Enviamos el mensaje
	/*mensajeAEnviar = string_substring_until(mensajeAEnviar,20);
	EnviarMensaje(socketFD,mensajeAEnviar);*/
	//printf("%s","Mensaje enviado");
 	//close(socketFD);
	return 0;
}
