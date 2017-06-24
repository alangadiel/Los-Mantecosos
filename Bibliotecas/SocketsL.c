#include "SocketsL.h"
uint32_t TamanioPaginaMemoria;

void Servidor(char* ip, int puerto, char nombre[11],
		void (*accion)(Paquete* paquete, int socketFD),
		int (*RecibirPaquete)(int socketFD, char receptor[11], Paquete* paquete)) {
	int SocketEscucha = StartServidor(ip, puerto);
	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);
	FD_SET(SocketEscucha, &master); // añadir listener al conjunto maestro
	int fdmax = SocketEscucha; // seguir la pista del descriptor de fichero mayor, por ahora es éste
	struct sockaddr_in remoteaddr; // dirección del cliente

	for (;;) {	// bucle principal
		read_fds = master; // cópialo
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}
		// explorar conexiones existentes en busca de datos que leer
		int i;
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == SocketEscucha) { // gestionar nuevas conexiones
					int addrlen = sizeof(remoteaddr);
					int nuevoSocket = accept(SocketEscucha,
							(struct sockaddr*) &remoteaddr, &addrlen);
					if (nuevoSocket == -1)
						perror("accept");
					else {
						FD_SET(nuevoSocket, &master); // añadir al conjunto maestro
						if (nuevoSocket > fdmax)
							fdmax = nuevoSocket; // actualizar el máximo
						printf("\nNueva conexion de %s en " "socket %d\n",
								inet_ntoa(remoteaddr.sin_addr), nuevoSocket);
					}
				} else {
					Paquete paquete;
					int result = RecibirPaquete(i, nombre, &paquete);
					if (result > 0)
						accion(&paquete, i); //>>>>Esto hace el servidor cuando recibe algo<<<<
					else
						FD_CLR(i, &master); // eliminar del conjunto maestro si falla
					free(paquete.Payload); //Y finalmente, no puede faltar hacer el free
				}
			}
		}
	}
}

void ServidorConcuerrente(char* ip, int puerto, char nombre[11], t_list** listaDeHilos,
		bool* terminar, void (*accionHilo)(void* socketFD)) {

	*terminar = false;
	*listaDeHilos = list_create();
	int socketFD = StartServidor(ip, puerto);
	struct sockaddr_in their_addr; // información sobre la dirección del cliente
	int new_fd;
	socklen_t sin_size;

	while(!*terminar) { // Loop Principal
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(socketFD, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
			perror("accept");
			continue;
		}
		printf("\nNueva conexion de %s en " "socket %d\n", inet_ntoa(their_addr.sin_addr), new_fd);
		//pthread_t hilo = agregarAListaHiloSiNoEsta(listaHilos, socketFD);
		pthread_t threadNuevo;
		pthread_create(&threadNuevo, NULL, (void*)accionHilo, &new_fd);
		structHilo* itemNuevo = malloc(sizeof(structHilo));
		itemNuevo->hilo = threadNuevo;
		itemNuevo->socket = new_fd;
		list_add(*listaDeHilos, itemNuevo);
	}
	printf("Apagando Servidor");
	close(socketFD);
	//libera los items de lista de hilos , destruye la lista y espera a que termine cada hilo.
	list_destroy_and_destroy_elements(*listaDeHilos,
			LAMBDA(void _(void* elem) { pthread_join(((structHilo*)elem)->hilo, NULL); }));

}

int ConectarAServidor(int puertoAConectar, char* ipAConectar, char servidor[11], char cliente[11],
		void RecibirElHandshake(int socketFD, char emisor[11])) {
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in direccionKernel;

	direccionKernel.sin_family = AF_INET;
	direccionKernel.sin_port = htons(puertoAConectar);
	direccionKernel.sin_addr.s_addr = inet_addr(ipAConectar);
	memset(&(direccionKernel.sin_zero), '\0', 8);
	if (connect(socketFD, (struct sockaddr *) &direccionKernel,
			sizeof(struct sockaddr))>=0){
		EnviarHandshake(socketFD, cliente);
		RecibirElHandshake(socketFD, servidor);
		return socketFD;
	}else{
		return -1;
	}
}

