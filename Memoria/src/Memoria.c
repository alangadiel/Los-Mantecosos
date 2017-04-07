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
	int contadorDeVariables = 0;
	int c;
	FILE *file;
	file = fopen("ArchivoConfiguracion.txt", "r");
	if (file) {
		while ((c = getc(file)) != EOF)
			if (c == '=')
			{
				if (contadorDeVariables == 6) {
					fscanf(file, "%i", &RETARDO_MEMORIA);
				}
				if (contadorDeVariables == 5)
				{
					char buffer[10000];
					REEMPLAZO_CACHE = fgets(buffer, sizeof buffer, file);
					strtok(REEMPLAZO_CACHE, "\n");
					contadorDeVariables++;
				}
				if (contadorDeVariables == 4) {
					fscanf(file, "%i", &CACHE_X_PROC);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 3) {
					fscanf(file, "%i", &ENTRADAS_CACHE);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 2) {
					fscanf(file, "%i", &MARCO_SIZE);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 1) {
					fscanf(file, "%i", &MARCOS);
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
