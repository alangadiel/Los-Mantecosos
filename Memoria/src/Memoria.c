#include "SocketsL.h"

#define IP_MEMORIA 127.0.0.1
#define DATOS ((uint32_t*)paquete->Payload)
//#define ADM_MAR_SIZE 10 //el tamaño del marco de

int PUERTO;
int MARCOS;
int MARCO_SIZE;
int ENTRADAS_CACHE;
int CACHE_X_PROC;
char* REEMPLAZO_CACHE;
int RETARDO_MEMORIA;
char* IP;

typedef struct {
	uint32_t Frame;
	uint32_t PID;
	uint32_t Pag;
} tabla_Adm ;

typedef struct {
	uint32_t PID;
	uint32_t Pag;
	void* contenido;
} tabla_Cache ;

void obtenerValoresArchivoConfiguracion() {
	int contadorDeVariables = 0;
	int c;
	FILE *file;
	file = fopen("ArchivoConfiguracion.txt", "r");
	if (file) {
		while ((c = getc(file)) != EOF)
			if (c == '=')
			{
				if (contadorDeVariables == 7) {
					char buffer[10000];
					IP = fgets(buffer, sizeof buffer, file);
					strtok(IP, "\n");
				}
				if (contadorDeVariables == 6) {
					fscanf(file, "%i", &RETARDO_MEMORIA);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 5)
				{
					char buffer[10000];
					REEMPLAZO_CACHE = fgets(buffer, sizeof buffer, file);
					strtok(REEMPLAZO_CACHE, "\n");
					contadorDeVariables++;
				}
				if (contadorDeVariables == 4) {
					fscanf(file, "%i", &CACHE_X_PROC);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 3) {
					fscanf(file, "%i", &ENTRADAS_CACHE);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 2) {
					fscanf(file, "%i", &MARCO_SIZE);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 1) {
					fscanf(file, "%i", &MARCOS);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 0) {
					fscanf(file, "%i", &PUERTO);
					contadorDeVariables++;
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

void IniciarPrograma(uint32_t pid, uint32_t cantPag) {

}
void SolicitarBytes(uint32_t pid, uint32_t numPag, uint32_t offset, uint32_t tam, int socketFD) {

}
void AlmacenarBytes(Paquete* paquete) {

}
void AsignarPaginas(uint32_t pid, uint32_t cantPag, int socketFD) {

}
void FinalizarPrograma(uint32_t pid) {

}

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();

	void* bloquePpal = malloc((MARCOS * MARCO_SIZE) + (sizeof(tabla_Adm) * MARCOS)); //Reservo toda mi memoria
	//tabla_Adm tablaAdm[MARCOS]; //no, mejor accedamos casteando y recorriendo el bloquePpal

	tabla_Cache tablaCache[ENTRADAS_CACHE]; //crear y allocar cache
	int i;
	for (i = 0; i < MARCOS; ++i)
		tablaCache[i].contenido = malloc(MARCO_SIZE);

//start servidor
	int SocketEscucha = StartServidor(IP, PUERTO);

	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);
	FD_SET(SocketEscucha, &master); // añadir listener al conjunto maestro
	int fdmax = SocketEscucha; // seguir la pista del descriptor de fichero mayor, por ahora es éste
	struct sockaddr_in remoteaddr; // dirección del cliente
	// bucle principal
	for(;;)	{
		read_fds = master; // cópialo
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)		{
			perror("select");
			exit(1);
		}
		// explorar conexiones existentes en busca de datos que leer
		int i;
		for(i = 0; i <= fdmax; i++)	{
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == SocketEscucha) { // gestionar nuevas conexiones
					int addrlen = sizeof(remoteaddr);
					int nuevoSocket = accept(SocketEscucha, (struct sockaddr*)&remoteaddr,&addrlen);
					if (nuevoSocket == -1)
						perror("accept");
					else {
						FD_SET(nuevoSocket, &master); // añadir al conjunto maestro
						if (nuevoSocket > fdmax) fdmax = nuevoSocket; // actualizar el máximo
						printf("\nNueva conexion de %s en " "socket %d\n", inet_ntoa(remoteaddr.sin_addr),nuevoSocket);
					}
				}
				else  {
					Paquete* paquete = malloc(sizeof(Paquete));
					int result = RecibirPaquete(i, MEMORIA, paquete);
					if(	result>0){
						switch (paquete->header.tipoMensaje){
						case ESSTRING:
							printf("\nTexto recibido: %s", (char*)paquete->Payload);
							switch ((*(uint32_t*)paquete->Payload)){
							case INIC_PROG:
								IniciarPrograma(DATOS[1],DATOS[2]);
							break;
							case SOL_BYTES:
								SolicitarBytes(DATOS[1],DATOS[2],DATOS[3],DATOS[4],i);
							break;
							case ALM_BYTES:
								AlmacenarBytes(paquete);
							break;
							case ASIG_PAG:
								AsignarPaginas(DATOS[1],DATOS[2],i);
							break;
							case FIN_PROG:
								FinalizarPrograma(DATOS[1]);
							break;
							}
						break;
						case ESARCHIVO:


						break;
						}





						//Y finalmente, no puede faltar hacer el free
						free (paquete->Payload); //No olvidar hacer DOS free
						free(paquete);
				 }
				else
					FD_CLR(i, &master); // eliminar del conjunto maestro si falla
				}
			}
		}
	}



//fin servidor
	for (i = 0; i < MARCOS; ++i)  //liberar cache.
			free(tablaCache[i].contenido);
	free(bloquePpal);

	return 0;
}
