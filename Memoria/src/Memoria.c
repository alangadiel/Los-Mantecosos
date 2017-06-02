#include "SocketsL.h"

#define IP_MEMORIA 127.0.0.1
#define DATOS ((uint32_t*)paquete->Payload)

int PUERTO;
int MARCOS;
uint32_t MARCO_SIZE;
int ENTRADAS_CACHE;
int CACHE_X_PROC;
char* REEMPLAZO_CACHE;
int RETARDO_MEMORIA;
char* IP;
t_list* listaHilos;

typedef struct {
	pthread_t hilo;
	int socket;
} structHilo;

typedef struct {
	uint32_t Frame;
	uint32_t PID;
	uint32_t Pag;
} RegistroTablaPaginacion;

typedef struct {
	uint32_t PID;
	uint32_t Pag;
	void* contenido;
} tabla_Cache;

void* bloquePpal;
uint32_t tamanioTotalBytesMemoria;
uint32_t cantidadPaginas = 0;
t_list* TablaPaginacion;
int socketABuscar;
t_list* procesosActivos;

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
					fscanf(file, "%i", &RETARDO_MEMORIA);
					contadorDeVariables++;
					break;
				case 5:
					REEMPLAZO_CACHE = fgets(buffer, sizeof buffer, file);
					strtok(REEMPLAZO_CACHE, "\n");
					contadorDeVariables++;
					break;
				case 4:
					fscanf(file, "%i", &CACHE_X_PROC);
					contadorDeVariables++;
					break;
				case 3:
					fscanf(file, "%i", &ENTRADAS_CACHE);
					contadorDeVariables++;
					break;
				case 2:
					fscanf(file, "%i", &MARCO_SIZE);
					contadorDeVariables++;
					break;
				case 1:
					fscanf(file, "%i", &MARCOS);
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

void IniciarPrograma(uint32_t pid, uint32_t cantPag, int socketFD) {
	//TODO
	//Sacar hardcodeo y programarlo bien
	uint32_t cantPaginasAsignadas = 0;
	if (list_size(TablaPaginacion) < 1) {
		//El checkpoint 2 dice que por ahora hay solo una unica pagina
		list_add(procesosActivos, pid);
		RegistroTablaPaginacion nuevoRegistros;
		nuevoRegistros.Frame = 3;
		nuevoRegistros.PID = pid;
		nuevoRegistros.Pag = 2;
		list_add(TablaPaginacion, &nuevoRegistros);
		cantPaginasAsignadas = cantPag;
	}
	EnviarDatos(socketFD, MEMORIA, &cantPaginasAsignadas, sizeof(uint32_t));
}

void SolicitarBytes(uint32_t pid, uint32_t numPag, uint32_t offset,
		uint32_t tam, int socketFD) {

}
void AlmacenarBytes(Paquete* paquete) {
	//Buscar pagina
	sleep(RETARDO_MEMORIA);	//esperar tiempo definido por arch de config
	memcpy(bloquePpal + DATOS[3], (paquete->Payload) + sizeof(uint32_t) * 5,
	DATOS[4]);
	//printf("%s\n", DATOS[4]);
	printf("Datos Almacenados correctamente! \n"); //TODO: imprimir datos
	printf("El contenido de la memoria es: %s", (char*) bloquePpal);

	//actualizar cache
}

void AsignarPaginas(uint32_t pid, uint32_t cantPag, int socketFD) {

}

void FinalizarPrograma(uint32_t pid) {
	//join de hilo correspondiente
	list_remove_and_destroy_by_condition(procesosActivos,
			LAMBDA(
					bool _(uint32_t pidAEliminar) { return pidAEliminar != pid;}),
			&free);
}

void dumpMemoryContent() {
	char* archivoParaGuardar;
	string_append(archivoParaGuardar, bloquePpal);
	string_append(archivoParaGuardar, "\n");
	printf("%s", archivoParaGuardar);
	char* nombreDelArchivo = "reporte contenido en memoria ";
	string_append(nombreDelArchivo, obtenerTiempoString(time(0)));
	FILE* file = fopen(nombreDelArchivo, "w");
	fputs(archivoParaGuardar, file);
	fclose(file);
}

