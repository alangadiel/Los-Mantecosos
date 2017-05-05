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

	int socketFD = ConectarServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CONSOLA);

	char str[100];
	printf("\n\nIngrese un mensaje: \n");
	scanf("%99[^\n]",str);

	EnviarMensaje(socketFD,str,CONSOLA, ESSTRING);

 	close(socketFD);
	return 0;
}
