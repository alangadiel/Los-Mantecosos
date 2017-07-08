#include "SocketsL.h"
//correr esto en una consola antes de ejecutar sudo chmod -R 755 /mnt

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


struct stat st = {0};

typedef struct {
	int Tamanio;
	t_list* Bloques;
}__attribute__((packed)) ValoresArchivo;

bool existeArchivo(char* path) {
	return access( path , F_OK ) != -1;
}

bool existeDirectorio(char* path) {
	return stat(path, &st) != -1;
}

void crearDirectorio(char* path) {
	mkdir(path, 0777);
}

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

ValoresArchivo* obtenerValoresDeArchivo(char* pathArchivo) {
	ValoresArchivo* valores = malloc(sizeof(ValoresArchivo));
	t_config* arch = config_create(pathArchivo);
	char** Bloques = config_get_array_value(arch, "BLOQUES");
	valores->Tamanio = config_get_int_value(arch, "TAMANIO");
	int cantidadBloques = ceil(valores->Tamanio/TAMANIO_BLOQUES);
	if (cantidadBloques == 0) cantidadBloques = 1;
	int i = 0;
	for (i = 0; i<cantidadBloques; i++) {
		list_add(valores->Bloques, Bloques[i]);
	}
	return valores;
}

void obtenerValoresArchivoConfiguracion() {
	t_config* arch = config_create("ArchivoConfiguracion.txt");
	PUNTO_MONTAJE = string_duplicate(config_get_string_value(arch, "PUNTO_MONTAJE"));
	PUERTO = config_get_int_value(arch, "PUERTO");
	IP = string_duplicate(config_get_string_value(arch, "IP"));
	config_destroy(arch);
}

int validarArchivo(char* path, int socketFD) {
	char* pathForValidation = string_duplicate(ARCHIVOSPATH);
	string_append(&pathForValidation, path);
	if(existeArchivo(pathForValidation)) {
		EnviarDatos(socketFD, FS, 1, sizeof(uint32_t));
		return 1;
	}
	else {
		EnviarDatos(socketFD, FS, 0, sizeof(uint32_t));
		return 0;
	}
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
	free(bitmapToWrite);
}

int encontrarPrimerBloqueLibre() {
	int i = 0;
	while (bitmapArray[i] < CANTIDAD_BLOQUES) {
		if (bitmapArray[i] == 0) {
			return i;
		}
	}
	return -1;
}

void crearBloqueVacio(int idBloque) {
	char* pathABloque = obtenerPathABloque(idBloque);
	FILE* file = fopen(pathABloque, "a");
	fclose(file);
}

void crearBloques(t_list* bloques) {
	int i;
	for (i = 0; i < list_size(bloques); i++) {
		int idBloque = (int) list_get(bloques, i);
		crearBloqueVacio(idBloque);
		cambiarValorBitmap(idBloque, 1);
	}
}

void eliminarBloques(t_list* bloques) {
	int i;
	for (i = 0; i < list_size(bloques); i++) {
		int idBloque = (int) list_get(bloques, i);
		cambiarValorBitmap(idBloque, 0);
		char* pathDeBloque = obtenerPathABloque(idBloque);
		remove(pathDeBloque);
	}
}

void modificarValoresDeArchivo(ValoresArchivo* valoresNuevos, char* path) {
	FILE* newFile = fopen(path, "w+");
	char* strTamanio = string_new();
	string_append(&strTamanio, "TAMANIO=");
	string_append(&strTamanio, string_itoa(valoresNuevos->Tamanio));
	string_append(&strTamanio, "\n");
	fputs(strTamanio, newFile);
	free(strTamanio);

	char* strBloque = string_new();
	string_append(&strBloque, "BLOQUES=[");
	string_append(&strBloque, string_itoa((int)list_get(valoresNuevos->Bloques, 0)));
	int i;
	for (i = 1; i < list_size(valoresNuevos->Bloques); i++) {
		string_append(&strBloque, ",");
		string_append(&strBloque, string_itoa((int)list_get(valoresNuevos->Bloques, i)));
	}

	string_append(&strBloque, "]");
	fputs(strBloque, newFile);
	free(strBloque);
	fclose(newFile);
}

