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
	uint32_t Frame;
	uint32_t PID;
	uint32_t Pag;
} RegistroTablaPaginacion ;

typedef struct {
	uint32_t PID;
	uint32_t Pag;
	void* contenido;
} tabla_Cache ;

void* bloquePpal;
uint32_t tamanioTotalBytesMemoria;
uint32_t cantidadPaginas=0;
t_list* TablaPaginacion;
int socketABuscar;

void obtenerValoresArchivoConfiguracion() {
	int contadorDeVariables = 0;
	int c;
	FILE *file;
	file = fopen("ArchivoConfiguracion.txt", "r");
	if (file) {
		while ((c = getc(file)) != EOF)
			if (c == '=')
			{
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
	if(list_size(TablaPaginacion)<1)
	{
		//El checkpoint 2 dice que por ahora hay solo una unica pagina
		RegistroTablaPaginacion nuevoRegistros;
		nuevoRegistros.Frame = 3;
		nuevoRegistros.PID = pid;
		nuevoRegistros.Pag = 2;
		list_add(TablaPaginacion,&nuevoRegistros);
		cantPaginasAsignadas = cantPag;
	}
	EnviarDatos(socketFD,MEMORIA,&cantPaginasAsignadas,sizeof(uint32_t));
}

void SolicitarBytes(uint32_t pid, uint32_t numPag, uint32_t offset, uint32_t tam, int socketFD) {

}
void AlmacenarBytes(Paquete* paquete) {
	//Buscar pagina
	sleep(RETARDO_MEMORIA);//esperar tiempo definido por arch de config
	memcpy(bloquePpal+DATOS[3],(paquete->Payload) + sizeof(uint32_t)*5,DATOS[4]);
	//printf("%s\n", DATOS[4]);
	printf("Datos Almacenados correctamente! \n"); //TODO: imprimir datos
	printf("El contenido de la memoria es: %s",(char*)bloquePpal);

	//actualizar cache
}

void AsignarPaginas(uint32_t pid, uint32_t cantPag, int socketFD) {

}

void FinalizarPrograma(uint32_t pid) {
	//join de hilo correspondiente
}
void userInterfaceHandler(void* socketFD) {
	int end = 1;
	while (end) {
		char orden[100];
		printf("\n\nIngrese una orden: \n");
		scanf("%s", orden);
		char* command = getWord(orden, 0);
		if(strcmp(command,"exit")==0)
				exit(1);
		else if (strcmp(command, "dump") == 0) {
			printf("El contenido de la memoria es: %s",(char*)bloquePpal);
			}
		else if (strcmp(command, "disconnect") == 0) {
			end = 0;
		}  else {
			printf("No se conoce el mensaje %s\n", orden);
		}
	}
}
void* inputConsola (void* p){

	for(;;){
	char orden[100];
	printf("\n\nIngrese una orden: \n");
	scanf("%s", orden);
	//bloquear hasta que reciba algo

	if(strcmp(orden,"exit")==0)
		exit(1);
	else if (strcmp(orden,"dump")==0)
		printf("%s",(char*)bloquePpal);
	}
	return NULL;
}

void* accionHilo(int* socketFD){
	while(true) {
	}
}

pthread_t agregarAListaHiloSiNoEsta(t_list* listaHilos, int socketFD) {
	socketABuscar = socketFD;
	structHilo* structActual = list_find(listaHilos, LAMBDA(bool _(void* shilo) { return ((structHilo*)shilo)->socket != socketABuscar; }));
		if (structActual != 0) { //el hilo ya existe
			return structActual->hilo;
		}
	else { // crear hilo y agregar a lista
		pthread_t threadNuevo;
		pthread_create(&threadNuevo, NULL, accionHilo, &socketFD);
		structHilo itemNuevo;
		itemNuevo.hilo = threadNuevo;
		itemNuevo.socket = socketFD;
		list_add(listaHilos, &itemNuevo);
		return threadNuevo;
	}
}

void accion(Paquete* paquete, int socketFD){

	//pthread_t hilo = agregarAListaHiloSiNoEsta(listaHilos, socketFD);
	switch (paquete->header.tipoMensaje){
	case ESDATOS:
		//printf("\nTexto recibido: %s\n", (char*)paquete->Payload);
		switch ((*(uint32_t*)paquete->Payload)){
		(uint32_t*)paquete->Payload++; //Queda un vector de 4 (0..3) posiciones por el ++
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
		case FIN_PROG:
			FinalizarPrograma(DATOS[0]);
		break;
		}
	break;
	case ESARCHIVO:
	break;

	case ESINT:
		if(strcmp(paquete->header.emisor,KERNEL)==0){
			EnviarDatos(socketFD,MEMORIA,&MARCO_SIZE,sizeof(MARCO_SIZE));
		}
	break;
	}
}

int RecibirPaqueteMemoria (int socketFD, char receptor[11], Paquete* paquete) {
	paquete->Payload = malloc(1);
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0) { //si no hubo error
		if (paquete->header.tipoMensaje == ESHANDSHAKE) { //vemos si es un handshake
			printf("Se establecio conexion con %s\n", paquete->header.emisor);
			if(strcmp(paquete->header.emisor, KERNEL) == 0){
				Paquete paquete;
				paquete.header.tipoMensaje = ESHANDSHAKE;
				paquete.header.tamPayload = sizeof(uint32_t);
				strcpy(paquete.header.emisor, MEMORIA);
				paquete.Payload=&MARCO_SIZE;
				EnviarPaquete(socketFD, &paquete);
			} else
			EnviarHandshake(socketFD, receptor); // paquete->header.emisor
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
	tamanioTotalBytesMemoria = (MARCOS * MARCO_SIZE) + (sizeof(RegistroTablaPaginacion) * MARCOS);
	bloquePpal = malloc(tamanioTotalBytesMemoria); //Reservo toda mi memoria
	TablaPaginacion = list_create();
	//tabla_Adm tablaAdm[MARCOS]; //no, mejor accedamos casteando y recorriendo el bloquePpal
	/*//MEMORIA CACHE, NO BORRAR
	tabla_Cache tablaCache[ENTRADAS_CACHE]; //crear y allocar cache
	int i;
	for (i = 0; i < MARCOS; ++i)
		tablaCache[i].contenido = malloc(MARCO_SIZE);
 	 */
	pthread_t hiloConsola;
	pthread_create(&hiloConsola, NULL, (void*)userInterfaceHandler, NULL);

	Servidor(IP, PUERTO, MEMORIA, accion, RecibirPaqueteMemoria);

	pthread_join(hiloConsola, NULL);
	/*//MEMORIA CACHE, NO BORRAR
	for (i = 0; i < MARCOS; ++i)  //liberar cache.
			free(tablaCache[i].contenido);
	*/
	free(bloquePpal);

	return 0;
}
