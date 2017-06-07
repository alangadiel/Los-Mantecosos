#include "SocketsL.h"
//correr esto en una consola antes de ejecutar sudo chmod -R 755 /mnt

#define length(x)  (sizeof(x) / sizeof((x)[0]))

int PUERTO;
char* PUNTO_MONTAJE;
char* IP;
char* METADATAPATH;
char* METADATAFILE;
char* BITMAPFILE;
char* ARCHIVOSPATH;
char* BLOQUESPATH;
int TAMANIO_BLOQUES;
int CANTIDAD_BLOQUES;
char* MAGIC_NUMBER;
int *bitmapArray;
int YA_ESTA_CREADO;

typedef struct {
	int Tamanio;
	int Bloques[];
}__attribute__((packed)) ValoresArchivo;

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


int validarArchivo(char* path, int socketFD) {
	char fileToValidate = string_duplicate(PUNTO_MONTAJE);
	strcat(fileToValidate, path);
	if( access( fileToValidate, F_OK ) != -1 ) {
	    if (socketFD != 0) {
	    	EnviarDatos(socketFD,FS,1,sizeof(uint32_t));
	    }
	    return 1;
	} else {
		if (socketFD != 0) {
			EnviarDatos(socketFD,FS,0,sizeof(uint32_t));
		}
		return 0;
	}
}

void cambiarValorBitmap(int posicion, int valor) {
	bitmapArray[posicion] = valor;
	//cambiarlo en el archivo
}

int encontrarPrimerBloqueLibreYOcuparlo() {
	int i = 0;
	int seEncontro;
	while (bitmapArray[i] < CANTIDAD_BLOQUES && seEncontro == 0) {
		if (bitmapArray[i] == 0) {
			cambiarValorBitmap(i, 1);
			seEncontro = 1;
		}
	}
	if (seEncontro == 1) {
		return i;
	}
	else {
		return -1;
	}
}

int* obtenerBloques(int bytes) {
	int cantidadDeBloques = (int) ceil(bytes/TAMANIO_BLOQUES);
	int* bloques;
	int i;
	if (cantidadDeBloques == 0) cantidadDeBloques = 1;
	for (i = 0; i<cantidadDeBloques;i++) {
		int posicion = encontrarPrimerBloqueLibreYOcuparlo();
		if (posicion < 0) {
			return -1;
		}
		char* newFile = string_duplicate(ARCHIVOSPATH);
		strcat(newFile, string_itoa(posicion));
		strcat(newFile, ".bin");
		FILE* file = fopen(newFile, "a");
		bloques[i] = posicion;
	}
	i++;
	bloques[i] = -1;
	return bloques;
}

void modificarValoresDeArchivo(int tamanio, int bloques[], char* path) {
	FILE* newFile = fopen(path, "a");
	char* strTamanio = "TAMANIO=";
	strcat(strTamanio, string_itoa(tamanio));
	strcat(strTamanio, "\n");
	fputs(strTamanio, newFile);
	char* strBloque = "BLOQUES[";
	int i = 0;
	while (bloques[i] >= 0) {
		strcat(strBloque, string_itoa(bloques[i]));
	}
	strcat(strBloque, "]");
	fputs(strBloque, newFile);
	fclose(newFile);
}

void crearArchivo(char* path,int socketFD) {
	char* pathForValidation = string_duplicate(ARCHIVOSPATH);
	strcat(pathForValidation, path);
	if (S_ISDIR(pathForValidation) && !validarArchivo(pathForValidation, 0)) {
		char** pathArray = string_split(path, "/");
		int i = 0;
		char* nuevoPath = string_duplicate(ARCHIVOSPATH);
		while (!string_ends_with(pathArray[i], ".bin")) {
			strcat(nuevoPath, pathArray[i]);
			if (stat(nuevoPath, &st) == -1) {
				mkdir(nuevoPath, 0777);
			}
			i++;
		}
		strcat(nuevoPath, pathArray[i]);
		int* bloques = obtenerBloques(0);
		if (bloques >= 0) {
			modificarValoresDeArchivo(0, bloques, nuevoPath);
			EnviarDatos(socketFD,FS,1,sizeof(uint32_t));
		}
		else {
			EnviarDatos(socketFD,FS,0,sizeof(uint32_t));
		}
	}
}