void dumpMemoryContentOfPID(int pid) {
	char* archivoParaGuardar;
	string_append(archivoParaGuardar, bloquePpal);
	string_append(archivoParaGuardar, "\n");
	printf("%s", archivoParaGuardar);
	char* nombreDelArchivo = "reporte contenido en memoria de pid: ";
	string_append(nombreDelArchivo, string_itoa(pid));
	string_append(nombreDelArchivo, obtenerTiempoString(time(0)));
	FILE* file = fopen(nombreDelArchivo, "w");
	fputs(archivoParaGuardar, file);
	fclose(file);
}

void dumpMemoryStruct() {
	char* archivoParaGuardar;
	int i;
	for (i = 0; i < list_size(TablaPaginacion); i++) {
		RegistroTablaPaginacion* reg;
		reg = list_get(TablaPaginacion, i);
		char* frame = integer_to_string(reg->Frame);
		char* PID = integer_to_string(reg->PID);
		char* pag = integer_to_string(reg->Pag);
		char* strAgregar = "frame: ";
		string_append(strAgregar, frame);
		string_append(strAgregar, "pid: ");
		string_append(strAgregar, PID);
		string_append(strAgregar, "pag: ");
		string_append(strAgregar, pag);
		string_append(strAgregar, "\n");
		string_append(archivoParaGuardar, strAgregar);
	}
	char* procesosActivosParaMostrar = "procesos activos: ";
	for(i = 0; i < list_size(procesosActivos); i++)
	{
		int pid = list_get(procesosActivos, i);
		char* strPid = integer_to_string(pid) ;
		string_append(archivoParaGuardar, ", ");
		string_append(procesosActivosParaMostrar, strPid);
	}
	string_append(archivoParaGuardar, procesosActivosParaMostrar);
	string_append(archivoParaGuardar, "\n");
	printf("%s", archivoParaGuardar);
	char* nombreDelArchivo = "reporte estructuras de memoria ";
	string_append(nombreDelArchivo, obtenerTiempoString(time(0)));
	FILE* file = fopen(nombreDelArchivo, "w");
	fputs(archivoParaGuardar, file);
	fclose(file);
}

void userInterfaceHandler(void* socketFD) {
	int end = 1;
	while (end) {
		char orden[100];
		printf("\n\nIngrese una orden: \n");
		scanf("%s", orden);
		char* command = getWord(orden, 0);
		char* secondWord = getWord(orden, 1);
		char* thirdWord = getWord(orden, 2);
		if (strcmp(command, "retardo​") == 0) {
			if (secondWord != NULL) {
				RETARDO_MEMORIA = integer_to_string(secondWord);
			}
			else {
				printf("El retardo es: %d", RETARDO_MEMORIA);
			}
		}
		else if (strcmp(command, "dump") == 0) {
			if (strcmp(secondWord, "cache") == 0) {
				//TODO mostrar cache
			}
			else if (strcmp(secondWord, "struct") == 0){
				dumpMemoryStruct();
			}
			else if (strcmp(secondWord, "content") == 0) {
				if (thirdWord != NULL) {
					dumpMemoryContentOfPID(integer_to_string(thirdWord));
				}
				else {
					dumpMemoryContent();
				}
			}
		} else if (strcmp(command, "disconnect") == 0) {
			end = 0;
		}
		else if (strcmp(command, "flush cache") == 0) {
			//TODO limpiar cache
		} else if (strcmp(command, "size") == 0) {
			if (strcmp(secondWord, "memory") == 0) {

			}
			else if (secondWord != NULL) {

			}
			else {
				printf("No se conoce el mensaje %s\n", orden);
			}
		} else {
			printf("No se conoce el mensaje %s\n", orden);
		}
	}
}

void* accionHilo(int* socketFD) {
	while (true) {
	}
}

