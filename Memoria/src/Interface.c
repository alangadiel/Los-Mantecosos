#include "Interface.h"

void dumpMemoryContent() {
	if(cantPagAsignadas == 0){
		printf("La memoria esta vacia\n");
	} else {
		printf("Imprimiendo todo el contenido de la memoria\n");
		printf("%*s\n", tamanioTotalBytesMemoria, (char*)ContenidoMemoria);
		char nombreDelArchivo[64+27+1];//tam de la hora + tam de "Contenido de la memoria en " + \0
		char* t = temporal_get_string_time();
		sprintf(nombreDelArchivo, "Contenido de la memoria en %s", t);
		free(t);
		FILE* file = fopen(nombreDelArchivo, "w");
		fprintf(file, "%*s", tamanioTotalBytesMemoria, (char*)ContenidoMemoria);
		fclose(file);
	}
}

void dumpMemoryContentOfPID(uint32_t pid) {
	uint32_t cantPag = cuantasPagTiene(pid);
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
			printf("%*s", MARCO_SIZE, contenido);
			fprintf(file, "%*s", MARCO_SIZE, contenido);
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
		RegistroTablaPaginacion reg = TablaDePagina[i];
		fprintf(file, "Frame: %i, PID: %i, Pagina: %i\n", reg.Frame, reg.PID, reg.Pag);
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
		for(i=0; i<list_size(tablaCache);i++) {
			entrada = list_get(tablaCache, i);
			contenido = ContenidoMemoria + MARCO_SIZE*FrameLookup(entrada->PID, entrada->Pag);
			printf("PID: %u, Pag: %u: %*s\n",entrada->PID, entrada->Pag, MARCO_SIZE, contenido);
			fprintf(file, "PID: %u, Pag: %u: %*s\n",entrada->PID, entrada->Pag, MARCO_SIZE, contenido);
		}
		fclose(file);
	}
}
void cleanCache(){
	list_clean_and_destroy_elements(tablaCache, free);
}
void userInterfaceHandler(void* socketFD) {
	char* command = malloc(20);
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

			} else if (!strcmp(command, "all")){
				dumpMemoryContent();

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

				if (strtol(command, NULL, 10) != 0 && strtol(command, NULL, 10) <= (2^32)) {
					//que sea un numero y que no se pase del max del uint32
					printf("El proceso %li tiene %u paginas asignadas\n",
							strtol(command, NULL, 10), cuantasPagTiene(strtol(command, NULL, 10)));

				} else
					printf("Numero invalido\n");

			} else
				printf("No se conoce el comando %s\n", command);

		} else
			printf("No se conoce el comando %s\n", command);
	}
	free(command);
}
