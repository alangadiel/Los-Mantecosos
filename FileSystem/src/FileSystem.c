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
	if( access( path, F_OK ) != -1 ) {
		if (socketFD != 0) {
			EnviarDatos(socketFD,FS,(void*)1,sizeof(uint32_t));
		}
		return 1;
	} else {
		if (socketFD != 0) {
			EnviarDatos(socketFD,FS,0,sizeof(uint32_t));
		}
	}
	return 0;
}

void cambiarValorBitmap(int posicion, int valor) {
	bitmapArray[posicion] = valor;
	int i;
	FILE* bitmap = fopen(BITMAPFILE, "w+");
	char* bitmapToWrite = string_new();
	for(i = 0; i < CANTIDAD_BLOQUES; i++) {
		string_append(&bitmapToWrite, string_itoa(bitmapArray[i]));
	}
	fputs(bitmapToWrite, bitmap);
	fclose(bitmap);
}

int encontrarPrimerBloqueLibre() {
	int i = 0;
	int seEncontro = 0;
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

void crearBloques(int* bloques, int cantidadBloques) {
	int i;
	for (i = 0; i < cantidadBloques; i++) {
		char* newFile = string_duplicate(BLOQUESPATH);
		string_append(&newFile, string_itoa(i));
		string_append(&newFile, ".bin");
		FILE* file = fopen(newFile, "a");
		fclose(file);
		cambiarValorBitmap(bloques[i], 1);
	}
}

void eliminarBloques(int* bloques, int cantidadBloques) {
	int i;
	for (i = 0; i < cantidadBloques; i++) {
		cambiarValorBitmap(bloques[i], 0);
		char* pathToRemove = string_duplicate(BLOQUESPATH);
		string_append(&pathToRemove, string_itoa(bloques[i]));
		string_append(&pathToRemove, ".bin");
		remove(pathToRemove);
	}
}

void modificarValoresDeArchivo(int tamanio, int* bloques, char* path) {
	FILE* newFile = fopen(path, "a");
	char* strTamanio = string_new();
	string_append(&strTamanio, "TAMANIO=");
	string_append(&strTamanio, string_itoa(tamanio));
	string_append(&strTamanio, "\n");
	fputs(strTamanio, newFile);
	char* strBloque = string_new();
	string_append(&strBloque, "BLOQUES=[");
	int i = 0;
	string_append(&strBloque, string_itoa(bloques[i]));
	i++;
	while (bloques[i] >= 0) {
		string_append(&strBloque, ",");
		string_append(&strBloque, string_itoa(bloques[i]));
	}
	string_append(&strBloque, "]");
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
	crearBloques(bloques, cantidadDeBloques);
	modificarValoresDeArchivo(size, bloques, path);
	free(bloques);
	return 1;
}

void crearArchivo(char* path,int socketFD) {
	char* pathForValidation = string_duplicate(ARCHIVOSPATH);
	string_append(&pathForValidation, path);
	if (!validarArchivo(pathForValidation, 0)) {
		char** pathArray1 = string_split(path, "/");
		int i = 0;
		char* nuevoPath = string_duplicate(ARCHIVOSPATH);
		while (!string_ends_with(pathArray1[i], ".bin")) {
			string_append(&nuevoPath, pathArray1[i]);
			string_append(&nuevoPath, "/");
			if (stat(nuevoPath, &st) == -1) {
				mkdir(nuevoPath, 0777);
			}
			i++;
		}
		string_append(&nuevoPath, pathArray1[i]);
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
	int contadorDeVariables = 0;
		int c;
		FILE *file;
		file = fopen(pathArchivo, "r");
		if (file) {
			while ((c = getc(file)) != EOF)
				if (c == '=')
				{
					if (contadorDeVariables == 1)
					{
						char buffer[10000];
						char *line = fgets(buffer,sizeof buffer,file);
						int length = string_length(line)-2;
						char *texto = string_substring(line,1,length);
						texto  = strtok(texto,",");
						int i = 0;
						while (texto != NULL)
						{
							valores.Bloques[i++] = atoi(texto);
							texto = strtok (NULL, ",");
						}
					}
					if (contadorDeVariables == 0) {
						fscanf(file, "%i", &valores.Tamanio);
						contadorDeVariables++;
					}
				}
			fclose(file);
		}
	return valores;
}

void borrarArchivo(char* path, int socketFD) {
	char* pathArchivo = string_new();
	pathArchivo = string_duplicate(ARCHIVOSPATH);
	string_append(&pathArchivo, path);
	if (validarArchivo(pathArchivo, 0)) {
		ValoresArchivo valores = obtenerValoresDeArchivo(pathArchivo);
		int cantidadBloques = ceil(valores.Tamanio/TAMANIO_BLOQUES);
		if (cantidadBloques == 0) cantidadBloques = 1;
		eliminarBloques(valores.Bloques, cantidadBloques);
		int removeFile = remove(pathArchivo);
		if (removeFile >= 0 && socketFD != 0) {
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
	FILE *fp;
	long lSize;
	char *buffer;

	fp = fopen ( fileToScan , "rb" );
	if( !fp ) perror(fileToScan),exit(1);

	fseek( fp , 0L , SEEK_END);
	lSize = ftell( fp );
	rewind( fp );
	buffer = calloc( 1, lSize+1 );
	if( !buffer ) fclose(fp),fputs("memory alloc fails",stderr),exit(1);
	if( 1!=fread( buffer , lSize, 1 , fp) )
	  fclose(fp),free(buffer),fputs("entire read fails",stderr),exit(1);

	fclose(fp);
	return buffer;
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
	char* pathAEscribir = string_new();
	string_append(&pathAEscribir, ARCHIVOSPATH);
	string_append(&pathAEscribir, path);
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
					string_append(&pathAEscribir, string_itoa(i));
					string_append(&pathAEscribir, ".bin");
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
							strtok(MAGIC_NUMBER, "\n");
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
	    string_append(&strTamanioBloques, "\n");
	    fputs(strTamanioBloques, metadata);
	    TAMANIO_BLOQUES = atoi(strTamanioBloques);

	    fputs("CANTIDAD_BLOQUES=", metadata);
	    printf("Ingrese cantidad de bloques: \n");
	    char* strCantidadBloques = string_new();
	    scanf("%s", strCantidadBloques);
	    string_append(&strCantidadBloques, "\n");
	    fputs(strCantidadBloques, metadata);
	    CANTIDAD_BLOQUES = atoi(strCantidadBloques);

	    fputs("MAGIC_NUMBER=", metadata);
	    printf("Ingrese magic number: \n");
	    char* strMAGIC_NUMBER = string_new();
	    scanf("%s", strMAGIC_NUMBER);
	    MAGIC_NUMBER = string_duplicate(strMAGIC_NUMBER);

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
		int i;
		int lengtharray = string_length(stringBitmap);
		for (i = 0; i < CANTIDAD_BLOQUES; i++) {
			bitmapArray[i] = (int) strtol(&stringBitmap[i], (char **)NULL, 10);
		}
		fclose(bitmap);
	}
	else {
		FILE* bitmap = fopen(BITMAPFILE, "a");
		for(i = 0; i < CANTIDAD_BLOQUES; i++) {
			fputc('0', bitmap);
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
	string_append(&BLOQUESPATH, "Bloques/");
	if (stat(BLOQUESPATH, &st) == -1) {
		mkdir(BLOQUESPATH, 0777);
	}

	METADATAFILE = string_duplicate(METADATAPATH);
	string_append(&METADATAFILE, "Metadata.bin");

	BITMAPFILE = string_duplicate(METADATAPATH);
	string_append(&BITMAPFILE, "Bitmap.bin");
}

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	crearEstructurasDeCarpetas();
	archivoMetadata();
	archivoBitmap();
	crearArchivo("kmisi/hola.bin", 0);
	char* datos = "hola kmisi como estas todo bien? que contas?";
	guardarDatos("kmisi/hola.bin", 0, string_length(datos), datos, 0);
	//Servidor(IP, PUERTO, MEMORIA, accion, RecibirPaqueteFileSystem);
	return 0;
}
