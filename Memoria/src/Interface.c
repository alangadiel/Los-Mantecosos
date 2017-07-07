#include "Interface.h"

void dumpMemoryContent() {
	/*if(cantPagAsignadas == 0){
		printf("La memoria esta vacia\n");
	} else {*/
	printf("Imprimiendo todo el contenido de la memoria\n");
	char nombreDelArchivo[64+27+1];//tam de la hora + tam de "Contenido de la memoria en " + \0
	char* t = temporal_get_string_time();
	sprintf(nombreDelArchivo, "Contenido de la memoria en %s", t);
	free(t);
	FILE* file = fopen(nombreDelArchivo, "w");
	uint32_t i;
	for(i=0; i<MARCOS; i++){
		pthread_mutex_lock( &mutexTablaPagina );
		uint32_t pid = TablaDePagina[i].PID;
		pthread_mutex_unlock( &mutexTablaPagina );
		if(pid!=0) {
			char* contenido = ContenidoMemoria + (i * MARCO_SIZE);
			pthread_mutex_lock( &mutexContenidoMemoria );
			printf("%*s", MARCO_SIZE, contenido);
			fprintf(file, "%*s", MARCO_SIZE, contenido);
			pthread_mutex_unlock( &mutexContenidoMemoria );
		}
	}
	fclose(file);

}
void dumpVariablesOfPID(uint32_t pid,uint32_t pagCodigo){
	uint32_t cantPag = cuantasPagTieneTodos(pid);
	if (cantPag == 0){
		printf("El proceso no existe\n");
	} else {
		printf("Imprimiendo numeros menores a 1000 en memoria del proceso %d\n", pid);
		char nombreDelArchivo[200];
		char* t = temporal_get_string_time();
		sprintf(nombreDelArchivo, "Contenido del proceso %i en %s", pid, t);
		free(t);
		FILE* file = fopen(nombreDelArchivo, "w");
		uint32_t i, j;
		for(i=pagCodigo; i<cantPag; i++){
			void* contenidoPagina = ContenidoMemoria + (FrameLookup(pid, i) * MARCO_SIZE);
			for(j=0; j<MARCO_SIZE; j+=4){
				int* numero = contenidoPagina + j;
				pthread_mutex_lock( &mutexContenidoMemoria );
				if(*numero<1000){
					printf("%i, ", *numero);
					fprintf(file, "%i, ", *numero);
				}
				pthread_mutex_unlock( &mutexContenidoMemoria );
			}
			printf("\n");
			fprintf(file, "\n");
		}
		fclose(file);
	}
}
void dumpMemoryContentOfPID(uint32_t pid) {
	uint32_t cantPag = cuantasPagTieneTodos(pid);
	if (cantPag == 0){
		printf("El proceso no existe\n");
	} else {
		printf("Imprimiendo el contenido en memoria del proceso %d\n", pid);
		char nombreDelArchivo[200];
		char* t = temporal_get_string_time();
		sprintf(nombreDelArchivo, "Contenido del proceso %i en %s", pid, t);
		free(t);
		FILE* file = fopen(nombreDelArchivo, "w");
		uint32_t i;
		for(i=0; i<cantPag; i++){
			char* contenido = ContenidoMemoria + (FrameLookup(pid, i) * MARCO_SIZE);
			pthread_mutex_lock( &mutexContenidoMemoria );
			printf("%*s", MARCO_SIZE, contenido);
			fprintf(file, "%*s", MARCO_SIZE, contenido);
			pthread_mutex_unlock( &mutexContenidoMemoria );
		}
		fclose(file);
	}
}