ValoresArchivo* reservarBloques(char* path, int tamanioNecesitado, ValoresArchivo* valoresViejos) {
	int cantidadDeBloquesAReservar = calcularCantidadDeBloques(tamanioNecesitado) - list_size(valoresViejos->Bloques);
	t_list* bloquesViejos = valoresViejos->Bloques;
	t_list* bloquesNuevos = list_create();
	t_list* bloquesTotales = list_create();
	int i;
	for (i = 0; i < cantidadDeBloquesAReservar; i++) {
		int posicion = encontrarPrimerBloqueLibre();
		if (posicion < 0) {
			return NULL;
		}
		list_add(bloquesNuevos, &posicion);
	}
	list_add_all(bloquesTotales, bloquesViejos);
	list_add_all(bloquesTotales, bloquesNuevos);
	crearBloques(bloquesNuevos);
	ValoresArchivo* valoresNuevos = malloc(sizeof(ValoresArchivo));
	valoresNuevos->Bloques = bloquesTotales;
	valoresNuevos->Tamanio = tamanioNecesitado;
	modificarValoresDeArchivo(valoresNuevos, path);
	return valoresNuevos;
}

void crearArchivo(char* path,int socketFD) {
	char* pathForValidation = string_duplicate(ARCHIVOSPATH);
	string_append(&pathForValidation, path);
	if (!validarArchivo(pathForValidation, 0)) {
		char** pathArray = string_split(path, "/");
		int i = 0;
		char* nuevoPath = string_duplicate(ARCHIVOSPATH);
		while (!string_ends_with(pathArray[i], ".bin")) {
			string_append(&nuevoPath, pathArray[i]);
			string_append(&nuevoPath, "/");
			if (existeDirectorio(nuevoPath)) {
				crearDirectorio(nuevoPath);
			}
			i++;
		}
		string_append(&nuevoPath, pathArray[i]);
		ValoresArchivo* nuevosValores = reservarBloques(nuevoPath, 0, NULL);
		if (nuevosValores != NULL) {
			EnviarDatos(socketFD, FS, 1, sizeof(uint32_t));
		}
		else {
			EnviarDatos(socketFD, FS, 0, sizeof(uint32_t));
		}
		free(nuevosValores);
		free(nuevoPath);
		free(pathArray);
		free(pathForValidation);
	}
}

void borrarArchivo(char* path, int socketFD) {
	char* pathArchivo = string_new();
	pathArchivo = string_duplicate(ARCHIVOSPATH);
	string_append(&pathArchivo, path);
	if (validarArchivo(pathArchivo, 0)) {
		ValoresArchivo* valores = obtenerValoresDeArchivo(pathArchivo);
		eliminarBloques(valores->Bloques);
		int removeFile = remove(pathArchivo);
		if (removeFile >= 0 && socketFD != 0) {
			EnviarDatos(socketFD, FS, 1, sizeof(uint32_t));
		}
		else {
			EnviarDatos(socketFD, FS, 0, sizeof(uint32_t));
		}
		free(valores);
	}
	else
	{
		EnviarDatos(socketFD, FS, 0, sizeof(uint32_t));
	}
	free(pathArchivo);
}

char* leerTodoElArchivo(char* fileToScan) {
	char *buffer = NULL;
	size_t size = 0;
	FILE *fp = fopen(fileToScan, "r");
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);
	buffer = malloc((size + 1) * sizeof(*buffer));
	fread(buffer, size, 1, fp);
	buffer[size] = '\0';
	return buffer;
}

char* obtenerTodosLosDatosDeBloques(ValoresArchivo* valores) {
	char* datos = string_new();
	int i;
	for (i = 0; i < list_size(valores->Bloques); i++) {
		string_append(&datos, leerTodoElArchivo(obtenerPathABloque(i)));
	}
	return datos;
}