int StartServidor(char* MyIP, int MyPort) // obtener socket a la escucha
{
	struct sockaddr_in myaddr; // dirección del servidor
	int yes = 1; // para setsockopt() SO_REUSEADDR, más abajo
	int SocketEscucha;

	if ((SocketEscucha = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	if (setsockopt(SocketEscucha, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) // obviar el mensaje "address already in use" (la dirección ya se está 	usando)
			{
		perror("setsockopt");
		exit(1);
	}

	// enlazar
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = inet_addr(MyIP);
	myaddr.sin_port = htons(MyPort);
	memset(&(myaddr.sin_zero), '\0', 8);

	if (bind(SocketEscucha, (struct sockaddr*) &myaddr, sizeof(myaddr)) == -1) {
		perror("bind");
		exit(1);
	}

	// escuchar
	if (listen(SocketEscucha, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	return SocketEscucha;
}

void EnviarDatos(int socketFD, char emisor[11], void* datos, int tamDatos) {
	Paquete* paquete = malloc(sizeof(Paquete));
	paquete->header.tipoMensaje = ESDATOS;
	paquete->header.tamPayload = tamDatos;
	strcpy(paquete->header.emisor, emisor);
	paquete->Payload = datos;

	EnviarPaquete(socketFD, paquete);

	free(paquete);
}

void EnviarDatosTipo(int socketFD, char emisor[11], void* datos, int tamDatos, int tipoMensaje){
	Paquete* paquete = malloc(sizeof(Paquete));
	paquete->header.tipoMensaje = tipoMensaje;
	paquete->header.tamPayload = tamDatos;
	strcpy(paquete->header.emisor, emisor);
	paquete->Payload = datos;

	EnviarPaquete(socketFD, paquete);

	free(paquete);
}
void EnviarPaquete(int socketCliente, Paquete* paquete) {
	int cantAEnviar = sizeof(Header) + paquete->header.tamPayload;
	void* datos = malloc(cantAEnviar);
	memcpy(datos, &(paquete->header), TAMANIOHEADER);
	if (paquete->header.tamPayload > 0) //No sea handshake
		memcpy(datos + TAMANIOHEADER, (paquete->Payload),
				paquete->header.tamPayload);

	//Paquete* punteroMsg = datos;
	int enviado = 0; //bytes enviados
	int totalEnviado = 0;

	do {
		enviado = send(socketCliente, datos + totalEnviado,
				cantAEnviar - totalEnviado, 0);
		//largo -= totalEnviado;
		totalEnviado += enviado;
		//punteroMsg += enviado; //avanza la cant de bytes que ya mando
	} while (totalEnviado != cantAEnviar);
	free(datos);
}

void EnviarMensaje(int socketFD, char* msg, char emisor[11]) {
	Paquete paquete;
	strcpy(paquete.header.emisor, emisor);
	paquete.header.tipoMensaje = ESSTRING;
	paquete.header.tamPayload = string_length(msg) + 1;
	paquete.Payload = msg;
	EnviarPaquete(socketFD, &paquete);

}

void EnviarHandshake(int socketFD, char emisor[11]) {
	Paquete* paquete = malloc(TAMANIOHEADER);
	Header header;
	header.tipoMensaje = ESHANDSHAKE;
	header.tamPayload = 0;
	strcpy(header.emisor, emisor);
	paquete->header = header;
	EnviarPaquete(socketFD, paquete);

	free(paquete);
}

void RecibirHandshake(int socketFD, char emisor[11]) {
	Header* header = malloc(TAMANIOHEADER);
	int resul = RecibirDatos(header, socketFD, TAMANIOHEADER);
	if (resul > 0) { // si no hubo error en la recepcion
		if (strcmp(header->emisor, emisor) == 0) {
			if (header->tipoMensaje == ESHANDSHAKE)
				printf("\nConectado con el servidor %s\n", emisor);
			else
				perror("Error de Conexion, no se recibio un handshake\n");
		} else
			perror("Error, no se recibio un handshake del servidor esperado\n");
	}
	free(header);
}

void RecibirHandshake_DeMemoria(int socketFD, char emisor[11]){
	Paquete* paquete =  malloc(sizeof(Paquete));
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0 && paquete->header.tipoMensaje == ESHANDSHAKE) { //si no hubo error y es un handshake
		if (strcmp(paquete->header.emisor, emisor) == 0) {
				printf("\nConectado con el servidor!\n");
				if(strcmp(paquete->header.emisor, MEMORIA) == 0){
					paquete->Payload = malloc(paquete->header.tamPayload);
					resul = RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
					TamanioPaginaMemoria = *((uint32_t*)paquete->Payload);
					free(paquete->Payload);
				}
		} else
			perror("Error, no se recibio un handshake del servidor esperado\n");
	} else
		perror("Error de Conexion, no se recibio un handshake\n");

	free(paquete);
}

int RecibirDatos(void* paquete, int socketFD, uint32_t cantARecibir) {
	void* datos = malloc(cantARecibir);
	//char* punteroMsg = paquete;
	int recibido = 0;
	int totalRecibido = 0;

	do {
		recibido = recv(socketFD, datos + totalRecibido,
				cantARecibir - totalRecibido, 0);
		//send(socketCliente,punteroMsg+totalEnviado,cantAEnviar-totalEnviado,0);
		totalRecibido += recibido;
	} while (totalRecibido != cantARecibir && recibido > 0);
	memcpy(paquete, datos, cantARecibir);
	free(datos);

	if (recibido < 0) {
		perror("Error de Recepcion, no se pudo leer el mensaje\n");
		close(socketFD); // ¡Hasta luego!
	} else if (recibido == 0) {
		printf("\nSocket %d: ", socketFD);
		perror("Fin de Conexion, se cerro la conexion\n");
		close(socketFD); // ¡Hasta luego!
	}

	return recibido;
}

int RecibirPaqueteServidor(int socketFD, char receptor[11], Paquete* paquete) {
	paquete->Payload = malloc(1);
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0) { //si no hubo error
		if (paquete->header.tipoMensaje == ESHANDSHAKE) { //vemos si es un handshake
			printf("Se establecio conexion con %s\n", paquete->header.emisor);
			EnviarHandshake(socketFD, receptor); // paquete->header.emisor
		} else { //recibimos un payload y lo procesamos (por ej, puede mostrarlo)
			paquete->Payload = realloc(paquete->Payload,
					paquete->header.tamPayload);
			resul = RecibirDatos(paquete->Payload, socketFD,
					paquete->header.tamPayload);
		}
	}

	return resul;
}

int RecibirPaqueteCliente(int socketFD, char receptor[11], Paquete* paquete) {
	paquete->Payload = malloc(1);
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0 && paquete->header.tipoMensaje >= 0) { //si no hubo error ni es un handshake
		paquete->Payload = realloc(paquete->Payload,
				paquete->header.tamPayload);
		resul = RecibirDatos(paquete->Payload, socketFD,
				paquete->header.tamPayload);
	}

	return resul;
}

//Interfaz para comunicarse con Memoria: (definido por el TP, no se puede modificar)
bool IM_InicializarPrograma(int socketFD, char emisor[11], uint32_t ID_Prog,
		uint32_t CantPag) { //Solicitar paginas para un programa nuevo
	int tamDatos = sizeof(uint32_t) * 3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = INIC_PROG;
	((uint32_t*) datos)[1] = ID_Prog;
	((uint32_t*) datos)[2] = CantPag;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete paquete;
	while (RecibirPaqueteCliente(socketFD, MEMORIA, &paquete) <= 0);
	bool r = true;
	if (paquete.header.tipoMensaje==ESERROR) r = false;
	free(paquete.Payload);
	return r;
}
void* IM_LeerDatos(int socketFD, char emisor[11], uint32_t ID_Prog,
		uint32_t PagNum, uint32_t offset, uint32_t cantBytes) { //Devuelve los datos de una pagina, ¡Recordar hacer free(puntero) cuando los terminamos de usar!
	int tamDatos = sizeof(uint32_t) * 5;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = SOL_BYTES;
	((uint32_t*) datos)[1] = ID_Prog;
	((uint32_t*) datos)[2] = PagNum;
	((uint32_t*) datos)[3] = offset;
	((uint32_t*) datos)[4] = cantBytes;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, MEMORIA, paquete) <= 0);
	void* r;
	if(paquete->header.tipoMensaje == ESERROR)
		r = NULL;
	else if(paquete->header.tipoMensaje == ESDATOS)
		r = paquete->Payload;
	free(paquete);
	return r;
}
bool IM_GuardarDatos(int socketFD, char emisor[11], uint32_t ID_Prog,
		uint32_t PagNum, uint32_t offset, uint32_t cantBytes, void* buffer) {
	int tamDatos = sizeof(uint32_t) * 5 + cantBytes;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = ALM_BYTES;
	((uint32_t*) datos)[1] = ID_Prog;
	((uint32_t*) datos)[2] = PagNum;
	((uint32_t*) datos)[3] = offset;
	((uint32_t*) datos)[4] = cantBytes;
	memcpy(datos+sizeof(uint32_t) * 5, buffer, cantBytes);
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete paquete;
	while (RecibirPaqueteCliente(socketFD, MEMORIA, &paquete) <= 0);
	bool r = true;
	if (paquete.header.tipoMensaje==ESERROR) r = false;
	free(paquete.Payload);
	return r;
}
bool IM_AsignarPaginas(int socketFD, char emisor[11], uint32_t ID_Prog,
		uint32_t CantPag) { //Devuelve la cant de paginas que pudo asignar
	int tamDatos = sizeof(uint32_t) * 3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = ASIG_PAG;
	((uint32_t*) datos)[1] = ID_Prog;
	((uint32_t*) datos)[2] = CantPag;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete paquete;
	while (RecibirPaqueteCliente(socketFD, MEMORIA, &paquete) <= 0);
	bool r = true;
	if (paquete.header.tipoMensaje==ESERROR) r = false;
	free(paquete.Payload);
	return r;
}
bool IM_LiberarPagina(int socketFD, char emisor[11], uint32_t ID_Prog, uint32_t NumPag) {//Agregado en el Fe de Erratas, responde 0 si hubo error y 1 si libero la pag.
	int tamDatos = sizeof(uint32_t) * 3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = LIBE_PAG;
	((uint32_t*) datos)[1] = ID_Prog;
	((uint32_t*) datos)[2] = NumPag;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete paquete;
	while (RecibirPaqueteCliente(socketFD, MEMORIA, &paquete) <= 0);
	bool r = true;
	if (paquete.header.tipoMensaje==ESERROR) r = false;
	free(paquete.Payload);
	return r;
}

