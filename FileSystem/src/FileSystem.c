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
	t_list* Bloques;
}__attribute__((packed)) ValoresArchivo;

char* obtenerPathABloque(int num) {
	char* pathAEscribir = string_new();
	string_append(&pathAEscribir, BLOQUESPATH);
	string_append(&pathAEscribir, string_itoa(num));
	string_append(&pathAEscribir, ".bin");
	return pathAEscribir;
}

int calcularCantidadDeBloques(int tamanio) {
	int cantidad = ceil(tamanio/TAMANIO_BLOQUES);
	if (cantidad == 0) return 1;
	return cantidad;
}

void obtenerValoresDeArchivo(char* pathArchivo, ValoresArchivo* valores) {
	t_config* arch = config_create(pathArchivo);
	char** Bloques = config_get_array_value(arch, "BLOQUES");
	valores->Tamanio = config_get_int_value(arch, "TAMANIO");
	int i = 0;
	for (i = 0; i<calcularCantidadDeBloques(valores->Tamanio); i++) {
		list_add(valores->Bloques, Bloques[i]);
	}
}

void obtenerValoresArchivoConfiguracion() {
	t_config* arch = config_create("ArchivoConfiguracion.txt");
	PUNTO_MONTAJE = config_get_string_value(arch, "PUNTO_MONTAJE");
	PUERTO = config_get_int_value(arch, "PUERTO");
	config_destroy(arch);
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
		FILE* file = fopen((char*)obtenerPathABloque(i), "a");
		fclose(file);
		cambiarValorBitmap(bloques[i], 1);
	}
}

void eliminarBloques(int* bloques, int cantidadBloques) {
	int i;
	for (i = 0; i < cantidadBloques; i++) {
		cambiarValorBitmap(bloques[i], 0);
		remove((char*)obtenerPathABloque(bloques[i]));
	}
}

void modificarValoresDeArchivo(int tamanio, int* bloques, char* path) {
	ValoresArchivo valores;
	obtenerValoresDeArchivo(path, &valores);

	FILE* newFile = fopen(path, "w+");
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

int reservarBloques(int cantidadDeBloquesNuevos, char* path, int size, ValoresArchivo* valoresViejos) {
	t_list* bloquesNuevos = list_create();
	int i;
	for (i = 0; i < cantidadDeBloquesNuevos; i++) {
		int posicion = encontrarPrimerBloqueLibre();
		if (posicion < 0) {
			return 0;
		}
		bloquesNuevos[i] = posicion;
	}
	int cantidadDeBloquesViejos;
	int j;
	t_list* bloquesTotales = list_create();
	if (valoresViejos != NULL) {
		cantidadDeBloquesViejos = calcularCantidadDeBloques(valoresViejos->Tamanio);
		bloquesTotales = malloc(sizeof(int) * (cantidadDeBloquesViejos + cantidadDeBloquesNuevos));
		for (j = 0; j < cantidadDeBloquesViejos; j++) {
			list_add(bloquesTotales, list_get(valoresViejos->Bloques,j));
		}
	}
	else {
		bloquesTotales = malloc(sizeof(int) * (cantidadDeBloquesNuevos));
		cantidadDeBloquesViejos = 0;
	}
	for (j = cantidadDeBloquesViejos; j < cantidadDeBloquesNuevos; j++) {
		bloquesTotales[j] = bloquesNuevos[j];
	}
	i++;
	j++;
	bloquesNuevos[i] = -1;
	bloquesTotales[j] = -1;
	crearBloques(bloquesNuevos, cantidadDeBloquesNuevos);
	modificarValoresDeArchivo(size, bloquesTotales, path);
	free(bloquesNuevos);
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
		int sePudoReservar = reservarBloques(1, nuevoPath, 0, NULL);
		if (socketFD != 0) {
			if (sePudoReservar) {

				EnviarDatos(socketFD,FS,(void*)1,sizeof(uint32_t));
			}
			else {
				EnviarDatos(socketFD,FS,0,sizeof(uint32_t));
			}
		}
	}
}

