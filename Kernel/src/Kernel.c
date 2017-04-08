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
char* SEM_IDS[4];
int SEM_INIT[100];
char* SHARED_VARS[100];
int STACK_SIZE;
char* 	ObtenerTextoSinCorchetes(FILE* f){
	char buffer[10000];
	char *line = fgets(buffer,sizeof buffer,f);
	int length = string_length(line)-3;
	char *texto = string_substring(line,1,length);
	texto  = strtok(texto,",");
	return texto;
}

void obtenerValoresArchivoConfiguracion() {
	int contadorDeVariables = 0;
	int c;
	FILE *file;
	file = fopen("ArchivoConfiguracion.txt", "r");
	if (file) {
		while ((c = getc(file)) != EOF)
			if (c == '=')
			{
				if (contadorDeVariables == 13) {
					fscanf(file, "%i", &STACK_SIZE);
				}

				if (contadorDeVariables==12){
					char * texto = ObtenerTextoSinCorchetes(file);
					int i=0;
					while (texto != NULL)
						{
						 SHARED_VARS[i++] = texto;
						  texto = strtok (NULL, ",");
						}

					 contadorDeVariables++;
				}
				if (contadorDeVariables==11){
					char * texto = ObtenerTextoSinCorchetes(file);
					int i=0;
					 while (texto != NULL)
						{
							SEM_INIT[i++] = atoi(texto);
							texto = strtok (NULL, ",");
						}

					contadorDeVariables++;
				}
				if (contadorDeVariables==10){
					char * texto = ObtenerTextoSinCorchetes(file);
					int i=0;
					 while (texto != NULL)
					    {
					        SEM_IDS[i++] = texto;
					        texto = strtok (NULL, ",");
					    }

					contadorDeVariables++;
				}
				if (contadorDeVariables == 9) {
					fscanf(file, "%i", &GRADO_MULTIPROG);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 8) {
					char buffer[10000];
					ALGORITMO = fgets(buffer, sizeof buffer, file);
					strtok(ALGORITMO, "\n");
					contadorDeVariables++;
				}
				if (contadorDeVariables == 7) {
					fscanf(file, "%i", &QUANTUM_SLEEP);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 6)
				{
					fscanf(file, "%i", &QUANTUM);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 5) {
					fscanf(file, "%i", &PUERTO_FS);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 4) {
					char buffer[10000];
					IP_FS = fgets(buffer, sizeof buffer, file);
					strtok(IP_FS, "\n");
					contadorDeVariables++;
				}
				if (contadorDeVariables == 3) {
					fscanf(file, "%i", &PUERTO_MEMORIA);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 2)
				{
					char buffer[10000];
					IP_MEMORIA = fgets(buffer, sizeof buffer, file);
					strtok(IP_MEMORIA, "\n");
					contadorDeVariables++;
				}
				if (contadorDeVariables == 1) {
					fscanf(file, "%i", &PUERTO_CPU);
					contadorDeVariables++;
				}
				if (contadorDeVariables == 0) {
					fscanf(file, "%i", &PUERTO_PROG);
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
	//imprimirArchivoConfiguracion();
	return 0;
}
