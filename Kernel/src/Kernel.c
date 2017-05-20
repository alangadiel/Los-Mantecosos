#include "SocketsL.h"
#include "Helper.h"

#define BACKLOG 6 //Backlog es el maximo de peticiones pendientes
#define LAMBDA(c_) ({ c_ _;}) //Para funciones lamda

int PUERTO_PROG;
int PUERTO_CPU;
char* IP_MEMORIA;
int PUERTO_MEMORIA;
char* IP_FS;
int PUERTO_FS;
int QUANTUM;
int QUANTUM_SLEEP;
char* ALGORITMO;
int GRADO_MULTIPROG;
char* SEM_IDS[4];
int SEM_INIT[100];
char* SHARED_VARS[100];
int STACK_SIZE;
char* IP_PROG;

int ultimoPID=0;
int socketConMemoria;
uint32_t TamanioPagina;
typedef struct  {
 uint32_t size;
 int isFree;
} HeapMetadata;

typedef struct {
	FILE * file;
	BloqueControlProceso pcb;
} PeticionTamanioStack;


uint32_t PidAComparar;
t_list* Nuevos;
t_list* Finalizados;
t_list* Bloqueados;
t_list* Ejecutando;
t_list* Listos;

/*
//replicar aca!!
int j;
for (j = 0; j <= fdmax; j++) {
		// ¡enviar a todo el mundo!
		if (FD_ISSET(j, &master)) {
			// excepto al listener y a nosotros mismos
			if (j != SocketEscucha && j != i) {
				EnviarMensaje(j,(char*)paquete->Payload,KERNEL, ESSTRING);
				//if (send(j, paquete, result, 0) == -1) {
					//perror("send");
				//}
			}
		}
	}
*/
char* ObtenerTextoDeArchivoSinCorchetes(FILE* f) //Para obtener los valores de los arrays del archivo de configuracion
{
	char buffer[10000];
	char *line = fgets(buffer,sizeof buffer,f);
	int length = string_length(line)-3;
	char *texto = string_substring(line,1,length);

	texto  = strtok(texto,",");

	return texto;
}


void ObtenerTamanioPagina(int socketFD){
	Paquete* datosInicialesMemoria = malloc(sizeof(Paquete));
	uint32_t datosRecibidos = RecibirPaqueteCliente(socketFD,KERNEL,datosInicialesMemoria);
	if(datosRecibidos>0){
		TamanioPagina = *((uint32_t*)datosInicialesMemoria->Payload);
	}
	free(datosInicialesMemoria->Payload+1);
	free(datosInicialesMemoria);

}

BloqueControlProceso CrearNuevoProceso(){
	//Creo el pcb y lo guardo en la lista de nuevos
	BloqueControlProceso pcb;
	pcb.PID = ultimoPID+1;
	ultimoPID++;
	list_add(Nuevos,&pcb);
	return pcb;
}
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
				/*if (contadorDeVariables == 15)
				{
					fscanf(file, "%i", &TAM_PAG);
					contadorDeVariables++;

				}*/
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
						SHARED_VARS[i++] = texto;
						texto = strtok (NULL, ",");
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

void accion(Paquete* paquete, int socketConectado){
	switch(paquete->header.tipoMensaje) {

		case ESSTRING:

				if(strcmp(paquete->header.emisor,CONSOLA)==0)
				{
					double tamanioArchivo = paquete->header.tamPayload/TamanioPagina;
					double tamanioTotalPaginas = ceil(tamanioArchivo+STACK_SIZE);
					BloqueControlProceso pcb = CrearNuevoProceso();
					//Manejo la multiprogramacion
					if(GRADO_MULTIPROG - list_size(Ejecutando) - list_size(Listos) > 0 && list_size(Nuevos) >= 1){
						//Pregunta a la memoria si me puede guardar estas paginas
						uint32_t paginasConfirmadas = IM_InicializarPrograma(socketConMemoria,KERNEL,pcb.PID,tamanioTotalPaginas);
						if(paginasConfirmadas == tamanioTotalPaginas) // N° negativo significa que la memoria no tiene espacio
						{
							pcb.PaginasDeCodigo = tamanioTotalPaginas;
							//Saco el programa de la lista de NEW y lo agrego el programa a la lista de READY
							PidAComparar = pcb.PID;
							list_remove_by_condition(Nuevos, LAMBDA(bool _(BloqueControlProceso* pcb) { return pcb->PID == PidAComparar; }));
							list_add(Listos,&pcb);

							//Solicito a la memoria que me guarde el codigo del programa
							IM_GuardarDatos(socketConMemoria, KERNEL, pcb.PID, 0, 0, paquete->header.tamPayload, paquete->Payload); //TODO: sacar harcodeo

						}
						else
						{
							EnviarMensaje(socketConectado,"No se pudo guardar el programa",KERNEL);
						}

					}

				}

		break;
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
	Nuevos = list_create();
	Finalizados= list_create();
	Bloqueados= list_create();
    Ejecutando= list_create();
	Listos= list_create();

	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	while((socketConMemoria = ConectarAServidor(PUERTO_MEMORIA,IP_MEMORIA,MEMORIA,KERNEL, RecibirHandshake_KernelDeMemoria))<0);
	Servidor(IP_PROG, PUERTO_PROG, KERNEL, accion, RecibirPaqueteServidor);


	return 0;
}
