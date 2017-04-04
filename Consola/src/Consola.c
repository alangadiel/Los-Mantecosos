/*
 ============================================================================
 Name        : Consola.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/txt.h>

char* IP_KERNEL;
int PUERTO_KERNEL;

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