void borrarArchivo(char* path, int socketFD) {
	char* pathArchivo = string_new();
	pathArchivo = string_duplicate(ARCHIVOSPATH);
	string_append(&pathArchivo, path);
	if (validarArchivo(pathArchivo, 0)) {
		ValoresArchivo valores;
		obtenerValoresDeArchivo(pathArchivo, &valores);
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
	char *buffer = NULL;
	size_t size = 0;
	FILE *fp = fopen(fileToScan, "r");
	fseek(fp, 0, SEEK_END); /* Go to end of file */
	size = ftell(fp); /* How many bytes did we pass ? */
	rewind(fp);
	buffer = malloc((size + 1) * sizeof(*buffer)); /* size + 1 byte for the \0 */
	fread(buffer, size, 1, fp); /* Read 1 chunk of size bytes from fp into buffer */
	buffer[size] = '\0';
	return buffer;
}

char* obtenerTodosLosDatosDeBloques(ValoresArchivo* valores) {
	char* datos = string_new();
	int i;
	for (i = 0; i < calcularCantidadDeBloques(valores->Tamanio); i++) {
		string_append(&datos, leerTodoElArchivo(obtenerPathABloque(i)));
	}
	return datos;
}

void obtenerDatos(char* path, uint32_t offset, uint32_t size, int socketFD) {
	char* pathAObtener = string_duplicate(ARCHIVOSPATH);
	string_append(&pathAObtener, path);
	if (validarArchivo(pathAObtener, 0)) {
		ValoresArchivo valores;
		obtenerValoresDeArchivo(pathAObtener, &valores);
		char* datos = obtenerTodosLosDatosDeBloques(&valores);
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

char* obtenerTextoParaBloque(int numeroDeBloque, int cantidadDeBloques, char* datosActuales, char* datosAAgregar) {
	int j = TAMANIO_BLOQUES*(numeroDeBloque+1);
	if (numeroDeBloque == cantidadDeBloques) {
		return string_substring(datosActuales, j - TAMANIO_BLOQUES, string_length(datosAAgregar));
	}
	else {
		return string_substring(datosActuales, j - TAMANIO_BLOQUES, TAMANIO_BLOQUES);
	}
}

int obtenerTamanioNuevoDeArchivo(int offset, ValoresArchivo* valores, int size) {
	if (valores->Tamanio > offset) {
		return valores->Tamanio + size;
	}
	else {
		return offset + size;
	}
}

void guardarDatos(char* path, uint32_t offset, uint32_t size, char* buffer, int socketFD){
	char* pathAEscribir = string_new();
	string_append(&pathAEscribir, ARCHIVOSPATH);
	string_append(&pathAEscribir, path);
	if (validarArchivo(pathAEscribir, 0)) {
		ValoresArchivo valores;
		obtenerValoresDeArchivo(pathAEscribir, &valores);
		int nuevoTamanioDeArchivoQueSeNecesita = obtenerTamanioNuevoDeArchivo(offset, &valores, size);
		int cantidadDeBloquesNecesitados = calcularCantidadDeBloques(nuevoTamanioDeArchivoQueSeNecesita) - calcularCantidadDeBloques(valores.Tamanio);
		reservarBloques(cantidadDeBloquesNecesitados, pathAEscribir, nuevoTamanioDeArchivoQueSeNecesita, &valores);
		ValoresArchivo nuevosValores;
		obtenerValoresDeArchivo(pathAEscribir, &nuevosValores);
		char* datosActuales = obtenerTodosLosDatosDeBloques(&nuevosValores);
		int i;
		int j = 0;
		for (i = offset; i < nuevoTamanioDeArchivoQueSeNecesita; i++) {
			datosActuales[i] = buffer[j];
			j++;
		}
		int cantidadDeBloques = calcularCantidadDeBloques(nuevosValores.Tamanio);
		for (i = 0; i < cantidadDeBloques; i++) {
			FILE* file = fopen(obtenerPathABloque(i), "w+");
			fputs(obtenerTextoParaBloque(i, cantidadDeBloques, datosActuales, buffer), file);
			fclose(file);
		}
		if (socketFD != 0) {
			EnviarDatos(socketFD,FS,(void*)1,sizeof(uint32_t));
		}
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
	Servidor(IP, PUERTO, MEMORIA, accion, RecibirPaqueteFileSystem);
	return 0;
}
