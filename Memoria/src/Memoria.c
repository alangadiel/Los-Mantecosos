#include "SocketsL.h"

#define IP_MEMORIA 127.0.0.1
#define DATOS ((uint32_t*)paquete->Payload)

int PUERTO;
int MARCOS;
int MARCO_SIZE;
int ENTRADAS_CACHE;
int CACHE_X_PROC;
char* REEMPLAZO_CACHE;
int RETARDO_MEMORIA;
char* IP;

void* bloquePpal;

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

void IniciarPrograma(uint32_t pid, uint32_t cantPag) {

}
void SolicitarBytes(uint32_t pid, uint32_t numPag, uint32_t offset, uint32_t tam, int socketFD) {

}
void AlmacenarBytes(Paquete* paquete) {
//Buscar pagina
//esperar tiempo definido por arch de config
memcpy(bloquePpal+DATOS[3],(void*)DATOS[5],DATOS[4]);
//actualizar cache
}
void AsignarPaginas(uint32_t pid, uint32_t cantPag, int socketFD) {

}
void FinalizarPrograma(uint32_t pid) {

}

void accion(Paquete* paquete, int socketFD){
	switch (paquete->header.tipoMensaje){
	case ESSTRING:
		printf("\nTexto recibido: %s", (char*)paquete->Payload);
		switch ((*(uint32_t*)paquete->Payload)){
		case INIC_PROG:
			IniciarPrograma(DATOS[1],DATOS[2]);
		break;
		case SOL_BYTES:
			SolicitarBytes(DATOS[1],DATOS[2],DATOS[3],DATOS[4],socketFD);
		break;
		case ALM_BYTES:
			AlmacenarBytes(paquete);
		break;
		case ASIG_PAG:
			AsignarPaginas(DATOS[1],DATOS[2],socketFD);
		break;
		case FIN_PROG:
			FinalizarPrograma(DATOS[1]);
		break;
		}
	break;
	case ESARCHIVO:


	break;
	}
}

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();

	bloquePpal = malloc((MARCOS * MARCO_SIZE) + (sizeof(tabla_Adm) * MARCOS)); //Reservo toda mi memoria
	//tabla_Adm tablaAdm[MARCOS]; //no, mejor accedamos casteando y recorriendo el bloquePpal

	tabla_Cache tablaCache[ENTRADAS_CACHE]; //crear y allocar cache
	int i;
	for (i = 0; i < MARCOS; ++i)
		tablaCache[i].contenido = malloc(MARCO_SIZE);

	Servidor(IP, PUERTO, MEMORIA, accion);

	for (i = 0; i < MARCOS; ++i)  //liberar cache.
			free(tablaCache[i].contenido);
	free(bloquePpal);

	return 0;
}
