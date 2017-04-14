/*
 ============================================================================
 Name        : Kernel.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */


#include "SocketsL.h"

#define BACKLOG 6


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
char* IP_PROG;


char* ObtenerTextoSinCorchetes(FILE* f){     //Para obtener los valores de los arrays del archivo de configuracion
	char buffer[10000];
	char *line = fgets(buffer,sizeof buffer,f);
	//int length = string_length(line)-3;
	//char *texto = string_substring(line,1,length);
	//texto  = strtok(texto,",");
	//return texto;
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
				if (contadorDeVariables == 14) {
					char buffer[10000];
					IP_PROG = fgets(buffer, sizeof buffer, file);
					strtok(IP_PROG, "\n");
				}
				if (contadorDeVariables == 13) {
					fscanf(file, "%i", &STACK_SIZE);
					contadorDeVariables++;

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

int startServidor(){
	int socketFD = socket(AF_INET,SOCK_STREAM,0);
	printf("%i\n", socketFD);
	struct sockaddr_in estructuraDireccion;
	estructuraDireccion.sin_family = AF_INET;
	estructuraDireccion.sin_port = htons(5000/*PUERTO_PROG*/);
	estructuraDireccion.sin_addr.s_addr = htonl("10.0.2.15" /*IP_PROG*/);
	//Backlog es el maximo de peticiones pendientes
	bind(socketFD,(struct sockaddr *) &estructuraDireccion,sizeof(struct sockaddr));
	listen(socketFD,BACKLOG);
	return socketFD;
}
int aceptarConexion(int socketEscucha){
	struct sockaddr_in their_addr;
	socklen_t sin_size = sizeof(struct sockaddr_in);
	int nuevoFD = accept(socketEscucha, (struct sockaddr *)&their_addr, &sin_size);
	return nuevoFD;

}

int main(void) {
	//socket_t socket = iniciarServidor(PUERTO_PROG);
	//socket_t socketEscucha = realizarConexion(3500);
	//obtenerValoresArchivoConfiguracion();
	//imprimirArchivoConfiguracion();
	int SocketEscucha = startServidor();
	char str[100];
	printf( "Enter a value :");
	scanf("%s", str);
	printf( "\nYou entered: %s \n", str);
	int nuevoSocket=aceptarConexion(SocketEscucha);
	Handshake(nuevoSocket);
	printf("%d\n",nuevoSocket);
	close(SocketEscucha);
	close(nuevoSocket);
	return 0;
}
