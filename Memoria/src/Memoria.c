#include "SocketsL.h"

#define IP_MEMORIA 127.0.0.1
#define DATOS ((uint32_t*)paquete.Payload)

int PUERTO;
uint32_t MARCOS;
uint32_t MARCO_SIZE;
uint32_t ENTRADAS_CACHE;
uint32_t CACHE_X_PROC;
char* REEMPLAZO_CACHE;
unsigned int RETARDO_MEMORIA;
char* IP;
t_list* listaHilos;
char end;

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
int tamanioTotalBytesMemoria;
int cantidadPaginas = 0;
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

void IniciarPrograma(uint32_t pid, uint32_t cantPag, int socketFD) {
	//TODO
	//Sacar hardcodeo y programarlo bien
	uint32_t cantPaginasAsignadas = 0;
	if (list_size(TablaPaginacion) < 1) { //El checkpoint 2 dice que por ahora hay solo una unica pagina
		uint32_t* PID = malloc(sizeof(uint32_t));
		*PID=pid;
		list_add(procesosActivos, PID);
		RegistroTablaPaginacion* nuevoRegistros = malloc(sizeof(RegistroTablaPaginacion));
		nuevoRegistros->Frame = 3;
		nuevoRegistros->PID = pid;
		nuevoRegistros->Pag = 2;
		list_add(TablaPaginacion, nuevoRegistros);
		cantPaginasAsignadas = cantPag;
	}
	EnviarDatos(socketFD, MEMORIA, &cantPaginasAsignadas, sizeof(uint32_t));
}

void SolicitarBytes(uint32_t pid, uint32_t numPag, uint32_t offset,
		uint32_t tam, int socketFD) {

}
void AlmacenarBytes(Paquete paquete) {
	//Buscar pagina
	sleep(RETARDO_MEMORIA);//esperar tiempo definido por arch de config
	memcpy(bloquePpal+DATOS[3],(paquete.Payload) + sizeof(uint32_t)*5,DATOS[4]);
	//printf("%s\n", DATOS[4]);
	printf("Datos Almacenados correctamente! \n");
	printf("El contenido de la memoria es: %s",(char*)bloquePpal);
	//actualizar cache
}

void AsignarPaginas(uint32_t pid, uint32_t cantPag, int socketFD) {

}

void LiberarPaginas(uint32_t pid, uint32_t numPag, int socketFD) {
/*
 * Ante un pedido de liberación de página por parte del kernel, el proceso memoria deberá liberar
  la página que corresponde con el número solicitado. En caso de que dicha página no exista
  o no pueda ser liberada, se deberá informar de la imposibilidad de realizar dicha operación
  como una excepcion de memoria.
 */
}

void FinalizarPrograma(uint32_t pid) {
	//join de hilo correspondiente
	list_remove_and_destroy_by_condition(procesosActivos, LAMBDA(bool _(void* pidAEliminar) { return *(uint32_t*)pidAEliminar != pid;}), free);
}

void dumpMemoryContent() {
	printf("Imprimiendo todo el contenido de la memoria");
	printf("%*s", tamanioTotalBytesMemoria, (char*)bloquePpal);
	char* nombreDelArchivo = malloc(64+27+1);//tam de la hora + tam de "Contenido de la memoria en " + \0
	nombreDelArchivo = "Contenido de la memoria en ";
	string_append(&nombreDelArchivo, obtenerTiempoString(time(0)));
	FILE* file = fopen(nombreDelArchivo, "w");
	fprintf(file, "%*s", tamanioTotalBytesMemoria, (char*)bloquePpal);
	fclose(file);
	free(nombreDelArchivo);
}

void dumpMemoryContentOfPID(uint32_t pid) {
	printf("Imprimiendo el contenido en memoria del proceso %d", pid);
	//TODO
}

