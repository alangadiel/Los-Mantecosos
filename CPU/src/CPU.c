#include <parser/metadata_program.h>
#include "Primitivas.h"

char* IP_KERNEL;
int PUERTO_KERNEL;
char* IP_MEMORIA;
int PUERTO_MEMORIA;
int socketKernel;
int socketMemoria;
static const char* PROGRAMA =
		"begin\n"
		"variables a, b\n"
		"a = 3\n"
		"b = 5\n"
		"a = b + 12\n"
		"end\n"
		"\n";

AnSISOP_funciones functions = {
  .AnSISOP_definirVariable = primitiva_definirVariable,
  /*.AnSISOP_obtenerPosicionVariable= dummy_obtenerPosicionVariable,
  .AnSISOP_finalizar     = dummy_finalizar,
  .AnSISOP_dereferenciar   = dummy_dereferenciar,
  .AnSISOP_asignar    = dummy_asignar,*/

};


AnSISOP_kernel kernel_functions = { };

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


int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	socketKernel = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CPU, RecibirHandshake);
	socketMemoria = ConectarAServidor(PUERTO_MEMORIA, IP_MEMORIA, MEMORIA, CPU, RecibirHandshake);

	Paquete* paquete = malloc(sizeof(Paquete));
	uint32_t datosRecibidos = RecibirPaqueteCliente(socketKernel, CPU, paquete);
	while(datosRecibidos<=0){
		datosRecibidos = RecibirPaqueteCliente(socketKernel, CPU, paquete);
	}
	if (datosRecibidos > 0)
	{
		switch(paquete -> header.tipoMensaje)
		{
			case ESSTRING:
				if (strcmp(paquete -> header.emisor, MEMORIA) == 0)
				{
					printf("Texto recibido: %s", (char*)paquete -> Payload);
					analizadorLinea(PROGRAMA, &functions, &kernel_functions);
				}
			break;

			case ESPCB:
				if (strcmp(paquete -> header.emisor, KERNEL) == 0)
				{
					//BloqueControlProceso pcb = (BloqueControlProceso) paquete -> Payload;
				}
			break;
		}
	}
	//Hago free
	free(paquete -> Payload);
	free(paquete);
	return 0;
}
