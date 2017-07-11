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
	string_append(&pathAEscribir, "/");
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

bool archivoEsValido(char* pathForValidation) {
	return string_ends_with(pathForValidation, ".bin");
}
void armarPath(char** path){
	char* pathForValidation = string_duplicate(ARCHIVOSPATH);
	char* subsrt = string_new();
	int finWhile = 0;
	int hasta = 0;

	string_trim(path);

	int i = 0;

	while(i < string_length(*path) && finWhile != 4)
	{
		hasta++;

		if((*path)[i] == '.')
		{
			finWhile++;
		}

		if((*path)[i-1] == '.')
		{
			if((*path)[i] == 'b')
			{
				finWhile++;
			}
			else
			{
				finWhile = 0;
			}
		}

		if((*path)[i-2] == '.' && (*path)[i-1] == 'b')
		{
			if((*path)[i] == 'i')
			{
				finWhile++;
			}
			else
			{
				finWhile = 0;
			}
		}

		if((*path)[i-3] == '.' && (*path)[i-2] == 'b' && (*path)[i-1] == 'i')
		{
			if((*path)[i] == 'n')
			{
				finWhile++;
			}
			else
			{
				finWhile = 0;
			}
		}

		i++;
	}

	subsrt = string_substring_until(*path, hasta);

	string_append(&pathForValidation,subsrt);
	free(*path);
	*path = string_duplicate(pathForValidation);
	free(pathForValidation);
	free(subsrt);
}

int validarArchivo(char* path, int socketFD) {
	uint32_t r;
	bool esValido = archivoEsValido(path);
	bool existe = existeArchivo(path);
	if(esValido && existe) {
		r=1;
		if(socketFD>0)
			EnviarDatos(socketFD, FS, &r, sizeof(uint32_t));
	}
	else {
		r=0;
		if(socketFD>0)
			EnviarDatos(socketFD, FS, &r, sizeof(uint32_t));
	}
	return r;
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

		i++;
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
		int idBloque = (int)list_get(bloques, i);
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

	//list_add_all(bloquesTotales, bloquesViejos);
	list_add_all(bloquesTotales, bloquesNuevos);
	crearBloques(bloquesNuevos);
	ValoresArchivo* valoresNuevos = malloc(sizeof(ValoresArchivo));
	valoresNuevos->Bloques = bloquesTotales;
	valoresNuevos->Tamanio = tamanioNecesitado;
	modificarValoresDeArchivo(valoresNuevos, path);
	return valoresNuevos;
}

bool validarArchivoSinEnviarAKernel(char* path)
{
	return archivoEsValido(path) && existeArchivo(path);
}

void crearArchivo(char* path,int socketFD) {
	if (!validarArchivoSinEnviarAKernel(path)) {
		char** pathArray = string_split(path, "/");
		int i = 0;
		char* nuevoPath = string_new();

		string_append(&nuevoPath, "/");
		string_append(&nuevoPath, pathArray[0]);

		while (!string_ends_with(pathArray[i], ".bin")) {
			if(i != 0)
			{
				string_append(&nuevoPath, "/");
				string_append(&nuevoPath, pathArray[i]);
			}

			i++;
		}

		if (!existeDirectorio(nuevoPath)) {
			crearDirectorio(nuevoPath);
		}

		string_append(&nuevoPath, "/");
		string_append(&nuevoPath, pathArray[i]);

		//REVISAR ESTO, HICE CUALQUIER COSA PARA PODER SEGUIR EJECUTANDO
		ValoresArchivo* valoresViejos = malloc(sizeof(ValoresArchivo));
		valoresViejos->Bloques = list_create();
		valoresViejos->Tamanio = 0;

		ValoresArchivo* nuevosValores = reservarBloques(nuevoPath, 0, valoresViejos);
		if (nuevosValores != NULL) {
			uint32_t r = 1;
			EnviarDatos(socketFD, FS, &r, sizeof(uint32_t));
		}
		else {
			uint32_t r = 0;
			EnviarDatos(socketFD, FS, &r, sizeof(uint32_t));
		}

		free(nuevosValores);
		free(nuevoPath);
		free(pathArray);
	}
}

