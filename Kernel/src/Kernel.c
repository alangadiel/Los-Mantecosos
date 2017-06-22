#include "Service.h"
#include "UserInterface.h"
#include "Threads.h"

//Variables archivo de configuracion
char* IP_PROG;
int PUERTO_PROG;
int PUERTO_CPU;
char* IP_MEMORIA;
int PUERTO_MEMORIA;
char* IP_FS;
int PUERTO_FS;


t_list* HilosDeConexiones;

void obtenerValoresArchivoConfiguracion() {
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
				if (contadorDeVariables == 14)
				{
					char buffer[10000];

					IP_PROG = fgets(buffer, sizeof buffer, file);
					strtok(IP_PROG, "\n");
					contadorDeVariables++;
				}

				if (contadorDeVariables == 13)
				{
					fscanf(file, "%i", &STACK_SIZE);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 12)
				{
					char* texto = ObtenerTextoDeArchivoSinCorchetes(file);
					int i = 0;

					while (texto != NULL)
					{
						SHARED_VARS[i] = texto;
						texto = strtok (NULL, ",");
						//Inicializo las variables compartidas
						VariableCompartida nuevaVar;
						nuevaVar.nombreVariableGlobal = texto;
						nuevaVar.valorVariableGlobal=0;
						list_add(VariablesCompartidas,&nuevaVar);
						i++;
					}

					contadorDeVariables++;
				}

				if (contadorDeVariables == 11)
				{
					char* texto = ObtenerTextoDeArchivoSinCorchetes(file);
					int i = 0;

					while (texto != NULL)
					{
						SEM_INIT[i++] = atoi(texto);
						texto = strtok (NULL, ",");
					}

					contadorDeVariables++;
				}

				if (contadorDeVariables == 10){
					char * texto = ObtenerTextoDeArchivoSinCorchetes(file);
					int i = 0;

					while (texto != NULL)
					{
						SEM_IDS[i++] = texto;
						texto = strtok (NULL, ",");
					}

					contadorDeVariables++;
				}

				if (contadorDeVariables == 9)
				{
					fscanf(file, "%i", &GRADO_MULTIPROG);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 8)
				{
					char buffer[10000];

					ALGORITMO = fgets(buffer, sizeof buffer, file);
					strtok(ALGORITMO, "\n");

					contadorDeVariables++;
				}

				if (contadorDeVariables == 7)
				{
					fscanf(file, "%i", &QUANTUM_SLEEP);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 6)
				{
					fscanf(file, "%i", &QUANTUM);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 5)
				{
					fscanf(file, "%i", &PUERTO_FS);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 4)
				{
					char buffer[10000];

					IP_FS = fgets(buffer, sizeof buffer, file);
					strtok(IP_FS, "\n");

					contadorDeVariables++;
				}

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
					fscanf(file, "%i", &PUERTO_CPU);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 0)
				{
					fscanf(file, "%i", &PUERTO_PROG);
					contadorDeVariables++;
				}
			}
		}

		fclose(file);
	}
}

void RecibirHandshake_KernelDeMemoria(int socketFD, char emisor[11]) {
	Paquete* paquete =  malloc(sizeof(Paquete));
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0 && paquete->header.tipoMensaje == ESHANDSHAKE) { //si no hubo error y es un handshake
		if (strcmp(paquete->header.emisor, emisor) == 0) {
				printf("\nConectado con el servidor!\n");
				if(strcmp(paquete->header.emisor, MEMORIA) == 0){
					paquete->Payload = malloc(paquete->header.tamPayload);
					resul = RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
					TamanioPagina = *((uint32_t*)paquete->Payload);
					free(paquete->Payload);
				}
		} else
			perror("Error, no se recibio un handshake del servidor esperado\n");
	} else
	perror("Error de Conexion, no se recibio un handshake\n");


	free(paquete);
}

int main(void)
{
	CrearListas(); //TODO: Cargar listas de semaforos y variables globales
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	while((socketConMemoria = ConectarAServidor(PUERTO_MEMORIA,IP_MEMORIA,MEMORIA,KERNEL, RecibirHandshake_KernelDeMemoria))<0);
	while((socketConFS = ConectarAServidor(PUERTO_FS,IP_FS,FS,KERNEL, RecibirHandshake))<0);

	//pthread_t hiloSyscallWrite;
	//pthread_create(&hiloSyscallWrite, NULL, (void*)syscallWrite, 2); //socket 2 hardcodeado
	//pthread_join(hiloSyscallWrite, NULL);

	pthread_t hiloConsola;
	pthread_create(&hiloConsola, NULL, (void*)userInterfaceHandler, &socketConMemoria);

	pthread_t hiloDispatcher;
	pthread_create(&hiloDispatcher, NULL, (void*)dispatcher, NULL);
	ServidorConcuerrente(IP_PROG, PUERTO_PROG, KERNEL,&HilosDeConexiones, &end, accion);

	pthread_join(hiloConsola, NULL);
	pthread_join(hiloDispatcher, NULL);
	LimpiarListas();

	return 0;
}