void dumpMemoryStruct() { //TODO
	printf("Imprimiendo las estructuras de la memoria");
	char* archivoParaGuardar;
	int i;
	for (i = 0; i < list_size(TablaPaginacion); i++) {
		RegistroTablaPaginacion* reg;
		reg = list_get(TablaPaginacion, i);
		char* frame = string_itoa(reg->Frame);
		char* PID = string_itoa(reg->PID);
		char* pag = string_itoa(reg->Pag);
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
		char* strPid = string_itoa(pid) ;
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

void printMemorySize() {
	//TODO

	printf("Imprimiendo el tamaño total de la memoria");
}
void printProcessSize(uint32_t pid) {
	//TODO

	printf("Imprimiendo el tamaño en memoria del proceso %d", pid);
}

void userInterfaceHandler(void* socketFD) {
	char* command = malloc(20);
	while (!end) {
		printf("\n\nIngrese un comando: \n");
		scanf("%s", command);
		//string_trim(&command);
		//int c = strcmp(command, "retardo​");
		if (!strcmp(command, "retardo")) { // !int es lo mismo que int!=0
			command[0] = '\0'; //lo vacio
			scanf("%s", command);
			if (strtol(command, NULL, 10) != 0) {
				RETARDO_MEMORIA = strtol(command, NULL, 10);
				printf("el retardo de la memoria ahora es %u", RETARDO_MEMORIA);
			} else printf("Numero invalido");

		} else if (!strcmp(command, "dump")) {
			command[0] = '\0'; //lo vacio
			scanf("%s", command);

			if (!strcmp(command, "cache")) {
				//TODO mostrar cache

			} else if (!strcmp(command, "struct")){
				dumpMemoryStruct();

			} else if (!strcmp(command, "all")){
				dumpMemoryContent();

			} else if (!strcmp(command, "pid")) {
				command[0] = '\0'; //lo vacio
				scanf("%s", command);

				if (strtol(command, NULL, 10) != 0 && strtol(command, NULL, 10) <= (2^32))
					//que sea un numero y que no se pase del max del uint32
					dumpMemoryContentOfPID(strtol(command, NULL, 10));

				else printf("Numero invalido");

			} else
				printf("No se conoce el comando %s\n", command);

		} else if (!strcmp(command, "disconnect")) {
			end = 0;

		} else if (!strcmp(command, "flush")) {
			command[0] = '\0'; //lo vacio
			scanf("%s", command);

			if (!strcmp(command, "cache")) {
			//TODO limpiar cache

			} else printf("No se conoce el comando %s\n", command);

		} else if (!strcmp(command, "size")) {
			command[0] = '\0'; //lo vacio
			scanf("%s", command);

			if (!strcmp(command, "memory")) {
				printMemorySize();

			} else if (!strcmp(command, "pid")) {
				command[0] = '\0'; //lo vacio
				scanf("%s", command);

				if (strtol(command, NULL, 10) != 0 && strtol(command, NULL, 10) <= (2^32)) {
					//que sea un numero y que no se pase del max del uint32
					printProcessSize(strtol(command, NULL, 10));
				} else printf("Numero invalido");

			} else
				printf("No se conoce el comando %s\n", command);

		} else
			printf("No se conoce el comando %s\n", command);
	}
	free(command);
}

int RecibirPaqueteMemoria (int socketFD, char receptor[11], Paquete* paquete) {
	paquete->Payload = malloc(1);
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0) { //si no hubo error
		if (paquete->header.tipoMensaje == ESHANDSHAKE) { //vemos si es un handshake y le respondemos con el tam de pag
			if( !strcmp(paquete->header.emisor, KERNEL) || !strcmp(paquete->header.emisor, CPU)){
				printf("Se establecio conexion con %s\n", paquete->header.emisor);
				Paquete paquete;
				paquete.header.tipoMensaje = ESHANDSHAKE;
				paquete.header.tamPayload = sizeof(uint32_t);
				strcpy(paquete.header.emisor, MEMORIA);
				paquete.Payload=&MARCO_SIZE;
				EnviarPaquete(socketFD, &paquete);
			}
		} else { //recibimos un payload y lo procesamos (por ej, puede mostrarlo)
			paquete->Payload = realloc(paquete->Payload,
					paquete->header.tamPayload);
			resul = RecibirDatos(paquete->Payload, socketFD,
					paquete->header.tamPayload);
		}
	}

	return resul;
}

void* accionHilo(void* socket){
	int socketFD = *(int*)socket;
	Paquete paquete;
	while (RecibirPaqueteMemoria(socketFD, MEMORIA, &paquete) > 0) {
		switch (paquete.header.tipoMensaje){
		case ESDATOS:
			//printf("\nTexto recibido: %s\n", (char*)paquete->Payload);
			switch ((*(uint32_t*)paquete.Payload)){
			(uint32_t*)paquete.Payload++; //Queda un vector de 4 (0..3) posiciones por el ++
			case INIC_PROG:
				IniciarPrograma(DATOS[0],DATOS[1],socketFD);
			break;
			case SOL_BYTES:
				SolicitarBytes(DATOS[0],DATOS[1],DATOS[2],DATOS[3],socketFD);
			break;
			case ALM_BYTES:
				AlmacenarBytes(paquete);
			break;
			case ASIG_PAG:
				AsignarPaginas(DATOS[0],DATOS[1],socketFD);
			break;
			case LIBE_PAG:
				LiberarPaginas(DATOS[0],DATOS[1],socketFD);
			break;
			case FIN_PROG:
				FinalizarPrograma(DATOS[0]);
			break;
			}
		break;
		case ESARCHIVO:
		break;

		case ESINT:
			if(strcmp(paquete.header.emisor,KERNEL)==0){
				EnviarDatos(socketFD,MEMORIA,&MARCO_SIZE,sizeof(MARCO_SIZE));
			}
		break;
		}
	}
	close(socketFD);
	free(paquete.Payload);
	return NULL;
}
/*//no es necesaria por ahora
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

*/
int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();

	tamanioTotalBytesMemoria = (MARCOS * MARCO_SIZE) + (sizeof(RegistroTablaPaginacion) * MARCOS);
	bloquePpal = malloc(tamanioTotalBytesMemoria); //Reservo toda mi memoria
	listaHilos = list_create();
	TablaPaginacion = list_create();
	procesosActivos = list_create();
	end = 0;
	//tabla_Adm tablaAdm[MARCOS]; //no, mejor accedamos casteando y recorriendo el bloquePpal
	/*//MEMORIA CACHE, NO BORRAR
	 tabla_Cache tablaCache[ENTRADAS_CACHE]; //crear y allocar cache
	 int i;
	 for (i = 0; i < MARCOS; ++i)
	 tablaCache[i].contenido = malloc(MARCO_SIZE);
	 */
	pthread_t hiloConsola;
	pthread_create(&hiloConsola, NULL, (void*) userInterfaceHandler, NULL);

	int socketFD = StartServidor(IP, PUERTO);
	struct sockaddr_in their_addr; // información sobre la dirección del cliente
	int new_fd;
	socklen_t sin_size;

	while(!end) { // Loop Principal
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(socketFD, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
			perror("accept");
			continue;
		}
		printf("\nNueva conexion de %s en " "socket %d\n", inet_ntoa(their_addr.sin_addr), new_fd);
		//pthread_t hilo = agregarAListaHiloSiNoEsta(listaHilos, socketFD);
		pthread_t threadNuevo;
		pthread_create(&threadNuevo, NULL, accionHilo, &new_fd);
		structHilo* itemNuevo = malloc(sizeof(structHilo));
		itemNuevo->hilo = threadNuevo;
		itemNuevo->socket = new_fd;
		list_add(listaHilos, itemNuevo);
	}
	printf("fin");
	close(socketFD);
	//libera los items de lista de hilos , destruye la lista y espera a que termine cada hilo.
	list_destroy_and_destroy_elements(listaHilos, LAMBDA(void _(void* elem) { pthread_join(((structHilo*)elem)->hilo, NULL); }));

	pthread_join(hiloConsola, NULL);

	//liberar listas

	list_destroy_and_destroy_elements(procesosActivos, free); //recibe cada elemento y lo libera.
	list_destroy_and_destroy_elements(TablaPaginacion, free);
	/*//MEMORIA CACHE, NO BORRAR
	 for (i = 0; i < MARCOS; ++i)  //liberar cache.
	 free(tablaCache[i].contenido);
	 */
	free(bloquePpal);
	exit(3); //TODO: ¿Esto va? Se supone que termina todos los hilos del proceso.
	return 0;
}