void obtenerDatos(char* path, uint32_t offset, uint32_t size, int socketFD) {
	char* pathAObtener = string_duplicate(ARCHIVOSPATH);
	string_append(&pathAObtener, path);
	if (validarArchivo(pathAObtener, 0)) {
		ValoresArchivo* valores = obtenerValoresDeArchivo(pathAObtener);
		char* datosAEnviar = string_substring(obtenerTodosLosDatosDeBloques(valores), offset, size);
		EnviarDatos(socketFD,FS,datosAEnviar, sizeof(char) * string_length(datosAEnviar));
		free(datosAEnviar);
		free(valores);
	}
	else {
		EnviarDatosTipo(socketFD, FS, NULL, 0, ESERROR);
	}
	free(pathAObtener);
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
	if (validarArchivo(pathAEscribir, 0)) {
		ValoresArchivo* valores = obtenerValoresDeArchivo(pathAEscribir);
		int nuevoTamanioDeArchivo;
		if (valores->Tamanio > offset) {
			nuevoTamanioDeArchivo = valores->Tamanio + size;
		}
		else {
			nuevoTamanioDeArchivo = offset + size;
		}

		ValoresArchivo* nuevosValores = reservarBloques(pathAEscribir, nuevoTamanioDeArchivo, valores);
		char* datosActuales = obtenerTodosLosDatosDeBloques(nuevosValores);
		int i;
		for (i = offset; i < nuevoTamanioDeArchivo; i++) {
			datosActuales[i] = buffer[i];
			i++;
		}

		int j = 0;
		int posicion;
		FILE* file = fopen(obtenerPathABloque(i), "w+");
		//lleno todos los bloques hasta el anteultimo
		while (j < list_size(nuevosValores->Bloques) - 1) {
			posicion = TAMANIO_BLOQUES*(j+1);
			fputs(string_substring(datosActuales, posicion - TAMANIO_BLOQUES, TAMANIO_BLOQUES), file);
			j++;
		}
		//el ultimo bloque lo lleno hasta donde llegue el buffer
		fputs(string_substring(datosActuales, j - TAMANIO_BLOQUES, string_length(buffer)), file);
		fclose(file);

		if (socketFD != 0) {
			EnviarDatos(socketFD, FS, 1, sizeof(uint32_t));
		}
		free(nuevosValores);
		free(valores);
		free(datosActuales);
	}
	else {
		EnviarDatos(socketFD, FS, 0, sizeof(uint32_t));
	}
	free(pathAEscribir);
}


void accion(Paquete* paquete, int socketFD){
	switch ((*(uint32_t*)paquete->Payload)){
		(uint32_t*)paquete->Payload++;
		uint32_t* datos = paquete->Payload;
		case VALIDAR_ARCHIVO:
			validarArchivo(paquete->Payload, socketFD);
		break;
		case CREAR_ARCHIVO:
			crearArchivo(paquete->Payload, socketFD);
		break;
		case BORRAR_ARCHIVO:
			borrarArchivo(paquete->Payload, socketFD);
		break;
		case OBTENER_DATOS:
			obtenerDatos(&(datos[2]), datos[0], datos[1], socketFD);
		break;
		case GUARDAR_DATOS:
			guardarDatos(&(datos[2]), datos[0], datos[1],&(datos[3]), socketFD);
		break;
	}
}

/*void accion(void* socket){
	int socketFD = *(int*)socket;
	Paquete paquete;
	while (RecibirPaqueteMemoria(socketFD, MEMORIA, &paquete) > 0) {
		if (paquete.header.tipoMensaje == ESDATOS){
			switch ((*(uint32_t*)paquete.Payload)){
			case INIC_PROG:
				IniciarPrograma(DATOS[1],DATOS[2],socketFD);
			break;
			case SOL_BYTES:
				SolicitarBytes(DATOS[1],DATOS[2],DATOS[3],DATOS[4],socketFD);
			break;
			case ALM_BYTES:
				AlmacenarBytes(paquete,socketFD);
			break;
			case ASIG_PAG:
				AsignarPaginas(DATOS[1],DATOS[2],socketFD);
			break;
			case LIBE_PAG:
				LiberarPaginas(DATOS[1],DATOS[2],socketFD);
			break;
			case FIN_PROG:
				FinalizarPrograma(DATOS[1],socketFD);
			break;
			}
		}
		free(paquete.Payload);
	}
	close(socketFD);
}*/

bool esCorrectoArchivoMetadata() {
	t_config* conf = config_create(METADATAFILE);
	bool esCorrectoCantidadDeKeys = config_keys_amount(conf) == 3;
	bool existeMagicNumber = config_has_property(conf, "MAGIC_NUMBER");
	bool existeCantidadDeBloques = config_has_property(conf, "CANTIDAD_BLOQUES");
	bool existeTamanioDeBloques = config_has_property(conf, "TAMANIO_BLOQUES");
	config_destroy(conf);
	return esCorrectoCantidadDeKeys && existeCantidadDeBloques && existeMagicNumber && existeTamanioDeBloques;
}

