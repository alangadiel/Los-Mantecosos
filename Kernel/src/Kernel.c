#include "Service.h"
#include "ThreadsKernel.h"
#include "UserInterface.h"

int cantNombresSemaforos = 0;
int cantValoresSemaforos = 0;

t_list* HilosDeConexiones;

void agregarIdSemaforo(char* item){
	Semaforo* sem = malloc(sizeof(Semaforo));
	sem->nombreSemaforo = string_duplicate(item);
	sem->listaDeProcesos = queue_create();
	list_add(Semaforos, sem);
	cantNombresSemaforos++;
}

void agregarValorSemaforo(char* item) {
	Semaforo* sem = list_get(Semaforos,cantValoresSemaforos);
	sem->valorSemaforo = atoi(item);
	cantValoresSemaforos++;
}

void agregarSharedVar(char* item) {
	VariableCompartida* nuevaVar = malloc(sizeof(VariablesCompartidas));
	nuevaVar->nombreVariableGlobal = item;
	nuevaVar->valorVariableGlobal=0;
	list_add(VariablesCompartidas,nuevaVar);
}

void LlenarListas() {
	string_iterate_lines(SEM_IDS, agregarIdSemaforo);
	string_iterate_lines(SEM_INIT, agregarValorSemaforo);
	string_iterate_lines(SHARED_VARS, agregarSharedVar);
}

void obtenerValoresArchivoConfiguracion(bool* cantNombresSemaforosEsIgualAValores) {
	//TODO: usar cantNombresSemaforosEsIgualAValores
	t_config* arch = config_create("ArchivoConfiguracion.txt");
	PUERTO_PROG = config_get_int_value(arch, "PUERTO_PROG");
	PUERTO_CPU =  config_get_int_value(arch, "PUERTO_CPU");
	IP_MEMORIA = string_duplicate(config_get_string_value(arch, "IP_MEMORIA"));
	PUERTO_MEMORIA = config_get_int_value(arch, "PUERTO_MEMORIA");
	IP_FS = string_duplicate(config_get_string_value(arch, "IP_FS"));
	PUERTO_FS =  config_get_int_value(arch, "PUERTO_FS");
	QUANTUM =  config_get_int_value(arch, "QUANTUM");
	QUANTUM_SLEEP =  config_get_int_value(arch, "QUANTUM_SLEEP");
	ALGORITMO = string_duplicate(config_get_string_value(arch, "ALGORITMO"));
	GRADO_MULTIPROG =  config_get_int_value(arch, "GRADO_MULTIPROG");
	SEM_IDS = config_get_array_value(arch, "SEM_IDS");
	SHARED_VARS = config_get_array_value(arch, "SHARED_VARS");
	SEM_INIT = config_get_array_value(arch, "SEM_INIT");
	STACK_SIZE =  config_get_int_value(arch, "STACK_SIZE");
	IP_PROG = string_duplicate(config_get_string_value(arch, "IP_PROG"));
	config_destroy(arch);
}

void RecibirHandshake_KernelDeMemoria(int socketFD, char emisor[11]) {
	Paquete* paquete =  malloc(sizeof(Paquete));
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0 && paquete->header.tipoMensaje == ESHANDSHAKE) { //si no hubo error y es un handshake
		if (strcmp(paquete->header.emisor, emisor) == 0) {
			printf("Conectado con el servidor %s\n", emisor);
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
	bool cantNombresSemaforosEsIgualAValores = true;
	CrearListasYSemaforos();
	obtenerValoresArchivoConfiguracion(&cantNombresSemaforosEsIgualAValores);
	LlenarListas();
	if(cantNombresSemaforosEsIgualAValores)
	{
		planificacion_detenida = false;
		imprimirArchivoConfiguracion();
		socketConMemoria = ConectarAServidor(PUERTO_MEMORIA,IP_MEMORIA,MEMORIA,KERNEL, RecibirHandshake_KernelDeMemoria);
		socketConFS = ConectarAServidor(PUERTO_FS,IP_FS,FS,KERNEL, RecibirHandshake);
		//pthread_t hiloSyscallWrite;
		//pthread_create(&hiloSyscallWrite, NULL, (void*)syscallWrite, 2); //socket 2 hardcodeado
		//pthread_join(hiloSyscallWrite, NULL);

		pthread_t hiloConsola;
		pthread_create(&hiloConsola, NULL, (void*)userInterfaceHandler, &socketConMemoria);

		pthread_t hiloDispatcher;
		pthread_create(&hiloDispatcher, NULL, (void*)dispatcher, NULL);

		ServidorConcurrente(IP_PROG, PUERTO_PROG, KERNEL,&HilosDeConexiones, &end, accion);

		pthread_join(hiloConsola, NULL);
		pthread_join(hiloDispatcher, NULL);
		LiberarVariablesYListas();
	}
	else
	{
		printf("\nError: La cantidad de semaforos no concuerda con la cantidad de valores. Finalizando proceso\n");
	}
	return 0;
}
