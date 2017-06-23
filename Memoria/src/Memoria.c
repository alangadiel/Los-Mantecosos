#include "Memoria.h"

//Variables del Archivo de Config
int PUERTO;
uint32_t MARCOS;
uint32_t MARCO_SIZE;
uint32_t ENTRADAS_CACHE;
uint32_t CACHE_X_PROC;
char* REEMPLAZO_CACHE;
unsigned int RETARDO_MEMORIA;
char* IP;

void* BloquePrincipal;
void* ContenidoMemoria;
int tamanioTotalBytesMemoria;
int tamEstructurasAdm;
int cantPagAsignadas;
int socketABuscar;

t_list* tablaCache;
t_list* listaHilos;
bool end;


void obtenerValoresArchivoConfiguracion() {
	int contadorDeVariables = 0;
	int c;
	FILE *file;
	file = fopen("ArchivoConfiguracion.txt", "r");
	if (file) {
		while ((c = getc(file)) != EOF)
			if (c == '=') {
				char buffer[10000];
				switch (contadorDeVariables) {
				case 7:
					IP = fgets(buffer, sizeof buffer, file);
					strtok(IP, "\n");
					break;
				case 6:
					fscanf(file, "%u", &RETARDO_MEMORIA);
					contadorDeVariables++;
					break;
				case 5:
					REEMPLAZO_CACHE = fgets(buffer, sizeof buffer, file);
					strtok(REEMPLAZO_CACHE, "\n");
					contadorDeVariables++;
					break;
				case 4:
					fscanf(file, "%u", &CACHE_X_PROC);
					contadorDeVariables++;
					break;
				case 3:
					fscanf(file, "%u", &ENTRADAS_CACHE);
					contadorDeVariables++;
					break;
				case 2:
					fscanf(file, "%u", &MARCO_SIZE);
					contadorDeVariables++;
					break;
				case 1:
					fscanf(file, "%u", &MARCOS);
					contadorDeVariables++;
					break;
				case 0:
					fscanf(file, "%i", &PUERTO);
					contadorDeVariables++;
					break;
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
void InicializarTablaDePagina() {
	uint32_t i;
	for(i=0;i<MARCOS;i++){
		TablaDePagina[i].Frame = i;
	}
}

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	tablaCache = list_create();

	tamEstructurasAdm = sizeof(RegistroTablaPaginacion) * MARCOS;
	tamanioTotalBytesMemoria = (MARCOS * MARCO_SIZE) + tamEstructurasAdm;

	BloquePrincipal = malloc(tamanioTotalBytesMemoria); //Reservo toda mi memoria
	ContenidoMemoria = BloquePrincipal + tamEstructurasAdm; //guardo el puntero donde empieza el contenido
	cantPagAsignadas = 0;

	InicializarTablaDePagina();

	pthread_t hiloConsola;
	pthread_create(&hiloConsola, NULL, (void*) userInterfaceHandler, NULL);

	ServidorConcuerrente(IP, PUERTO, MEMORIA, &listaHilos, &end, accion);

	pthread_join(hiloConsola, NULL);
	list_destroy_and_destroy_elements(tablaCache, free);
	free(BloquePrincipal);
	exit(3); //TODO: ¿Esto va? Se supone que termina todos los hilos del proceso.
	return 0;
}