pthread_t agregarAListaHiloSiNoEsta(t_list* listaHilos, int socketFD) {
	socketABuscar = socketFD;
	structHilo* structActual =
			list_find(listaHilos,
					LAMBDA(
							bool _(void* shilo) { return ((structHilo*)shilo)->socket != socketABuscar; }));
	if (structActual != 0) { //el hilo ya existe
		return structActual->hilo;
	} else { // crear hilo y agregar a lista
		pthread_t threadNuevo;
		pthread_create(&threadNuevo, NULL, accionHilo, &socketFD);
		structHilo itemNuevo;
		itemNuevo.hilo = threadNuevo;
		itemNuevo.socket = socketFD;
		list_add(listaHilos, &itemNuevo);
		return threadNuevo;
	}
}

void accion(Paquete* paquete, int socketFD) {

	//pthread_t hilo = agregarAListaHiloSiNoEsta(listaHilos, socketFD);
	switch (paquete->header.tipoMensaje) {
	case ESDATOS:
		//printf("\nTexto recibido: %s\n", (char*)paquete->Payload);
		switch ((*(uint32_t*) paquete->Payload)) {
		(uint32_t*) paquete->Payload++; //Queda un vector de 4 (0..3) posiciones por el ++
	case INIC_PROG:
		IniciarPrograma(DATOS[0], DATOS[1], socketFD);
		break;
	case SOL_BYTES:
		SolicitarBytes(DATOS[0], DATOS[1], DATOS[2], DATOS[3], socketFD);
		break;
	case ALM_BYTES:
		AlmacenarBytes(paquete);
		break;
	case ASIG_PAG:
		AsignarPaginas(DATOS[0], DATOS[1], socketFD);
		break;
	case FIN_PROG:
		FinalizarPrograma(DATOS[0]);
		break;
		}
		break;
	case ESARCHIVO:
		break;

	case ESINT:
		if (strcmp(paquete->header.emisor, KERNEL) == 0) {
			EnviarDatos(socketFD, MEMORIA, &MARCO_SIZE, sizeof(MARCO_SIZE));
		}
		break;
	}
}

int RecibirPaqueteMemoria(int socketFD, char receptor[11], Paquete* paquete) {
	paquete->Payload = malloc(1);
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0) { //si no hubo error
		if (paquete->header.tipoMensaje == ESHANDSHAKE) { //vemos si es un handshake y le respondemos con el tam de pag
			printf("Se establecio conexion con %s\n", paquete->header.emisor);
			Paquete paquete;
			paquete.header.tipoMensaje = ESHANDSHAKE;
			paquete.header.tamPayload = sizeof(uint32_t);
			strcpy(paquete.header.emisor, MEMORIA);
			paquete.Payload = &MARCO_SIZE;
			EnviarPaquete(socketFD, &paquete);
		} else { //recibimos un payload y lo procesamos (por ej, puede mostrarlo)
			paquete->Payload = realloc(paquete->Payload,
					paquete->header.tamPayload);
			resul = RecibirDatos(paquete->Payload, socketFD,
					paquete->header.tamPayload);
		}
	}

	return resul;
}

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	listaHilos = list_create();
	tamanioTotalBytesMemoria = (MARCOS * MARCO_SIZE)
			+ (sizeof(RegistroTablaPaginacion) * MARCOS);
	bloquePpal = malloc(tamanioTotalBytesMemoria); //Reservo toda mi memoria
	TablaPaginacion = list_create();
	procesosActivos = list_create();
	//tabla_Adm tablaAdm[MARCOS]; //no, mejor accedamos casteando y recorriendo el bloquePpal
	/*//MEMORIA CACHE, NO BORRAR
	 tabla_Cache tablaCache[ENTRADAS_CACHE]; //crear y allocar cache
	 int i;
	 for (i = 0; i < MARCOS; ++i)
	 tablaCache[i].contenido = malloc(MARCO_SIZE);
	 */
	pthread_t hiloConsola;
	pthread_create(&hiloConsola, NULL, (void*) userInterfaceHandler, NULL);

	Servidor(IP, PUERTO, MEMORIA, accion, RecibirPaqueteMemoria);

	pthread_join(hiloConsola, NULL);
	/*//MEMORIA CACHE, NO BORRAR
	 for (i = 0; i < MARCOS; ++i)  //liberar cache.
	 free(tablaCache[i].contenido);
	 */
	free(bloquePpal);

	return 0;
}
