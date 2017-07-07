#include "Memoria.h"

//Variables del Archivo de Config
int PUERTO;
uint32_t MARCOS;
uint32_t MARCO_SIZE;
uint32_t ENTRADAS_CACHE;
uint32_t CACHE_X_PROC;
char* REEMPLAZO_CACHE;
int RETARDO_MEMORIA;
char* IP;

void* BloquePrincipal;
void* ContenidoMemoria;
int tamanioTotalBytesMemoria;
int tamEstructurasAdm;
int cantPagAsignadas;
int socketABuscar;
t_list* tablaCacheRastro;
t_list* tablaCache;
t_list* listaHilos;
bool end;

//semaforos
pthread_mutex_t mutexTablaCache = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexTablaPagina = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t mutexContenidoMemoria = PTHREAD_MUTEX_INITIALIZER;

void obtenerValoresArchivoConfiguracion() {
	t_config* arch = config_create("ArchivoConfiguracion.txt");
	PUERTO= config_get_int_value(arch, "PUERTO");
	MARCOS=config_get_int_value(arch, "MARCOS");
	MARCO_SIZE=config_get_int_value(arch, "MARCO_SIZE");
	ENTRADAS_CACHE=config_get_int_value(arch, "ENTRADAS_CACHE");
	CACHE_X_PROC=config_get_int_value(arch, "CACHE_X_PROC");
	REEMPLAZO_CACHE=string_duplicate(config_get_string_value(arch, "REEMPLAZO_CACHE"));
	RETARDO_MEMORIA=config_get_int_value(arch, "RETARDO_MEMORIA");
	IP=string_duplicate(config_get_string_value(arch, "IP"));
	config_destroy(arch);
}

void InicializarTablaDePagina() {
	uint32_t i;
	for(i=0;i<MARCOS;i++){
		TablaDePagina[i].Frame = i;
		TablaDePagina[i].PID = 0;
		TablaDePagina[i].Pag = 0;
		TablaDePagina[i].disponible = true;
	}
}

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	tablaCache = list_create();
	tablaCacheRastro = list_create();

	tamEstructurasAdm = sizeof(RegistroTablaPaginacion) * MARCOS;
	tamanioTotalBytesMemoria = (MARCOS * MARCO_SIZE) + tamEstructurasAdm;
	BloquePrincipal = malloc(tamanioTotalBytesMemoria); //Reservo toda mi memoria
	ContenidoMemoria = BloquePrincipal + tamEstructurasAdm; //guardo el puntero donde empieza el contenido
	cantPagAsignadas = 0;

	InicializarTablaDePagina();

	pthread_t hiloConsola;
	pthread_create(&hiloConsola, NULL, (void*) userInterfaceHandler, NULL);

	ServidorConcurrente(IP, PUERTO, MEMORIA, &listaHilos, &end, accion);

	pthread_join(hiloConsola, NULL);

	//Liberar memoria
	list_destroy_and_destroy_elements(tablaCache, free);
	list_destroy_and_destroy_elements(tablaCacheRastro, free);
	free(BloquePrincipal); free(IP); free(REEMPLAZO_CACHE);

	exit(3); //TODO: ¿Esto va? Se supone que termina todos los hilos del proceso.
	return 0;
}