bool IM_FinalizarPrograma(int socketFD, char emisor[11], uint32_t ID_Prog) { //Borra las paginas de ese programa.
	int tamDatos = sizeof(uint32_t) * 2;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = FIN_PROG;
	((uint32_t*) datos)[1] = ID_Prog;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete paquete;
	while (RecibirPaqueteCliente(socketFD, MEMORIA, &paquete) <= 0);
	bool r = true;
	if (paquete.header.tipoMensaje==ESERROR) r = false;
	free(paquete.Payload);
	return r;
}

//interfaz filesystem
uint32_t FS_ValidarPrograma(int socketFD, char emisor[11], char* path) {
	int tamDatos = sizeof(uint32_t) + sizeof(char) * string_length(path);
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = VALIDAR_ARCHIVO;
	((char*) datos)[1] = path;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, FS, paquete) <= 0);
	uint32_t r = *(uint32_t*) (paquete->Payload);
	free(paquete->Payload);
	free(paquete);
	return r;
}
void* FS_CrearPrograma(int socketFD, char emisor[11], char* path) {
	int tamDatos = sizeof(uint32_t) + sizeof(char) * string_length(path);
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = CREAR_ARCHIVO;
	((char*) datos)[1] = path;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, FS, paquete) <= 0);
	uint32_t r = *(uint32_t*) (paquete->Payload);
	free(paquete->Payload);
	free(paquete);
	return r;

}
uint32_t FS_BorrarArchivo(int socketFD, char emisor[11], char* path) {
	int tamDatos = sizeof(uint32_t) + sizeof(char) * string_length(path);
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = BORRAR_ARCHIVO;
	((char*) datos)[1] = path;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, FS, paquete) <= 0);
	uint32_t r = *(uint32_t*) (paquete->Payload);
	free(paquete->Payload);
	free(paquete);
	return r;
}
uint32_t FS_ObtenerDatos(int socketFD, char emisor[11], char* path, uint32_t offset, uint32_t size) {
	int tamDatos = sizeof(uint32_t) * 3 + sizeof(char) * string_length(path);
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = OBTENER_DATOS;
	((char*) datos)[1] = path;
	((uint32_t*) datos)[2] = offset;
	((uint32_t*) datos)[3] = size;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, FS, paquete) <= 0);
	void* r;
	if(paquete->header.tipoMensaje == ESERROR)
		r = NULL;
	else if(paquete->header.tipoMensaje == ESDATOS)
		r = paquete->Payload;
	free(paquete);
	return r;
}
uint32_t FS_GuardarDatos(int socketFD, char emisor[11], char* path, int offset, int size, char* buffer) {//Agregado en el Fe de Erratas, responde 0 si hubo error y 1 si libero la pag.
	int tamDatos = sizeof(uint32_t) * 4 + + sizeof(char) * string_length(path);
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = GUARDAR_DATOS;
	((char*) datos)[1] = path;
	((uint32_t*) datos)[2] = offset;
	((uint32_t*) datos)[3] = size;
	((char*) datos)[4] = buffer;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, FS, paquete) <= 0);
	uint32_t r = *(uint32_t*) (paquete->Payload);
	free(paquete->Payload);
	free(paquete);
	return r;
}

