/*
 ============================================================================
 Name        : Memoria.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

int PUERTO;
int MARCOS;
int MARCO_SIZE;
int ENTRADAS_CACHE;
int CACHE_X_PROC;
char* REEMPLAZO_CACHE;
int RETARDO_MEMORIA;

void obtenerValoresArchivoConfiguracion() {
	//HACER
	//Se lee el archivo de configuracion y se llenan las variables globales

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

int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	return 0;
}