void dumpMemoryStruct() {
	printf("Imprimiendo las estructuras de la memoria\n");
	char nombreDelArchivo[64+29+1];//tam de la hora + tam de "Estructuras de la memoria en " + \0
	char* t = temporal_get_string_time();
	sprintf(nombreDelArchivo, "Estructuras de la memoria en %s", t);
	free(t);
	FILE* file = fopen(nombreDelArchivo, "w");
	int i;
	for (i = 0; i < MARCOS; i++) {
		pthread_mutex_lock( &mutexTablaPagina );
		RegistroTablaPaginacion reg = TablaDePagina[i];
		pthread_mutex_unlock( &mutexTablaPagina );
		if(reg.PID!=0){
			printf("Frame: %i, PID: %i, Pagina: %i, Disponible: %i\n", reg.Frame, reg.PID, reg.Pag, reg.disponible);
			fprintf(file, "Frame: %i, PID: %i, Pagina: %i, Disponible: %i\n", reg.Frame, reg.PID, reg.Pag, reg.disponible);
		}
	}
	fclose(file);
}
void dumpMemoryContentOfCache() {
	if(list_is_empty(tablaCache)){
		printf("La memoria cache esta vacia\n");
	} else {
		printf("Imprimiendo todo el contenido de la memoria cache\n");
		char nombreDelArchivo[64+33+1];//tam de la hora + tam de "Contenido de la memoria cache en " + \0
		char* t = temporal_get_string_time();
		//char* tr = string_substring_from(t, ) TODO, que no se guarden con la fecha, solo la hora
		sprintf(nombreDelArchivo, "Contenido de la memoria cache en %s", t);
		free(t);
		FILE* file = fopen(nombreDelArchivo, "w");
		int i; entradaCache* entrada;
		char* contenido;
		pthread_mutex_lock( &mutexTablaCache );
		for(i=0; i<list_size(tablaCache);i++) {
			entrada = list_get(tablaCache, i);
			pthread_mutex_lock( &mutexContenidoMemoria );
			contenido = ContenidoMemoria + MARCO_SIZE*FrameLookup(entrada->PID, entrada->Pag);
			pthread_mutex_unlock( &mutexContenidoMemoria );
			printf("PID: %u, Pag: %u: %*s\n",entrada->PID, entrada->Pag, MARCO_SIZE, contenido);
			fprintf(file, "PID: %u, Pag: %u: %*s\n",entrada->PID, entrada->Pag, MARCO_SIZE, contenido);
		}
		pthread_mutex_unlock( &mutexTablaCache );
		fclose(file);
	}
}
void dumpMemoryContentOfOldCache() {
	if(list_is_empty(tablaCacheRastro)){
		printf("La memoria cache esta vacia\n");
	} else {
		printf("Imprimiendo todo el contenido de la memoria cache\n");
		char nombreDelArchivo[64+33+1];//tam de la hora + tam de "Contenido de la memoria cache en " + \0
		char* t = temporal_get_string_time();
		//char* tr = string_substring_from(t, ) TODO, que no se guarden con la fecha, solo la hora
		sprintf(nombreDelArchivo, "Contenido de la memoria cache en %s", t);
		free(t);
		FILE* file = fopen(nombreDelArchivo, "w");
		int i; entradaCache* entrada;
		char* contenido;
		pthread_mutex_lock( &mutexTablaCache );
		for(i=0; i<list_size(tablaCacheRastro);i++) {
			entrada = list_get(tablaCacheRastro, i);
			pthread_mutex_lock( &mutexContenidoMemoria );
			contenido = ContenidoMemoria + MARCO_SIZE*FrameLookup(entrada->PID, entrada->Pag);
			pthread_mutex_unlock( &mutexContenidoMemoria );
			printf("PID: %u, Pag: %u: %*s\n",entrada->PID, entrada->Pag, MARCO_SIZE, contenido);
			fprintf(file, "PID: %u, Pag: %u: %*s\n",entrada->PID, entrada->Pag, MARCO_SIZE, contenido);
		}
		pthread_mutex_unlock( &mutexTablaCache );
		fclose(file);
	}
}
void cleanCache(){
	pthread_mutex_lock( &mutexTablaCache );
	list_clean_and_destroy_elements(tablaCache, free);
	pthread_mutex_unlock( &mutexTablaCache );
}
void userInterfaceHandler(void* socketFD) {
	char* command = malloc(50);
	while (!end) {
		printf("\nIngrese un comando: \n");
		scanf("%s", command);
		//string_trim(&command);
		//int c = strcmp(command, "retardo​");
		if (!strcmp(command, "retardo")) { // !int es lo mismo que int!=0
			command[0] = '\0'; //lo vacio
			scanf("%s", command);

			if (strtol(command, NULL, 10) != 0 && strtol(command, NULL, 10) < UINT_MAX) {
				RETARDO_MEMORIA = strtol(command, NULL, 10);
				printf("el retardo de la memoria ahora es %u", RETARDO_MEMORIA);

			} else
				printf("Numero invalido\n");

		} else if (!strcmp(command, "dump")) {
			command[0] = '\0'; //lo vacio
			scanf("%s", command);

			if (!strcmp(command, "cache")) {
				dumpMemoryContentOfCache();

			} else if (!strcmp(command, "struct")){
				dumpMemoryStruct();

			} else if (!strcmp(command, "old")){
				command[0] = '\0'; //lo vacio
				scanf("%s", command);
				if (!strcmp(command, "cache")){
					dumpMemoryContentOfOldCache();
				} else
					printf("No se conoce el comando %s\n", command);

			} else if (!strcmp(command, "all")){
				dumpMemoryContent();

			} else if (!strcmp(command, "var")){ //para ver las variables que son Int
				printf("Ingrese cantidad de paginas de codigo (para que no se impriman)\n");
				command[0] = '\0'; //lo vacio
				scanf("%s", command);
				long pagCodigo = strtol(command, NULL, 10);
				if (pagCodigo != 0 && pagCodigo <= (2^32)) { //que sea un numero y que no se pase del max del uint32
					command[0] = '\0'; //lo vacio
					scanf("%s", command);
					if (!strcmp(command, "pid")) {
						command[0] = '\0'; //lo vacio
						scanf("%s", command);
						long pid = strtol(command, NULL, 10);
						if (pid != 0 && pid <= (2^32)) {
							//que sea un numero y que no se pase del max del uint32
							uint32_t _pid = pid;
							uint32_t _pag = pagCodigo;
							dumpVariablesOfPID(_pid, _pag);

						} else
							printf("No se conoce el comando %s\n", command);

					} else
						printf("Numero invalido\n");

				} else
					printf("Numero invalido\n");

			} else if (!strcmp(command, "pid")) {
				command[0] = '\0'; //lo vacio
				scanf("%s", command);

				if (strtol(command, NULL, 10) != 0 && strtol(command, NULL, 10) <= (2^32))
					//que sea un numero y que no se pase del max del uint32
					dumpMemoryContentOfPID(strtol(command, NULL, 10));

				else
					printf("Numero invalido\n");

			} else
				printf("No se conoce el comando %s\n", command);

		} else if (!strcmp(command, "disconnect")) {
			end = 0;

		} else if (!strcmp(command, "flush")) {
			command[0] = '\0'; //lo vacio
			scanf("%s", command);

			if (!strcmp(command, "cache")) {
				cleanCache();

			} else
				printf("No se conoce el comando %s\n", command);

		} else if (!strcmp(command, "size")) {
			command[0] = '\0'; //lo vacio
			scanf("%s", command);

			if (!strcmp(command, "memory")) {
				printf("La memoria tiene %u frames en total, de los cuales %i estan ocupados y %i libres.\n",
						MARCOS, cantPagAsignadas, MARCOS-cantPagAsignadas);

			} else if (!strcmp(command, "pid")) {
				command[0] = '\0'; //lo vacio
				scanf("%s", command);
				long pid = strtol(command, NULL, 10);
				if (pid != 0 && pid <= (2^32)) {
					//que sea un numero y que no se pase del max del uint32
					printf("El proceso %li tiene %u paginas asignadas, y llego a tener %u.\n",
							pid, cuantasPagTieneVivos(pid), cuantasPagTieneTodos(pid));

				} else
					printf("Numero invalido\n");

			} else
				printf("No se conoce el comando %s\n", command);

		} else
			printf("No se conoce el comando %s\n", command);
	}
	free(command);
}
