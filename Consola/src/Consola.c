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
	int contadorDeVariables = 0;
	int c;
	FILE *file;
	file = fopen("ArchivoConfiguracion.txt", "r");
	if (file) {
		while ((c = getc(file)) != EOF)
			if (c == '=')
			{
				if (contadorDeVariables == 1) {
					fscanf(file, "%i", &PUERTO_KERNEL);
				}
				if (contadorDeVariables == 0)
				{
					char buffer[10000];
					IP_KERNEL = fgets(buffer, sizeof buffer, file);
					strtok(IP_KERNEL, "\n");

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
	printf("%s",IP_KERNEL);
	return 0;
}
