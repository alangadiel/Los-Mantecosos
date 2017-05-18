#include "SocketsL.h"

char* IP_KERNEL;
int PUERTO_KERNEL;
char* IP_MEMORIA;
int PUERTO_MEMORIA;

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
				if (contadorDeVariables == 3)
				{
					fscanf(file, "%i", &PUERTO_MEMORIA);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 2)
				{
					char buffer[10000];

					IP_MEMORIA = fgets(buffer, sizeof buffer, file);
					strtok(IP_MEMORIA, "\n");

					contadorDeVariables++;
				}
				if (contadorDeVariables == 1)
				{
					fscanf(file, "%i", &PUERTO_KERNEL);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 0)
				{
					char buffer[10000];

					IP_MEMORIA = fgets(buffer, sizeof buffer, file);
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


int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	int socketKernel = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CPU);
	int socketMEMORIA = ConectarAServidor(PUERTO_MEMORIA, IP_MEMORIA, MEMORIA, CPU);
	return 0;
}
