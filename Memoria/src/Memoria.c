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
t_list* listaHilos;
char end;


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


int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	tamEstructurasAdm = sizeof(RegistroTablaPaginacion) * MARCOS;
	tamanioTotalBytesMemoria = (MARCOS * MARCO_SIZE) + tamEstructurasAdm;
	BloquePrincipal = malloc(tamanioTotalBytesMemoria); //Reservo toda mi memoria
	ContenidoMemoria = BloquePrincipal + tamEstructurasAdm; //guardo el puntero donde empieza el contenido
	listaHilos = list_create();
	cantPagAsignadas = 0;
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
	uint32_t j = Hash(3, 5);

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
	//list_destroy_and_destroy_elements(lista, free); //recibe cada elemento y lo libera.
	/*//MEMORIA CACHE, NO BORRAR
	 for (i = 0; i < MARCOS; ++i)  //liberar cache.
	 free(tablaCache[i].contenido);
	 */
	free(BloquePrincipal);
	exit(3); //TODO: ¿Esto va? Se supone que termina todos los hilos del proceso.
	return 0;
}
