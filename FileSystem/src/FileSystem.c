#include "SocketsL.h"
//correr esto en una consola antes de ejecutar sudo chmod -R 755 /mnt

int PUERTO;
char* PUNTO_MONTAJE;
char* IP = "127.0.0.1";
char* METADATAPATH;
char* METADATAFILE;
char* BITMAPFILE;
char* ARCHIVOSPATH;
char* BLOQUESPATH;
int TAMANIO_BLOQUES;
int CANTIDAD_BLOQUES;
char* MAGIC_NUMBER;
int *bitmapArray;

struct stat st = {0};

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

int length(void* x) {
	return sizeof(x) / sizeof((x)[0]);
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
	char* fileToValidate = string_duplicate(PUNTO_MONTAJE);
	strcat(fileToValidate, path);
	//if (S_ISDIR(fileToValidate)) {
		if( access( fileToValidate, F_OK ) != -1 ) {
			if (socketFD != 0) {
				EnviarDatos(socketFD,FS,(void*)1,sizeof(uint32_t));
			}
			return 1;
		} else {
			if (socketFD != 0) {
				EnviarDatos(socketFD,FS,0,sizeof(uint32_t));
			}
		}
	//}
	return 0;
}

void cambiarValorBitmap(int posicion, int valor) {
	bitmapArray[posicion] = valor;
	//cambiarlo en el archivo
}