void archivoMetadata() {
	if(existeArchivo(METADATAFILE) && esCorrectoArchivoMetadata()) {
		t_config* conf = config_create(METADATAFILE);
		MAGIC_NUMBER = string_duplicate(config_get_string_value(conf, "MAGIC_NUMBER"));
		CANTIDAD_BLOQUES = config_get_int_value(conf, "CANTIDAD_BLOQUES");
		TAMANIO_BLOQUES = config_get_int_value(conf, "TAMANIO_BLOQUES");
		config_destroy(conf);
	} else {
	    FILE* metadata = fopen(METADATAFILE, "a");
	    fputs("TAMANIO_BLOQUES=", metadata);
	    printf("Ingrese tamanio de bloques: \n");
	    char* strTamanioBloques = string_new();
	    scanf("%s", strTamanioBloques);
	    string_append(&strTamanioBloques, "\n");
	    fputs(strTamanioBloques, metadata);
	    TAMANIO_BLOQUES = atoi(strTamanioBloques);
	    free(strTamanioBloques);

	    fputs("CANTIDAD_BLOQUES=", metadata);
	    printf("Ingrese cantidad de bloques: \n");
	    char* strCantidadBloques = string_new();
	    scanf("%s", strCantidadBloques);
	    string_append(&strCantidadBloques, "\n");
	    fputs(strCantidadBloques, metadata);
	    CANTIDAD_BLOQUES = atoi(strCantidadBloques);
	    free(strCantidadBloques);

	    fputs("MAGIC_NUMBER=", metadata);
	    printf("Ingrese magic number: \n");
	    char* strMAGIC_NUMBER = string_new();
	    scanf("%s", strMAGIC_NUMBER);
	    MAGIC_NUMBER = string_duplicate(strMAGIC_NUMBER);
	    free(strMAGIC_NUMBER);

	    fputs(MAGIC_NUMBER, metadata);
	    fclose(metadata);
	}
}

void archivoBitmap() {
	int i;
	bitmapArray = malloc (CANTIDAD_BLOQUES * sizeof(int)); //TODO revisar
	if(existeArchivo(BITMAPFILE)) {
		FILE* bitmap = fopen(BITMAPFILE, "a");
		char* stringBitmap = leerTodoElArchivo(BITMAPFILE);
		int i;
		for (i = 0; i < CANTIDAD_BLOQUES; i++) {
			bitmapArray[i] = (int) strtol(&stringBitmap[i], (char **)NULL, 10);
		}
		fclose(bitmap);
		free(stringBitmap);
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
	if (!existeDirectorio(PUNTO_MONTAJE)) {
		crearDirectorio(PUNTO_MONTAJE);
	}

	METADATAPATH = string_duplicate(PUNTO_MONTAJE);
	string_append(&METADATAPATH, "Metadata/");
	if (!existeDirectorio(METADATAPATH)) {
		crearDirectorio(METADATAPATH);
	}

	ARCHIVOSPATH = string_duplicate(PUNTO_MONTAJE);
	string_append(&ARCHIVOSPATH, "Archivos/");
	if (!existeDirectorio(ARCHIVOSPATH)) {
		crearDirectorio(ARCHIVOSPATH);
	}

	BLOQUESPATH = string_duplicate(PUNTO_MONTAJE);
	string_append(&BLOQUESPATH, "Bloques/");
	if (!existeDirectorio(BLOQUESPATH)) {
		crearDirectorio(BLOQUESPATH);
	}

	METADATAFILE = string_duplicate(METADATAPATH);
	string_append(&METADATAFILE, "Metadata.bin");

	BITMAPFILE = string_duplicate(METADATAPATH);
	string_append(&BITMAPFILE, "Bitmap.bin");
}

void LiberarVariables() {
	free(PUNTO_MONTAJE);
	free(IP);
	free(METADATAPATH);
	free(METADATAFILE);
	free(BITMAPFILE);
	free(ARCHIVOSPATH);
	free(BLOQUESPATH);
	free(bitmapArray);
	free(MAGIC_NUMBER);
}

int RecibirPaqueteFileSystem (int socketFD, char receptor[11], Paquete* paquete) {
	paquete->Payload = malloc(1);
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0) { //si no hubo error
		if (paquete->header.tipoMensaje == ESHANDSHAKE) {
			if (strcmp(paquete->header.emisor,KERNEL) == 0) {
				printf("Se establecio conexion con %s\n", paquete->header.emisor);
				EnviarHandshake(socketFD, FS);
			}
			else
				EnviarDatosTipo(socketFD, FS, NULL, 0, ESERROR);
		} else {
			if (strcmp(paquete->header.emisor, KERNEL) == 0) {
				accion(paquete, socketFD);
			}
		}
	}
	return resul;
}

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	crearEstructurasDeCarpetas();
	archivoMetadata();
	archivoBitmap();
	Servidor(IP, PUERTO, FS, accion, RecibirPaqueteFileSystem);
	LiberarVariables();
	return 0;
}