void EnviarPCB(int socketCliente, char emisor[11], BloqueControlProceso* pecebe) {

	int i = 0;
	uint32_t sizeIndStack = list_size(pecebe->IndiceDelStack);
	t_size sizeIndCod = list_size(pecebe->IndiceDeCodigo);
	uint32_t sizeIndEtiq = dictionary_size(pecebe->IndiceDeEtiquetas);


	int tam = sizeof(uint32_t) * 9 + //con la cantidad de rafagas  y los tam de las listas
				sizeof(int32_t) +
				sizeof(uint32_t) * 2 * sizeIndCod +
				sizeIndEtiq * sizeof(t_puntero_instruccion) +
				pecebe->etiquetas_size * sizeof(char) +
				sizeof(t_size) * 2;

	void* pcbSerializado = malloc(tam);

	//Serialización etiquetas_size
	*((t_size*)pcbSerializado) = pecebe->etiquetas_size;
	pcbSerializado += sizeof(uint32_t);
	//Serialización etiquetas
	memcpy(pcbSerializado, pecebe->etiquetas, pecebe->etiquetas_size);
	pcbSerializado += pecebe->etiquetas_size;
	//Serialización cantRafagas
	*((uint32_t*)pcbSerializado) = pecebe->cantRafagas;
	pcbSerializado += sizeof(uint32_t);
	//Serialización acumRafagas
	*((uint32_t*)pcbSerializado) = pecebe->acumRafagas;
	pcbSerializado += sizeof(uint32_t);
	//Serialización PID
	*((uint32_t*)pcbSerializado) = pecebe->PID;
	pcbSerializado += sizeof(uint32_t);
	//Serialización ProgramCounter
	*((uint32_t*)pcbSerializado) = pecebe->ProgramCounter;
	pcbSerializado += sizeof(uint32_t);
	//Serialización PaginasDeCodigo
	*((uint32_t*)pcbSerializado) = pecebe->PaginasDeCodigo;
	pcbSerializado += sizeof(uint32_t);
	//Serialización ExitCode
	*((int32_t*)pcbSerializado) = pecebe->ExitCode;
	pcbSerializado += sizeof(int32_t);
	//Serialización cantidad_de_etiquetas
	*((uint32_t*)pcbSerializado) = pecebe->cantidad_de_etiquetas;
	pcbSerializado += sizeof(uint32_t);
	//Serialización cantidad_de_etiquetas
	*((uint32_t*)pcbSerializado) = pecebe->cantidad_de_funciones;
	pcbSerializado += sizeof(uint32_t);
	//Serialización IndiceDeCodigo
	*((uint32_t*)pcbSerializado) = sizeIndCod;
	pcbSerializado += sizeof(uint32_t);

	for (i = 0; i < sizeIndCod; i++){
		uint32_t* elem = list_get(pecebe->IndiceDeCodigo, i);
		*((uint32_t*)pcbSerializado) = elem[0];
		pcbSerializado += sizeof(uint32_t);
		*((uint32_t*)pcbSerializado)= elem[1];
		pcbSerializado += sizeof(uint32_t);
	}
	//Serialización IndiceDelStack

	*((uint32_t*)pcbSerializado) = sizeIndStack;
	pcbSerializado += sizeof(uint32_t);

	for (i = 0; i < sizeIndStack; i++){
		IndiceStack indice = *((IndiceStack*)(list_get(pecebe->IndiceDelStack, i)));
		uint32_t sizeVariables = list_size(indice.Variables);
		uint32_t sizeArgumentos = list_size(indice.Argumentos);
		int j;

		tam += sizeof(uint32_t) * 3 +
				sizeof(PosicionDeMemoria) +
				sizeVariables * sizeof(uint32_t) +
				sizeArgumentos * sizeof(uint32_t) ;

		pcbSerializado = realloc(pcbSerializado, tam);

		*((uint32_t*)pcbSerializado) = indice.DireccionDeRetorno;
		pcbSerializado += sizeof(uint32_t);
		*((PosicionDeMemoria*)pcbSerializado) = indice.PosVariableDeRetorno;
		pcbSerializado += sizeof(PosicionDeMemoria);
		*((uint32_t*)pcbSerializado) = sizeVariables;
		pcbSerializado += sizeof(uint32_t);

		for (j = 0; j < sizeVariables; j++){
			*((uint32_t*)pcbSerializado) = *((uint32_t*)list_get(indice.Variables, j));
			pcbSerializado += sizeof(uint32_t);
		}

		*((uint32_t*)pcbSerializado) = sizeArgumentos;
		pcbSerializado += sizeof(uint32_t);

		for (j = 0; j < sizeArgumentos; j++){
			*((uint32_t*)pcbSerializado) = *((uint32_t*)list_get(indice.Argumentos, j));
			pcbSerializado += sizeof(uint32_t);
		}
	}

	//Serialización Índice De Etiquetas
	*((uint32_t*)pcbSerializado) = sizeIndEtiq;
	pcbSerializado += sizeof(uint32_t);

	char** etiquetas = string_n_split(pecebe->etiquetas,
			pecebe->cantidad_de_etiquetas + pecebe->cantidad_de_funciones ,(char*)&sizeIndCod);
	string_iterate_lines(etiquetas, LAMBDA(void _(char* etiqueta) {
		t_puntero_instruccion* instruccion = dictionary_get(pecebe->IndiceDeEtiquetas, etiqueta);
		*((t_puntero_instruccion*)pcbSerializado) = *instruccion;
		pcbSerializado += sizeof(t_puntero_instruccion);
	}));
	free(etiquetas);


	//Lo envio
	pcbSerializado -= tam;
	EnviarDatos(socketCliente, emisor, pcbSerializado, tam);
	free(pcbSerializado);
}

