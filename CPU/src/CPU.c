#include "SocketsL.h"
#include <parser/parser.h>

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

int main(void)
{
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();

	int socketFD = ConectarServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CPU);

	Paquete* paquete = malloc(sizeof(Paquete));
		int result = RecibirPaquete(socketFD, CPU, paquete);
		if(result>0)
		{
			if(paquete->header.tipoMensaje!='1')
			{
				printf("Texto recibido: %s",(char*)paquete->Payload);
				//QUÉ CARAJO HACER????????????


			}
		}
		free(paquete->Payload);
		free(paquete);

	return 0;
}
