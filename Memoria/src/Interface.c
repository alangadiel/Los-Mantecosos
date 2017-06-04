#include "Interface.h"

void dumpMemoryContent() {
	printf("Imprimiendo todo el contenido de la memoria");
	printf("%*s", tamanioTotalBytesMemoria, (char*)ContenidoMemoria);
	char nombreDelArchivo[64+27+1];//tam de la hora + tam de "Contenido de la memoria en " + \0
	sprintf(nombreDelArchivo, "Contenido de la memoria en %s", obtenerTiempoString(time(0)));
	FILE* file = fopen(nombreDelArchivo, "w");
	fprintf(file, "%*s", tamanioTotalBytesMemoria, (char*)ContenidoMemoria);
	fclose(file);
}

void dumpMemoryContentOfPID(uint32_t pid) {
	printf("Imprimiendo el contenido en memoria del proceso %d", pid);
	//TODO
}

void dumpMemoryStruct() { //TODO
	printf("Imprimiendo las estructuras de la memoria");
	char nombreDelArchivo[64+29+1];//tam de la hora + tam de "Estructuras de la memoria en " + \0
	sprintf(nombreDelArchivo, "Estructuras de la memoria en %s", obtenerTiempoString(time(0)));
	FILE* file = fopen(nombreDelArchivo, "w");
	int i;
	for (i = 0; i < MARCOS; i++) {
		RegistroTablaPaginacion reg = TablaDePagina[i];
		fprintf(file, "Frame: %i, PID: %i, Pagina: %i\n", reg.Frame, reg.PID, reg.Pag);
	}
	fclose(file);
}

void printMemorySize() {
	//TODO

	printf("Imprimiendo el tamaño total de la memoria");
}
void printProcessSize(uint32_t pid) {
	//TODO

	printf("Imprimiendo el tamaño en memoria del proceso %d", pid);
}

void userInterfaceHandler(void* socketFD) {
	char* command = malloc(20);
	while (!end) {
		printf("\n\nIngrese un comando: \n");
		scanf("%s", command);
		//string_trim(&command);
		//int c = strcmp(command, "retardo​");
		if (!strcmp(command, "retardo")) { // !int es lo mismo que int!=0
			command[0] = '\0'; //lo vacio
			scanf("%s", command);
			if (strtol(command, NULL, 10) != 0 && strtol(command, NULL, 10) < UINT_MAX) {
				RETARDO_MEMORIA = strtol(command, NULL, 10);
				printf("el retardo de la memoria ahora es %u", RETARDO_MEMORIA);
			} else printf("Numero invalido");

		} else if (!strcmp(command, "dump")) {
			command[0] = '\0'; //lo vacio
			scanf("%s", command);

			if (!strcmp(command, "cache")) {
				//TODO mostrar cache

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

				else printf("Numero invalido");

			} else
				printf("No se conoce el comando %s\n", command);

		} else if (!strcmp(command, "disconnect")) {
			end = 0;

		} else if (!strcmp(command, "flush")) {
			command[0] = '\0'; //lo vacio
			scanf("%s", command);

			if (!strcmp(command, "cache")) {
			//TODO limpiar cache

			} else printf("No se conoce el comando %s\n", command);

		} else if (!strcmp(command, "size")) {
			command[0] = '\0'; //lo vacio
			scanf("%s", command);

			if (!strcmp(command, "memory")) {
				printMemorySize();

			} else if (!strcmp(command, "pid")) {
				command[0] = '\0'; //lo vacio
				scanf("%s", command);

				if (strtol(command, NULL, 10) != 0 && strtol(command, NULL, 10) <= (2^32)) {
					//que sea un numero y que no se pase del max del uint32
					printProcessSize(strtol(command, NULL, 10));
				} else printf("Numero invalido");

			} else
				printf("No se conoce el comando %s\n", command);

		} else
			printf("No se conoce el comando %s\n", command);
	}
	free(command);
}