void eliminarBloques(int bloques[]) {
	int i;
	for (i = 0; i < length(bloques); i++) {
		char* pathToRemove = string_duplicate(BLOQUESPATH);
		strcat(pathToRemove, string_itoa(bloques[i]));
		strcat(pathToRemove, ".bin");
		remove(pathToRemove);
	}
}

void liberarBloquesBitmap(int bloques[]) {
	int i;
	for (i = 0; i < length(bloques); i++) {
		cambiarValorBitmap(bloques[i], 0);
	}
}

ValoresArchivo obtenerValoresDeArchivo(pathArchivo) {
	ValoresArchivo valores;
	return valores;
}

void borrarArchivo(char* path, int socketFD) {
	char* pathArchivo = string_duplicate(ARCHIVOSPATH);
	strcat(pathArchivo, path);
	if (validarArchivo(pathArchivo, 0)) {
		ValoresArchivo valores = obtenerValoresDeArchivo(pathArchivo);
		eliminarBloques(valores.Bloques);
		liberarBloquesBitmap(valores.Bloques);
		int removeFile = remove(pathArchivo);
		if (removeFile) {
			EnviarDatos(socketFD,FS,1,sizeof(uint32_t));
		}
		else {
			EnviarDatos(socketFD,FS,0,sizeof(uint32_t));
		}
	}
	else
	{
		EnviarDatos(socketFD,FS,0,sizeof(uint32_t));
	}
}

void obtenerDatos(char* Path, uint32_t Offset, uint32_t Size, int socketFD) {
	/*Ante un pedido de datos el File System devolverá del path enviado por parámetro, la
cantidad de bytes definidos por el Size a partir del offset solicitado. Requiere que el archivo
se encuentre abierto en modo lectura (“r”).
En caso de que se soliciten datos o se intenten guardar datos en un archivo inexistente el File System
deberá retornar un error de Archivo no encontrado.

	 *
	 */
}

