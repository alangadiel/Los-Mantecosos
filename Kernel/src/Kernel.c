/*
 ============================================================================
 Name        : Kernel.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

int PUERTO_PROG;
int PUERTO_CPU;
char* IP_MEMORIA;
int PUERTO_MEMORIA;
char* IP_FS;
int PUERTO_FS;
int QUANTUM;
int QUANTUM_SLEEP;
char* ALGORITMO;
int GRADO_MULTIPROG;
char* SEM_IDS[];
int SEM_INIT[];
int SHARED_VARS[];
int STACK_SIZE;

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