void borrarArchivo(char* pathArchivo , int socketFD) {
	uint32_t r = 0;
	if (validarArchivo(pathArchivo, 0)) {
		ValoresArchivo* valores = obtenerValoresDeArchivo(pathArchivo);
		eliminarBloques(valores->Bloques);
		int removeFile = remove(pathArchivo);
		if (removeFile >= 0 && socketFD != 0) {
			r= 1;
			EnviarDatos(socketFD, FS, &r, sizeof(uint32_t));
		}
		else {
			EnviarDatos(socketFD, FS, &r, sizeof(uint32_t));
		}
		free(valores);
	}
	else
	{
		EnviarDatos(socketFD, FS, &r, sizeof(uint32_t));
	}
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

void obtenerDatos(char* pathAObtener, uint32_t offset, uint32_t size, int socketFD) {
	if (validarArchivo(pathAObtener, 0)) {
		ValoresArchivo* valores = obtenerValoresDeArchivo(pathAObtener);
		char* datosAEnviar = string_substring(obtenerTodosLosDatosDeBloques(valores), offset, size);
		EnviarDatos(socketFD,FS,datosAEnviar, sizeof(char) * string_length(datosAEnviar));
		free(datosAEnviar);
		free(valores);
	}
	else {
		uint32_t r = 0;
		EnviarDatosTipo(socketFD, FS, &r, sizeof(uint32_t) , ESERROR);
	}
}

int obtenerTamanioDeArchivo(char* path) {
	FILE* file = fopen(path, "r");
	fseek(file, 0L, SEEK_END);
	int tamanio = ftell(file);
	fclose(file);
	return tamanio;
}

void guardarDatos(char* path, uint32_t offset, uint32_t size, void* buffer, int socketFD){
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
		memcpy(datosActuales, buffer+offset, nuevoTamanioDeArchivo); //puede ser que deba avanzar x2

		int j = 0;
		int posicion;
		char* nombreF = obtenerPathABloque(nuevoTamanioDeArchivo-offset);
		FILE* file = fopen(nombreF, "w+");
		free(nombreF);
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
			uint32_t r = 1;
			EnviarDatos(socketFD, FS, &r, sizeof(uint32_t));
		}
		free(nuevosValores);
		free(valores);
		free(datosActuales);
	}
	else {
		uint32_t r = 0;
		EnviarDatos(socketFD, FS, &r, sizeof(uint32_t));
	}
	free(pathAEscribir);
}


void accion(Paquete* paquete, int socketFD){

	if(paquete->header.tipoMensaje == ESDATOS)
	{
		uint32_t* datos = paquete->Payload;
		char* path = paquete->Payload+sizeof(uint32_t);
		switch (*datos){
			case VALIDAR_ARCHIVO:
				path = string_duplicate(path);
				armarPath(&path);
				validarArchivo(path, socketFD);
			break;
			case CREAR_ARCHIVO:
				path = string_duplicate(path);
				armarPath(&path);
				crearArchivo(path, socketFD);
			break;
			case BORRAR_ARCHIVO:
				path = string_duplicate(path);
				armarPath(&path);
				borrarArchivo(path, socketFD);
			break;
			case OBTENER_DATOS:
				path = paquete->Payload+sizeof(uint32_t)*3;
				path = string_duplicate(path);
				armarPath(&path);
				obtenerDatos(path, datos[1], datos[2], socketFD);
			break;
			case GUARDAR_DATOS:
				path = paquete->Payload+sizeof(uint32_t)*3;
				path = string_duplicate(path);
				armarPath(&path);
				guardarDatos(path, datos[1], datos[2],&(datos[4]), socketFD);
			break;
		}
		free(path);
	}
}

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
			bitmapArray[i] = stringBitmap[i] - 48; //Para pasar de ASCII
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
	string_append(&METADATAPATH, "/Metadata");
	if (!existeDirectorio(METADATAPATH)) {
		crearDirectorio(METADATAPATH);
	}

	ARCHIVOSPATH = string_duplicate(PUNTO_MONTAJE);
	string_append(&ARCHIVOSPATH, "/Archivos");
	if (!existeDirectorio(ARCHIVOSPATH)) {
		crearDirectorio(ARCHIVOSPATH);
	}

	BLOQUESPATH = string_duplicate(PUNTO_MONTAJE);
	string_append(&BLOQUESPATH, "/Bloques");
	if (!existeDirectorio(BLOQUESPATH)) {
		crearDirectorio(BLOQUESPATH);
	}

	METADATAFILE = string_duplicate(METADATAPATH);
	string_append(&METADATAFILE, "/Metadata.bin");

	BITMAPFILE = string_duplicate(METADATAPATH);
	string_append(&BITMAPFILE, "/Bitmap.bin");
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
			else{
				uint32_t r = 1;
				EnviarDatosTipo(socketFD, FS, &r,sizeof(uint32_t) , ESERROR);
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