int encontrarPrimerBloqueLibre() {
	int i = 0;
	int seEncontro;
	while (bitmapArray[i] < CANTIDAD_BLOQUES && seEncontro == 0) {
		if (bitmapArray[i] == 0) {
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

void crearBloques(int* bloques) {
	int i;
	for (i = 0; i < length(bloques); i++) {
		char* newFile = string_duplicate(BLOQUESPATH);
		strcat(newFile, string_itoa(i));
		strcat(newFile, ".bin");
		FILE* file = fopen(newFile, "a");
		fclose(file);
		cambiarValorBitmap(bloques[i], 1);
	}
}

void eliminarBloques(int bloques[]) {
	int i;
	for (i = 0; i < length(bloques); i++) {
		cambiarValorBitmap(bloques[i], 0);
		char* pathToRemove = string_duplicate(BLOQUESPATH);
		strcat(pathToRemove, string_itoa(bloques[i]));
		strcat(pathToRemove, ".bin");
		remove(pathToRemove);
	}
}

void modificarValoresDeArchivo(int tamanio, int* bloques, char* path) {
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

int reservarBloques(int cantidadDeBloques, char* path, int size) {
	int* bloques = malloc(sizeof(int) * cantidadDeBloques);
	int i;
	for (i = 0; i < cantidadDeBloques; i++) {
		int posicion = encontrarPrimerBloqueLibre();
		if (posicion < 0) {
			return 0;
		}
		bloques[i] = posicion;
	}
	i++;
	bloques[i] = -1;
	crearBloques(bloques);
	modificarValoresDeArchivo(size, bloques, path);
	free(bloques);
	return 1;
}

void crearArchivo(char* path,int socketFD) {
	char* pathForValidation = string_duplicate(ARCHIVOSPATH);
	strcat(pathForValidation, path);
	if (!validarArchivo(pathForValidation, 0)) {
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
		int sePudoReservar = reservarBloques(1, nuevoPath, 0);
		if (sePudoReservar) {
			EnviarDatos(socketFD,FS,(void*)1,sizeof(uint32_t));
		}
		else {
			EnviarDatos(socketFD,FS,0,sizeof(uint32_t));
		}
	}
}

ValoresArchivo obtenerValoresDeArchivo(char* pathArchivo) {
	ValoresArchivo valores;
	return valores;
}

void borrarArchivo(char* path, int socketFD) {
	char* pathArchivo = string_duplicate(ARCHIVOSPATH);
	strcat(pathArchivo, path);
	if (validarArchivo(pathArchivo, 0)) {
		ValoresArchivo valores = obtenerValoresDeArchivo(pathArchivo);
		eliminarBloques(valores.Bloques);

		int removeFile = remove(pathArchivo);
		if (removeFile) {
			EnviarDatos(socketFD,FS,(void*)1,sizeof(uint32_t));
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

char* leerTodoElArchivo(char* fileToScan) {
	FILE* file = fopen(fileToScan, "a");
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* fcontent = malloc(size);
	fread(fcontent, 1, size, file);
	fclose(file);
	return fcontent;
}

char* obtenerTodosLosDatosDeBloques(int bloques[]) {
	char* datos = string_new();
	int i;
	for (i = 0; i < length(bloques); i++) {
		char* pathAlBloque = string_duplicate(BLOQUESPATH);
		strcat(pathAlBloque, string_itoa(i));
		strcat(pathAlBloque, ".bin");
		strcat(datos, leerTodoElArchivo(pathAlBloque));
	}
	return datos;
}

void obtenerDatos(char* path, uint32_t offset, uint32_t size, int socketFD) {
	char* pathAObtener = string_duplicate(ARCHIVOSPATH);
	strcat(pathAObtener, path);
	if (!validarArchivo(pathAObtener, 0)) {
		ValoresArchivo valores = obtenerValoresDeArchivo(pathAObtener);
		char* datos = obtenerTodosLosDatosDeBloques(valores.Bloques);
		char* datosAEnviar = string_substring(datos, offset, size);
		EnviarDatos(socketFD,FS,datosAEnviar,sizeof(char) * string_length(datosAEnviar));
	}
	else {
		EnviarDatos(socketFD,FS,0,sizeof(uint32_t));
	}
}


int obtenerTamanioDeArchivo(char* path) {
	FILE* file = fopen(path, "r");
	fseek(file, 0L, SEEK_END);
	int tamanio = ftell(file);
	fclose(file);
	return tamanio;
}

void guardarDatos(char* path, uint32_t offset, uint32_t size, char* buffer, int socketFD){
	char* pathAEscribir = string_duplicate(ARCHIVOSPATH);
	strcat(pathAEscribir, path);
	if (!validarArchivo(pathAEscribir, 0)) {
		ValoresArchivo valores = obtenerValoresDeArchivo(pathAEscribir);
		int nuevoTamanioDeArchivoQueSeNecesita;
		if (valores.Tamanio > offset) {
			nuevoTamanioDeArchivoQueSeNecesita = valores.Tamanio + size;
		}
		else {
			nuevoTamanioDeArchivoQueSeNecesita = offset + size;
		}
		int cantidadDeBloquesTotales = (int) ceil(nuevoTamanioDeArchivoQueSeNecesita/TAMANIO_BLOQUES);
		int cantidadDeBloquesNecesitados = cantidadDeBloquesTotales - length(valores.Bloques);
		reservarBloques(cantidadDeBloquesNecesitados, pathAEscribir, nuevoTamanioDeArchivoQueSeNecesita);
		ValoresArchivo nuevosValores = obtenerValoresDeArchivo(pathAEscribir);
		char** datosArray = string_get_string_as_array(obtenerTodosLosDatosDeBloques(nuevosValores.Bloques));
		char** datosAGuardar = string_get_string_as_array(buffer);
		int i;
		int j = 0;
		for (i = offset; i < nuevoTamanioDeArchivoQueSeNecesita; i++) {
			datosArray[i] = datosAGuardar[j];
			j++;
		}
		FILE* file = fopen(pathAEscribir, "w");
		for (i = 0; i < length(valores.Bloques); i++) {
			for (j = 0; j < TAMANIO_BLOQUES; j++) {
				j += TAMANIO_BLOQUES*i;
				if (string_length(buffer) >= j) {
					char* pathAEscribir = string_duplicate(BLOQUESPATH);
					strcat(pathAEscribir, string_itoa(i));
					strcat(pathAEscribir, ".bin");
					fputc((int)datosArray[j], file);
				}
			}
		}
		fclose(file);
		EnviarDatos(socketFD,FS,(void*)1,sizeof(uint32_t));
	}
	else {
		EnviarDatos(socketFD,FS,0,sizeof(uint32_t));
	}
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
				paquete.Payload = (void*)1;
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
			guardarDatos(((char**)paquete->Payload)[0], ((uint32_t*)paquete->Payload)[1], ((uint32_t*)paquete->Payload)[2], ((char**)paquete->Payload)[3], socketFD);
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
	    char* strTamanioBloques = string_new();
	    scanf("%s", strTamanioBloques);
	    TAMANIO_BLOQUES = atoi(strTamanioBloques);

	    fputs("CANTIDAD_BLOQUES", metadata);
	    printf("Ingrese cantidad de bloques: \n");
	    char* strCantidadBloques = string_new();
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



void archivoBitmap() {
	int i;
	bitmapArray = malloc (CANTIDAD_BLOQUES * sizeof (bitmapArray[0]));
	if( access( BITMAPFILE , F_OK ) != -1 ) {
		// file exists
		FILE* bitmap = fopen(BITMAPFILE, "a");
		char* stringBitmap = leerTodoElArchivo(BITMAPFILE);
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
	if (stat(PUNTO_MONTAJE, &st) == -1) {
		mkdir(PUNTO_MONTAJE, 0777);
	}
	METADATAPATH = string_duplicate(PUNTO_MONTAJE);
	string_append(&METADATAPATH, "Metadata/");
	if (stat(METADATAPATH, &st) == -1) {
		mkdir(METADATAPATH, 0777);
	}

	ARCHIVOSPATH = string_duplicate(PUNTO_MONTAJE);
	string_append(&ARCHIVOSPATH, "Archivos/");
	if (stat(ARCHIVOSPATH, &st) == -1) {
		mkdir(ARCHIVOSPATH, 0777);
	}

	BLOQUESPATH = string_duplicate(PUNTO_MONTAJE);
	strcat(BLOQUESPATH, "Bloques/");
	if (stat(BLOQUESPATH, &st) == -1) {
		mkdir(BLOQUESPATH, 0777);
	}

	METADATAFILE = string_duplicate(METADATAPATH);
	strcat(METADATAFILE, "Metadata.bin");

	BITMAPFILE = string_duplicate(METADATAPATH);;
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

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	crearEstructurasDeCarpetas();
	archivoMetadata();
	archivoBitmap();
	/*char* scan = string_new();
	char* scan1 = string_new();
	char* scan2 = string_new();
	char* scan3 = string_new();
	char* scan4 = string_new();
	char* scan5 = string_new();
	while (true) {
		printf("validarArchivo 2, crearArchivo 2, borrarArchivo 2, obtenerDatos 4, guardarDatos 5\n");
		scanf("%s\n", scan);
		scanf("%s\n", scan1);
		scanf("%s\n", scan2);
		scanf("%s\n", scan3);
		scanf("%s\n", scan4);
		scanf("%s\n", scan5);
		if (strcmp(scan, "validarArchivo") == 0) {
			validarArchivo(scan1, atoi(scan2));
		}
		if (strcmp(scan, "crearArchivo") == 0) {
			crearArchivo(scan1, atoi(scan2));
				}
		if (strcmp(scan, "borrarArchivo") == 0) {
			borrarArchivo(scan1, atoi(scan2));
		}
		if (strcmp(scan, "obtenerDatos") == 0) {
			obtenerDatos(scan1, atoi(scan2), atoi(scan3), atoi(scan4));
				}
		if (strcmp(scan, "guardarDatos") == 0) {
			guardarDatos(scan1, atoi(scan2), atoi(scan3), scan4, atoi(scan5));
		}
	}
	Servidor(IP, PUERTO, MEMORIA, accion, RecibirPaqueteFileSystem);*/
	return 0;
}
