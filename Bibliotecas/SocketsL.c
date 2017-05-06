#include "SocketsL.h"

void Servidor(char* ip, int puerto, char nombre[11], void (*accion)(Paquete* paquete, int socketFD)){
	int SocketEscucha = StartServidor(ip, puerto);
	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);
	FD_SET(SocketEscucha, &master); // añadir listener al conjunto maestro
	int fdmax = SocketEscucha; // seguir la pista del descriptor de fichero mayor, por ahora es éste
	struct sockaddr_in remoteaddr; // dirección del cliente

	for(;;)	{	// bucle principal
		read_fds = master; // cópialo
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)		{
			perror("select");
			exit(1);
		}
		// explorar conexiones existentes en busca de datos que leer
		int i;
		for(i = 0; i <= fdmax; i++)	{
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == SocketEscucha) { // gestionar nuevas conexiones
					int addrlen = sizeof(remoteaddr);
					int nuevoSocket = accept(SocketEscucha, (struct sockaddr*)&remoteaddr,&addrlen);
					if (nuevoSocket == -1)
						perror("accept");
					else {
						FD_SET(nuevoSocket, &master); // añadir al conjunto maestro
						if (nuevoSocket > fdmax) fdmax = nuevoSocket; // actualizar el máximo
						printf("\nNueva conexion de %s en " "socket %d\n", inet_ntoa(remoteaddr.sin_addr),nuevoSocket);
					}
				}
				else  {
					Paquete paquete;
					int result = RecibirPaquete(i, nombre, &paquete);
					if(	result>0)
						accion(&paquete, i); //>>>>Esto hace el servidor cuando recibe algo<<<<
					else
						FD_CLR(i, &master); // eliminar del conjunto maestro si falla
					free (paquete.Payload); //Y finalmente, no puede faltar hacer el free
				}
			}
		}
	}
}

int ConectarAServidor(int puertoAConectar, char* ipAConectar, char servidor[11], char cliente[11])
{
	int socketFD = socket(AF_INET,SOCK_STREAM,0);

	struct sockaddr_in direccionKernel;

	direccionKernel.sin_family = AF_INET;
	direccionKernel.sin_port = htons(puertoAConectar);
	direccionKernel.sin_addr.s_addr = inet_addr(ipAConectar);
	memset(&(direccionKernel.sin_zero), '\0', 8);
	connect(socketFD,(struct sockaddr *)&direccionKernel, sizeof(struct sockaddr));

	EnviarHandshake(socketFD,cliente);
	RecibirHandshake(socketFD, servidor);

	return socketFD;
}


