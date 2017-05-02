#include "SocketsL.h"

#define BACKLOG 6 //Backlog es el maximo de peticiones pendientes


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

char* ObtenerTextoSinCorchetes(FILE* f) //Para obtener los valores de los arrays del archivo de configuracion
{
	char buffer[10000];
	char *line = fgets(buffer,sizeof buffer,f);
	int length = string_length(line)-3;
	char *texto = string_substring(line,1,length);

	texto  = strtok(texto,",");

	return texto;
}
void obtenerValoresArchivoConfiguracion()
{
	int contadorDeVariables = 0;
	int c;
	FILE *file;

	file = fopen("ArchivoConfiguracion.txt", "r");

	if (file)
	{
		while ((c = getc(file)) != EOF)
		{
			if (c == '=')
			{
				if (contadorDeVariables == 14)
				{
					char buffer[10000];

					IP_PROG = fgets(buffer, sizeof buffer, file);
					strtok(IP_PROG, "\n");
				}

				if (contadorDeVariables == 13)
				{
					fscanf(file, "%i", &STACK_SIZE);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 12)
				{
					char* texto = ObtenerTextoSinCorchetes(file);
					int i = 0;

					while (texto != NULL)
					{
						SHARED_VARS[i++] = texto;
						texto = strtok (NULL, ",");
					}

					contadorDeVariables++;
				}

				if (contadorDeVariables == 11)
				{
					char* texto = ObtenerTextoSinCorchetes(file);
					int i = 0;

					while (texto != NULL)
					{
						SEM_INIT[i++] = atoi(texto);
						texto = strtok (NULL, ",");
					}

					contadorDeVariables++;
				}

				if (contadorDeVariables == 10){
					char * texto = ObtenerTextoSinCorchetes(file);
					int i = 0;

					while (texto != NULL)
					{
						SEM_IDS[i++] = texto;
						texto = strtok (NULL, ",");
					}

					contadorDeVariables++;
				}

				if (contadorDeVariables == 9)
				{
					fscanf(file, "%i", &GRADO_MULTIPROG);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 8)
				{
					char buffer[10000];

					ALGORITMO = fgets(buffer, sizeof buffer, file);
					strtok(ALGORITMO, "\n");

					contadorDeVariables++;
				}

				if (contadorDeVariables == 7)
				{
					fscanf(file, "%i", &QUANTUM_SLEEP);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 6)
				{
					fscanf(file, "%i", &QUANTUM);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 5)
				{
					fscanf(file, "%i", &PUERTO_FS);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 4)
				{
					char buffer[10000];

					IP_FS = fgets(buffer, sizeof buffer, file);
					strtok(IP_FS, "\n");

					contadorDeVariables++;
				}

				if (contadorDeVariables == 3)
				{
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

				if (contadorDeVariables == 1)
				{
					fscanf(file, "%i", &PUERTO_CPU);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 0)
				{
					fscanf(file, "%i", &PUERTO_PROG);
					contadorDeVariables++;
				}
			}
		}

		fclose(file);
	}
}
void imprimirArchivoConfiguracion()
{
	int c;
	FILE *file;

	file = fopen("ArchivoConfiguracion.txt", "r");

	if (file)
	{
		while ((c = getc(file)) != EOF)
		{
			putchar(c);
		}

		fclose(file);
	}
}

int main(void)
{
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();

	int SocketEscucha = StartServidor(IP_PROG, PUERTO_PROG);

	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);
	FD_SET(SocketEscucha, &master); // añadir listener al conjunto maestro
	int fdmax = SocketEscucha; // seguir la pista del descriptor de fichero mayor, por ahora es éste
	struct sockaddr_in remoteaddr; // dirección del cliente
	// bucle principal
	int i;
	for(;;)
	{
		read_fds = master; // cópialo

		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
		{
			perror("select");
			exit(1);
		}

		// explorar conexiones existentes en busca de datos que leer
		for(i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &read_fds)) // ¡¡tenemos datos!!
			{
				if (i == SocketEscucha) // gestionar nuevas conexiones
				{
					int addrlen = sizeof(remoteaddr);
					int nuevoSocket = accept(SocketEscucha, (struct sockaddr*)&remoteaddr,&addrlen);
					if (nuevoSocket == -1)
					{
						perror("accept");
					}
					else
					{
						FD_SET(nuevoSocket, &master); // añadir al conjunto maestro

						if (nuevoSocket > fdmax)
						{
							fdmax = nuevoSocket; // actualizar el máximo
						}

						printf("\n\nselectserver: new connection from %s on " "socket %d\n", inet_ntoa(remoteaddr.sin_addr),	nuevoSocket);
					}
				}
				else
				{
					// inicio de transmision
					Header* header = malloc(TAMANIOHEADER);

					int resul = RecibirPaquete(header, i, TAMANIOHEADER);
						if(resul<0){
							printf("\nSocket %d: ", i);
							perror("Error de Recepcion, no se pudo leer el mensaje\n");
							close(i); // ¡Hasta luego!
							FD_CLR(i, &master); // eliminar del conjunto maestro
						} else if (resul==0){

							printf("\nSocket %d: ", i);
							perror("Fin de Conexion, se cerro la conexion\n");
							close(i); // ¡Hasta luego!
							FD_CLR(i, &master); // eliminar del conjunto maestro

						} else {
						//vemos si es un handshake
							if (header->esHandShake=='1'){
								printf("\nGracias por conectarte\n");
								EnviarHandshake(i, "Kernel");
							} else {
								//Paquete* paquete = malloc(TAMANIOHEADER + header->tamPayload);
								char* payload= malloc((header->tamPayload) + 1); //esto solo funciona con texto
								RecibirPaquete(payload, i, header->tamPayload);

								printf("\nTexto recibido: %s", payload);
								free (payload);
							}


					}
						free(header);
					//fin de transmision
				}
			}
		}
	}

	return 0;
}