void RecibirPCB(BloqueControlProceso* pecebe, int socketFD, char receptor[11], uint32_t* cantRafagas){
	Paquete paquete;
	RecibirPaqueteCliente(socketFD, receptor, &paquete);
	pcb_Create(pecebe);
	void* pcbSerializado = paquete.Payload;
	//TODO: cargar pcb
	pecebe->etiquetas_size = *(t_size*)pcbSerializado;
	pcbSerializado += sizeof(t_size);
	memcpy(pecebe->etiquetas, pcbSerializado, pecebe->etiquetas_size);
	pcbSerializado += pecebe->etiquetas_size;
	pecebe->cantRafagas = *(uint32_t*)pcbSerializado;
	pcbSerializado += sizeof(uint32_t);
	pecebe->acumRafagas= *(uint32_t*)pcbSerializado;
	pcbSerializado += sizeof(uint32_t);
	pecebe->PID = *(uint32_t*)pcbSerializado;
	pcbSerializado += sizeof(uint32_t);
	pecebe->ProgramCounter = *(uint32_t*)pcbSerializado;
	pcbSerializado += sizeof(uint32_t);
	pecebe->PaginasDeCodigo = *(uint32_t*)pcbSerializado;
	pcbSerializado += sizeof(uint32_t);
	pecebe->ExitCode = *(int32_t*)pcbSerializado;
	pcbSerializado += sizeof(int32_t);
	pecebe->cantidad_de_etiquetas = *(uint32_t*)pcbSerializado;
	pcbSerializado += sizeof(uint32_t);
	pecebe->cantidad_de_funciones = *(uint32_t*)pcbSerializado;
	pcbSerializado += sizeof(uint32_t);
	int sizeIndCod = *(uint32_t*)pcbSerializado;
	pcbSerializado += sizeof(uint32_t);
	int i;
	for (i = 0; i < sizeIndCod; i++){
		uint32_t* elem = malloc(sizeof(uint32_t) * 2);
		elem[0] = *(uint32_t*)pcbSerializado;
		pcbSerializado += sizeof(uint32_t*);
		elem[1] = *(uint32_t*)pcbSerializado;
		pcbSerializado += sizeof(uint32_t*);
		list_add(pecebe->IndiceDeCodigo, elem);
	}

	int sizeIndStack = *(uint32_t*)pcbSerializado;
	pcbSerializado += sizeof(uint32_t);

	for (i = 0; i < sizeIndStack; i++){
		IndiceStack* ind = malloc(sizeof(IndiceStack));
		ind->DireccionDeRetorno = *(uint32_t*) pcbSerializado;
		pcbSerializado += sizeof(uint32_t);
		ind->PosVariableDeRetorno = *(PosicionDeMemoria*)pcbSerializado;
		pcbSerializado += sizeof(PosicionDeMemoria);
		int sizeVariables = *(uint32_t*) pcbSerializado;
		pcbSerializado += sizeof(uint32_t);
		ind->Variables = list_create();
		int j;
		for (j = 0; j < sizeVariables; i++){
			uint32_t* elem = malloc(sizeof(uint32_t));
			*elem = *(uint32_t*)pcbSerializado;
			pcbSerializado += sizeof(uint32_t);
			list_add(ind->Variables, elem);
		}
		int sizeArgumentos = *(uint32_t*)pcbSerializado;
		pcbSerializado += sizeof(uint32_t);
		ind->Argumentos = list_create();
		for (j = 0; j < sizeArgumentos; i++){
			uint32_t* elem = malloc(sizeof(uint32_t));
			*elem = *(uint32_t*)pcbSerializado;;
			pcbSerializado += sizeof(uint32_t);
			list_add(ind->Argumentos, elem);
		}
		list_add(pecebe->IndiceDelStack,ind);
	}

	int sizeIndEtiq = *(uint32_t*)pcbSerializado;
	pcbSerializado += sizeof(uint32_t);
	char** etiquetas = string_n_split(pecebe->etiquetas,
			pecebe->cantidad_de_etiquetas + pecebe->cantidad_de_funciones,
			(char*)&sizeIndCod);
	string_iterate_lines(etiquetas, LAMBDA(void _(char* etiqueta) {
			t_puntero_instruccion* inst = malloc(sizeof(t_puntero_instruccion));
			*inst = *(t_puntero_instruccion*)pcbSerializado;
			pcbSerializado += sizeof(t_puntero_instruccion);
			dictionary_put(pecebe->IndiceDeEtiquetas, etiqueta, inst);
		}));
	free(etiquetas);
	free(paquete.Payload);
}

