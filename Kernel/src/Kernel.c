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


int startServidor()
{
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);
	//printf("%i\n", socketFD);
	struct sockaddr_in estructuraDireccion;

	estructuraDireccion.sin_family = AF_INET;
	estructuraDireccion.sin_port = htons(PUERTO_PROG);
	estructuraDireccion.sin_addr.s_addr = inet_addr(IP_PROG);
	memset(&(estructuraDireccion.sin_zero), '\0', 8);

	bind(socketFD, (struct sockaddr*) &estructuraDireccion, sizeof(struct sockaddr));
	listen(socketFD, BACKLOG);

	return socketFD;
}


int aceptarConexion(int socketEscucha)
{
	struct sockaddr_in their_addr;
	socklen_t sin_size = sizeof(struct sockaddr_in);

	int nuevoFD = accept(socketEscucha, (struct sockaddr *)&their_addr, &sin_size);

	return nuevoFD;

}


int main(void)
{
	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()

	struct sockaddr_in remoteaddr; // dirección del cliente
	int fdmax; // número máximo de descriptores de fichero
	int SocketEscucha; // descriptor de socket a la escucha
	int nuevoSocket; // descriptor de socket de nueva conexión aceptada
	char buf[256]; // buffer para datos del cliente
	int nbytes;

	int addrlen;
	int i, j;

	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);

	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	//int SocketEscucha = startServidor();

	SocketEscucha = StartServidor(IP_PROG, PUERTO_PROG);

	FD_SET(SocketEscucha, &master); // añadir listener al conjunto maestro

	fdmax = SocketEscucha; // seguir la pista del descriptor de fichero mayor, por ahora es éste

	// bucle principal
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
					addrlen = sizeof(remoteaddr);

					if ((nuevoSocket = accept(SocketEscucha, (struct sockaddr*)&remoteaddr,&addrlen)) == -1)
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
					char* header = malloc(TAMANIOHEADER + 1);

					if ((nbytes = recv(i, header, TAMANIOHEADER, 0)) <= 0) // gestionar datos de un cliente
					{
						if (nbytes == 0) // error o conexión cerrada por el cliente
						{
							// conexión cerrada
							printf("\nselectserver: socket %d CONEXIÓN FINALIZADA\n", i);
						}
						else
						{
							perror("recv");
						}

						close(i); // ¡Hasta luego!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					}
					else
					{
						if(header[0] == '1')
						{
							printf("Gracias por conectarte\n");
							send(nuevoSocket,STRHANDSHAKE,TAMANIOHEADER,1);
						}
						else
						{
							recv(nuevoSocket, buf, header[1], 0);
							printf("\nEl mensaje recibido es: %s\n", buf);
						}

						for(j = 0; j <= fdmax; j++) // tenemos datos de algún cliente
						{
							// ¡enviar a todo el mundo!
							if (FD_ISSET(j, &master))
							{
								// excepto al listener y a nosotros mismos
								if (j != SocketEscucha && j != i)
								{
									int trySend = send(j, buf, nbytes, 0);
									if (trySend!=-1)
									{
										perror("send");
									}
								}
							}
						}
					}

					free(header);
				}
			}
		}
	}

	return 0;
}