void guardarDatos(char* Path, uint32_t Offset, uint32_t Size, uint32_t Buffer, int socketFD){
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
			if (strcmp(paquete->header.emisor,KERNEL) == 0) {
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
		} else {
			if (strcmp(paquete->header.emisor, KERNEL) == 0) {
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
			validarArchivo(((char**)paquete->Payload)[0], socketFD);
		break;
		case CREAR_ARCHIVO:
			crearArchivo(((char**)paquete->Payload)[0], socketFD);
		break;
		case BORRAR_ARCHIVO:
			borrarArchivo(((char**)paquete->Payload)[0], socketFD);
		break;
		case OBTENER_DATOS:
			obtenerDatos(((char**)paquete->Payload)[0], ((uint32_t*)paquete->Payload)[1], ((uint32_t*)paquete->Payload)[2], socketFD);
		break;
		case GUARDAR_DATOS:
			guardarDatos(((char**)paquete->Payload)[0], ((uint32_t*)paquete->Payload)[1], ((uint32_t*)paquete->Payload)[2], ((uint32_t*)paquete->Payload)[3], socketFD);
		break;
	}
}

void archivoMetadata() {
	if( access( METADATAFILE , F_OK ) != -1 ) {
	    // file exists
		int contadorDeVariables = 0;
			int c;
			FILE *file;
			file = fopen(METADATAFILE, "r");
			if (file) {
				while ((c = getc(file)) != EOF)
					if (c == '=')
					{
						if (contadorDeVariables == 1)
						{
							fscanf(file, "%i", &YA_ESTA_CREADO);
							contadorDeVariables++;
						}
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

	    FILE* metadata = fopen(METADATAFILE, "a");
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
	    fclose(metadata);
	}
}

char* leerTodoElArchivo(FILE* file) {
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* fcontent = malloc(size);
	fread(fcontent, 1, size, file);
	return fcontent;
}

void archivoBitmap() {
	int i;
	bitmapArray = malloc (CANTIDAD_BLOQUES * sizeof (bitmapArray[0]));
	if( access( BITMAPFILE , F_OK ) != -1 ) {
		// file exists
		FILE* bitmap = fopen(BITMAPFILE, "a");
		char* stringBitmap = leerTodoElArchivo(bitmap);
		char** fileArrayBitmap = string_get_string_as_array(stringBitmap);
		int i;
		for (i = 0; i < CANTIDAD_BLOQUES; i++) {
			bitmapArray[i] = (int) strtol(fileArrayBitmap[i], (char **)NULL, 10);
		}
		fclose(bitmap);
	}
	else {
		FILE* bitmap = fopen(BITMAPFILE, "a");
		for(i = 0; i < CANTIDAD_BLOQUES; i++) {
			fputc(0, bitmap);
			bitmapArray[i] = 0;
		}
		fclose(bitmap);
	}
}

void crearEstructurasDeCarpetas() {
	strcat(METADATAPATH, PUNTO_MONTAJE);
	strcat(METADATAPATH, "Metadata/");
	if (stat(METADATAPATH, &st) == -1) {
		mkdir(METADATAPATH, 0777);
	}

	strcat(ARCHIVOSPATH, PUNTO_MONTAJE);
	strcat(ARCHIVOSPATH, "Archivos/");
	if (stat(ARCHIVOSPATH, &st) == -1) {
		mkdir(ARCHIVOSPATH, 0777);
	}

	strcat(BLOQUESPATH, PUNTO_MONTAJE);
	strcat(BLOQUESPATH, "Bloques/");
	if (stat(BLOQUESPATH, &st) == -1) {
		mkdir(BLOQUESPATH, 0777);
	}

	strcat(METADATAFILE, METADATAPATH);
	strcat(METADATAFILE, "Metadata.bin");

	strcat(BITMAPFILE, METADATAPATH);
	strcat(BITMAPFILE, "Bitmap.bin");
}

int contarArchivosEnDirectorio() {
	int file_count = 0;
	DIR * dirp;
	struct dirent * entry;

	dirp = opendir("path"); /* There should be error handling after this */
	while ((entry = readdir(dirp)) != NULL) {
	    if (entry->d_type == DT_REG) { /* If the entry is a regular file */
	         file_count++;
	    }
	}
	closedir(dirp);
	return file_count;
}

void crearArchivosVacios() {
	int i;
	int cantidadDeArchivos = contarArchivosEnDirectorio(ARCHIVOSPATH);
	int faltante = CANTIDAD_BLOQUES - cantidadDeArchivos;
	if (faltante < 0) {
		for(i=cantidadDeArchivos-1;i>=CANTIDAD_BLOQUES;i--){
			char* archivoAEliminar = string_duplicate(ARCHIVOSPATH);
			strcat(archivoAEliminar, string_itoa(i));
			strcat(archivoAEliminar, ".bin");
			remove(archivoAEliminar);
		}
	}
	else if (faltante > 0) {
		for(i=cantidadDeArchivos;i<CANTIDAD_BLOQUES;i++){
			char* nombre = string_duplicate(ARCHIVOSPATH);
			strcat(nombre, string_itoa(i));
			strcat(nombre, ".bin");
			FILE* archivo = fopen(nombre, "a");
			fclose(archivo);
		}
	}
}

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	crearEstructurasDeCarpetas();
	archivoMetadata();
	archivoBitmap();
	crearArchivosVacios();
	Servidor(IP, PUERTO, MEMORIA, accion, RecibirPaqueteFileSystem);
	return 0;
}