int StartServidor(char* MyIP, int MyPort) // obtener socket a la escucha
{
	struct sockaddr_in myaddr; // dirección del servidor
	int yes = 1; // para setsockopt() SO_REUSEADDR, más abajo
	int SocketEscucha;

	if ((SocketEscucha = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	if (setsockopt(SocketEscucha, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) // obviar el mensaje "address already in use" (la dirección ya se está 	usando)
	{
		perror("setsockopt");
		exit(1);
	}

	// enlazar
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = inet_addr(MyIP);
	myaddr.sin_port = htons(MyPort);
	memset(&(myaddr.sin_zero), '\0', 8);

	if (bind(SocketEscucha, (struct sockaddr*)&myaddr, sizeof(myaddr)) == -1)
	{
		perror("bind");
		exit(1);
	}

	// escuchar
	if (listen(SocketEscucha, 10) == -1)
	{
		perror("listen");
		exit(1);
	}

	return SocketEscucha;
}

void EnviarPaquete(int socketCliente, Paquete* paquete, int cantAEnviar)
{

	void* datos = malloc(cantAEnviar);
	memcpy(datos,&(paquete->header),TAMANIOHEADER);
	if(paquete->header.tipoMensaje!=ESHANDSHAKE)
		memcpy(datos+TAMANIOHEADER,(paquete->Payload),paquete->header.tamPayload);

	//Paquete* punteroMsg = datos;
	int enviado = 0; //bytes enviados
	int totalEnviado = 0;

	do
	{
		enviado = send(socketCliente,datos+totalEnviado,cantAEnviar-totalEnviado,0);
		//largo -= totalEnviado;
		totalEnviado += enviado;
		//punteroMsg += enviado; //avanza la cant de bytes que ya mando
	} while (totalEnviado != cantAEnviar);
	free(datos);
}


void EnviarInt(int socketFD, int numero,char emisor[11])
{
	Paquete* paquete = malloc(TAMANIOHEADER + sizeof(int));
	Header header;
	header.tipoMensaje= ESINT;
	header.tamPayload = sizeof(int);
	strcpy(header.emisor, emisor);
	//printf("%d",sizeof(header));

	paquete -> header = header;
	paquete -> Payload = numero;
	//paquete.Payload = msg;

	EnviarPaquete(socketFD,paquete,TAMANIOHEADER + sizeof(int));

	free(paquete);
}


void EnviarMensaje(int socketFD, char* msg,char emisor[11])
{
	Paquete* paquete = malloc(sizeof(Paquete));
	paquete->header.tipoMensaje= ESSTRING;
	paquete->header.tamPayload = string_length(msg)+1;
	strcpy(paquete->header.emisor, emisor);
	paquete -> Payload = msg;

	EnviarPaquete(socketFD,paquete,TAMANIOHEADER + paquete->header.tamPayload);

	free(paquete);
}

void EnviarHandshake(int socketFD,char emisor[11])
{
	Paquete* paquete = malloc(TAMANIOHEADER);
	Header header;
	header.tipoMensaje = ESHANDSHAKE;
	header.tamPayload = 0;
	strcpy(header.emisor, emisor);
	//printf("%d",sizeof(header));

	paquete -> header = header;
	//paquete -> Payload = ;
	//paquete.Payload = msg;

	EnviarPaquete(socketFD,paquete,TAMANIOHEADER);

	free(paquete);
}

void RecibirHandshake(int socketFD,char emisor[11])
{
	Header* header = malloc(TAMANIOHEADER);
	int resul = RecibirDatos(header, socketFD, TAMANIOHEADER);
	if(resul>0){ // si no hubo error en la recepcion
		if(strcmp(header->emisor,emisor)==0){
			if (header->tipoMensaje==ESHANDSHAKE) printf("\nConectado con el servidor!\n");
			else perror("Error de Conexion, no se recibio un handshake\n");
		} else perror("Error, no se recibio un handshake del servidor esperado\n");
	}
	free(header);
}

int RecibirDatos(void* paquete, int socketFD, uint32_t cantARecibir)
{
	void* datos = malloc(cantARecibir);
	//char* punteroMsg = paquete;
	int recibido = 0;
	int totalRecibido = 0;

	do
	{
		recibido = recv(socketFD, datos + totalRecibido, cantARecibir - totalRecibido, 0);
		//send(socketCliente,punteroMsg+totalEnviado,cantAEnviar-totalEnviado,0);
		totalRecibido += recibido;
	} while (totalRecibido != cantARecibir && recibido>0);
	memcpy(paquete,datos,cantARecibir);
	free(datos);

	if(recibido<0){
		perror("Error de Recepcion, no se pudo leer el mensaje\n");
		close(socketFD); // ¡Hasta luego!
	} else if (recibido==0){
		printf("\nSocket %d: ", socketFD);
		perror("Fin de Conexion, se cerro la conexion\n");
		close(socketFD); // ¡Hasta luego!
	}

	return recibido;
}

int RecibirPaquete(int socketFD, char receptor[11], Paquete* paquete){
	paquete->Payload= malloc(1);
	int resul = RecibirDatos(&(paquete->header),socketFD, TAMANIOHEADER);
	if(resul>0){ //si no hubo error
		if (paquete->header.tipoMensaje==ESHANDSHAKE){ //vemos si es un handshake
			printf("Se establecio conexion con %s\n", paquete->header.emisor);
			EnviarHandshake(socketFD, receptor);// paquete->header.emisor
		} else {  //recibimos un payload y lo procesamos (por ej, puede mostrarlo)
			paquete->Payload= realloc(paquete->Payload, paquete->header.tamPayload);
			resul= RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
		}
	}

	return resul;
}


/*
int RecibirHeader(int socketFD, Header* headerRecibido)
{
	//TODO: Hacer el malloc en donde invocas a la funcion, asi podemos obtener, cuando finaliza la funcion,
	//el emisor del mensaje y si poder validar si fue un handshake o no

	//Header* headerRecibido = malloc(TAMANIOHEADER);
	//Header* punteroMsg = headerRecibido;
	int var = RecibirDatos(headerRecibido, socketFD, TAMANIOHEADER);

	//chequear que sea el emisor
	//if (strcmp(emisor, headerRecibido -> emisor) != 0) var = -1;
	if(var<0)
		return ERRORRECEPCION;
	else if(var ==0)
		return CONEXIONCERRADA;
	else {
			if (headerRecibido->tamPayload>=0)
				return headerRecibido -> tamPayload;
			else
				return ERRORRECEPCION;
		}

}
int RecibirPayload(int socketFD,char* mensaje,uint32_t tamPayload)
{

	void* mensajeRecibido = malloc(tamPayload);
	int cantRecibida = RecibirDatos(mensajeRecibido, socketFD,tamPayload);
	strcpy(mensaje,mensajeRecibido);
	free(mensajeRecibido);
	return cantRecibida;

}
*/
