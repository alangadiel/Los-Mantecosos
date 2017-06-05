#include "SocketsL.h"

int PUERTO;
char* PUNTO_MONTAJE;
char* IP;
char* METADATAPATHFOLDER;
int TAMANIO_BLOQUES;
int CANTIDAD_BLOQUES;
char* MAGIC_NUMBER;


void obtenerValoresArchivoConfiguracion() {
	int contadorDeVariables = 0;
	int c;
	FILE *file;
	file = fopen("ArchivoConfiguracion.txt", "r");
	if (file) {
		while ((c = getc(file)) != EOF)
			if (c == '=')
			{
				if (contadorDeVariables == 2)
				{
				char buffer[10000];
				IP = fgets(buffer, sizeof buffer, file);
				strtok(IP, "\n");
				contadorDeVariables++;
				}
				if (contadorDeVariables == 1)
				{
					char buffer[10000];
					PUNTO_MONTAJE = fgets(buffer, sizeof buffer, file);
					strtok(PUNTO_MONTAJE, "\n");
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


void validarArchivo(char* path, int socketFD) {
	/*Cuando el Proceso Kernel reciba la operación de abrir un archivo deberá validar que el
	archivo exista.*/
}

void crearArchivo(char* path,int socketFD) {
	/*Cuando el Proceso Kernel reciba la operación de abrir un archivo deberá, en caso que el
archivo no exista y este sea abierto en modo de creación (“c”), llamar a esta operación que
creará el archivo dentro del path solicitado. Por default todo archivo creado se le debe
asignar al menos 1 bloque de datos.
	 *
	 */
}

void borrarArchivo(char* path, int socketFD) {
	/*Borrará el archivo en el path indicado, eliminando su archivo de metadata y marcando los
	bloques como libres dentro del bitmap*/
}

void obtenerDatos(char* Path, char* Offset, char* Size, int socketFD) {
	/*Ante un pedido de datos el File System devolverá del path enviado por parámetro, la
cantidad de bytes definidos por el Size a partir del offset solicitado. Requiere que el archivo
se encuentre abierto en modo lectura (“r”).
En caso de que se soliciten datos o se intenten guardar datos en un archivo inexistente el File System
deberá retornar un error de Archivo no encontrado.

	 *
	 */
}

void guardarDatos(char* Path, char* Offset, char* Size, char* Buffer, int socketFD){
	/*Ante un pedido de escritura el File System almacenará en el path enviado por parámetro, los
bytes del buffer, definidos por el valor del Size y a partir del offset solicitado. Requiere que el
archivo se encuentre abierto en modo escritura (“w”).
En caso de que se soliciten datos o se intenten guardar datos en un archivo inexistente el File System
deberá retornar un error de Archivo no encontrado.

	 *
	 */
}

int RecibirPaqueteFileSystem (int socketFD, char receptor[11], Paquete* paquete) {
	paquete->Payload = malloc(1);
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0) { //si no hubo error
		if (paquete->header.tipoMensaje == ESHANDSHAKE) {
			if (paquete->header.emisor == KERNEL){
				printf("Se establecio conexion con %s\n", paquete->header.emisor);
				Paquete paquete;
				paquete.header.tipoMensaje = ESHANDSHAKE;
				paquete.header.tamPayload = sizeof(uint32_t);
				strcpy(paquete.header.emisor, FS);
				paquete.Payload=1;
				EnviarPaquete(socketFD, &paquete);
			}
			else {
				Paquete paquete;
				paquete.header.tipoMensaje = ESERROR;
				paquete.header.tamPayload = sizeof(uint32_t);
				strcpy(paquete.header.emisor, FS);
				paquete.Payload=0;
				EnviarPaquete(socketFD, &paquete);
			}
		} else { //recibimos un payload y lo procesamos (por ej, puede mostrarlo)
			if (paquete->header.emisor == KERNEL) {
				paquete->Payload = realloc(paquete->Payload,
						paquete->header.tamPayload);
				resul = RecibirDatos(paquete->Payload, socketFD,
						paquete->header.tamPayload);
			}
		}
	}
	return resul;
}

void accion(Paquete* paquete, int socketFD){
	switch ((*(uint32_t*)paquete->Payload)){
		(uint32_t*)paquete->Payload++;
		case VALIDAR_ARCHIVO:
			validarArchivo((uint32_t*)paquete->Payload[0], socketFD);
		break;
		case CREAR_ARCHIVO:
			crearArchivo((uint32_t*)paquete->Payload[0],socketFD);
		break;
		case BORRAR_ARCHIVO:
			borrarArchivo((uint32_t*)paquete->Payload[0],socketFD);
		break;
		case OBTENER_DATOS:
			obtenerDatos((uint32_t*)paquete->Payload[0],(uint32_t*)paquete->Payload[1],(uint32_t*)paquete->Payload[2], socketFD);
		break;
		case GUARDAR_DATOS:
			guardarDatos((uint32_t*)paquete->Payload[0],(uint32_t*)paquete->Payload[1],(uint32_t*)paquete->Payload[2], (uint32_t*)paquete->Payload[3], socketFD);
		break;
	}
}

void archivoMetadata() {
	char* METADATAPATHFILE;
	string_append(METADATAPATHFOLDER, PUNTO_MONTAJE);
	string_append(METADATAPATHFOLDER, "/Metadata/");
	METADATAPATHFILE = string_duplicate(METADATAPATHFOLDER);
	string_append(METADATAPATHFILE, "Metadata.bin");
	if( access( METADATAPATHFILE , F_OK ) != -1 ) {
	    // file exists
		int contadorDeVariables = 0;
			int c;
			FILE *file;
			file = fopen(METADATA, "r");
			if (file) {
				while ((c = getc(file)) != EOF)
					if (c == '=')
					{
						if (contadorDeVariables == 2)
						{
							char buffer[10000];
							MAGIC_NUMBER = fgets(buffer, sizeof buffer, file);
							strtok(IP, "\n");
							contadorDeVariables++;
						}
						if (contadorDeVariables == 1)
						{
							fscanf(file, "%i", &CANTIDAD_BLOQUES);
							contadorDeVariables++;
						}
						if (contadorDeVariables == 0) {
							fscanf(file, "%i", &TAMANIO_BLOQUES);
							contadorDeVariables++;
						}
					}
				fclose(file);
			}
	} else {

		if (stat(METADATAPATHFOLDER, &st) == -1) {
		    mkdir(METADATAPATHFOLDER, 0700);
		}

	    FILE* metadata = fopen(METADATAPATHFILE, "a");
	    fputs("TAMANIO_BLOQUES=", metadata);
	    printf("Ingrese tamanio de bloques: \n");
	    char* strTamanioBloques;
	    scanf("%s", strTamanioBloques);
	    TAMANIO_BLOQUES = atoi(strTamanioBloques);

	    fputs("CANTIDAD_BLOQUES", metadata);
	    printf("Ingrese cantidad de bloques: \n");
	    char* strCantidadBloques;
	    scanf("%s", strCantidadBloques);
	    fputs(strCantidadBloques, metadata);
	    CANTIDAD_BLOQUES = atoi(strCantidadBloques);

	    fputs("MAGIC_NUMBER=", metadata);
	    printf("Ingrese magic number: \n");
	    scanf("%s", MAGIC_NUMBER);
	    fputs(MAGIC_NUMBER, metadata);
	}
}

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();

	archivoMetadata();
	Servidor(IP, PUERTO, MEMORIA, accion, RecibirPaqueteFileSystem);
	return 0;
}
