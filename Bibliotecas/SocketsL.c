#include "SocketsL.h"
uint32_t TamanioPaginaMemoria;
uint32_t StackSizeEnPaginas;

void Servidor(char* ip, int puerto, char nombre[11],
		void (*accion)(Paquete* paquete, int socketFD),
		int (*RecibirPaquete)(int socketFD, char receptor[11], Paquete* paquete)) {
	printf("Iniciando Servidor %s\n", nombre);
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
					socklen_t addrlen = sizeof(remoteaddr);
					int nuevoSocket = accept(SocketEscucha,
							(struct sockaddr*) &remoteaddr, &addrlen);
					if (nuevoSocket == -1)
						perror("accept");
					else {
						FD_SET(nuevoSocket, &master); // añadir al conjunto maestro
						if (nuevoSocket > fdmax)
							fdmax = nuevoSocket; // actualizar el máximo
						printf("\nConectando con %s en " "socket %d\n",
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

void ServidorConcurrente(char* ip, int puerto, char nombre[11], t_list** listaDeHilos,
		bool* terminar, void (*accionHilo)(void* socketFD)) {
	printf("Iniciando Servidor %s\n", nombre);
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
		structHilo* itemNuevo = malloc(sizeof(structHilo));
		itemNuevo->socket = new_fd;
		pthread_create(&(itemNuevo->hilo), NULL, (void*)accionHilo, &new_fd);
		list_add(*listaDeHilos, itemNuevo);
	}
	printf("Apagando Servidor");
	close(socketFD);
	//libera los items de lista de hilos , destruye la lista y espera a que termine cada hilo.
	list_destroy_and_destroy_elements(*listaDeHilos, LAMBDA(void _(void* elem) {
			pthread_join(((structHilo*)elem)->hilo, NULL);
			free(elem); }));

}

int ConectarAServidor(int puertoAConectar, char* ipAConectar, char servidor[11],
		char cliente[11], void RecibirElHandshake(int socketFD, char emisor[11])) {
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in direccionKernel;

	direccionKernel.sin_family = AF_INET;
	direccionKernel.sin_port = htons(puertoAConectar);
	direccionKernel.sin_addr.s_addr = inet_addr(ipAConectar);
	memset(&(direccionKernel.sin_zero), '\0', 8);

	while (connect(socketFD, (struct sockaddr *) &direccionKernel,	sizeof(struct sockaddr))<0)
		sleep(1); //Espera un segundo y se vuelve a tratar de conectar.
	EnviarHandshake(socketFD, cliente);
	RecibirElHandshake(socketFD, servidor);
	return socketFD;

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

void EnviarDatosTipo(int socketFD, char emisor[11], void* datos, int tamDatos, int tipoMensaje){
	Paquete* paquete = malloc(sizeof(Paquete));
	paquete->header.tipoMensaje = tipoMensaje;
	strcpy(paquete->header.emisor, emisor);
	uint32_t r = 0;
	if(tamDatos<=0 || datos==NULL){
		paquete->header.tamPayload = sizeof(uint32_t);
		paquete->Payload = &r;
	} else {
		paquete->header.tamPayload = tamDatos;
		paquete->Payload = datos;
	}
	EnviarPaquete(socketFD, paquete);

	free(paquete);
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

void EnviarDatos(int socketFD, char emisor[11], void* datos, int tamDatos) {
	EnviarDatosTipo(socketFD, emisor, datos, tamDatos, ESDATOS);
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
				printf("\nConectado con el servidor Memoria\n");
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
void RecibirHandshake_DeKernel(int socketFD, char emisor[11]){
	Paquete* paquete =  malloc(sizeof(Paquete));
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0 && paquete->header.tipoMensaje == ESHANDSHAKE) { //si no hubo error y es un handshake
		if (strcmp(paquete->header.emisor, emisor) == 0) {
				printf("\nConectado con el servidor Kernel\n");
				if(strcmp(paquete->header.emisor, KERNEL) == 0){
					paquete->Payload = malloc(paquete->header.tamPayload);
					resul = RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
					StackSizeEnPaginas = *((uint32_t*)paquete->Payload);
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
		exit(1);
	} else if (recibido == 0) {
		printf("Fin de Conexion en socket %d\n", socketFD);
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
	printf("\nEl programa con PID %u se inicializo en Memoria.\n", ID_Prog);
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
	Paquete paquete;
	while (RecibirPaqueteCliente(socketFD, MEMORIA, &paquete) <= 0);
	void* r;
	if(paquete.header.tipoMensaje == ESERROR)
		r = NULL;
	else if(paquete.header.tipoMensaje == ESDATOS)
		r = paquete.Payload;
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
	int tamDatos = sizeof(uint32_t) + string_length(path) + 1;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = VALIDAR_ARCHIVO;

	char* destinoPath = datos + sizeof(uint32_t);
	strcpy(destinoPath, path);

	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, FS, paquete) <= 0);
	uint32_t r = *(uint32_t*) (paquete->Payload);
	free(paquete->Payload);
	free(paquete);
	return r;
}
uint32_t FS_CrearPrograma(int socketFD, char emisor[11], char* path) {
	int tamDatos = sizeof(uint32_t) + sizeof(char) * string_length(path) + 1;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = CREAR_ARCHIVO;

	char* destinoPath = datos + sizeof(uint32_t);
	strcpy(destinoPath, path);

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
	int tamDatos = sizeof(uint32_t) + sizeof(char) * string_length(path) + 1;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = BORRAR_ARCHIVO;

	char* destinoPath = datos + sizeof(uint32_t);
	strcpy(destinoPath, path);

	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, FS, paquete) <= 0);
	uint32_t r = *(uint32_t*) (paquete->Payload);
	free(paquete->Payload);
	free(paquete);
	return r;
}
void* FS_ObtenerDatos(int socketFD, char emisor[11], char* path, uint32_t offset, uint32_t size) {
	int tamDatos = sizeof(uint32_t) * 3 + sizeof(char) * string_length(path) + 1;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = OBTENER_DATOS;
	((uint32_t*) datos)[1] = offset;
	((uint32_t*) datos)[2] = size;

	char* destinoPath = datos + sizeof(uint32_t) * 3;
	strcpy(destinoPath, path);

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
uint32_t FS_GuardarDatos(int socketFD, char emisor[11], char* path, int offset, int size, void* buffer) {//Agregado en el Fe de Erratas, responde 0 si hubo error y 1 si libero la pag.
	int tamDatos = sizeof(uint32_t) * 3 + string_length(path) + 1 + size;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = GUARDAR_DATOS;
	((uint32_t*) datos)[1] = offset;
	((uint32_t*) datos)[2] = size;

	char* destinoPath = datos + sizeof(uint32_t) *3;
	strcpy(destinoPath, path);

	void* destinoBuffer2 = datos + sizeof(uint32_t)*3 + string_length(path) + 1;
	memcpy(destinoBuffer2, buffer, size);

	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, FS, paquete) <= 0);
	uint32_t r = *(uint32_t*) (paquete->Payload);
	free(paquete->Payload);
	free(paquete);
	return r;
}

void serializar(void** pcbSerializado, void* datos, uint32_t tam,uint32_t* tamTotal){
	*tamTotal+=tam;
	*pcbSerializado = realloc(*pcbSerializado, *tamTotal);
	memcpy(*pcbSerializado + (*tamTotal-tam), datos, tam);
}

void EnviarPCB(int socketCliente, char emisor[11], BloqueControlProceso* pecebe) {

	int i = 0;
	uint32_t tamTotal = 0;
	uint32_t sizeIndStack = list_size(pecebe->IndiceDelStack);
	t_size sizeIndCod = list_size(pecebe->IndiceDeCodigo);
	//uint32_t sizeIndEtiq = dictionary_size(pecebe->IndiceDeEtiquetas);

	void* pcbSerializado = NULL;
	//void* posActual = NULL;
	IntsDelPCB ints;
	ints.etiquetas_size = pecebe->etiquetas_size;
	ints.ExitCode = pecebe->ExitCode;
	ints.PID = pecebe->PID;
	ints.ProgramCounter = pecebe->ProgramCounter;
	ints.PaginasDeCodigo = pecebe->PaginasDeCodigo;
	ints.cantidad_de_etiquetas = pecebe->cantidad_de_etiquetas;
	ints.cantidad_de_funciones = pecebe->cantidad_de_funciones;
	ints.cantidadDeRafagasAEjecutar = pecebe->cantidadDeRafagasAEjecutar;
	ints.cantidadDeRafagasEjecutadasHistorica = pecebe->cantidadDeRafagasEjecutadasHistorica;
	ints.cantidadDeRafagasEjecutadas = pecebe->cantidadDeRafagasEjecutadas;
	ints.cantidadSyscallEjecutadas = pecebe->cantidadSyscallEjecutadas;
	ints.cantTotalVariables = pecebe->cantTotalVariables;
	ints.cantidadAccionesAlocar = pecebe->cantidadAccionesAlocar;
	ints.cantidadAccionesLiberar = pecebe->cantidadAccionesLiberar;
	ints.cantBytesAlocados = pecebe->cantBytesAlocados;
	ints.cantBytesLiberados = pecebe->cantBytesLiberados;
	serializar(&pcbSerializado,&ints, sizeof(IntsDelPCB), &tamTotal);
	//Serialización IndiceDeEtiquetas
	serializar(&pcbSerializado,pecebe->IndiceDeEtiquetas, pecebe->etiquetas_size, &tamTotal);
	//Serialización IndiceDeCodigo
	serializar(&pcbSerializado,&sizeIndCod, sizeof(uint32_t), &tamTotal);
	for (i = 0; i < sizeIndCod; i++){
		RegIndiceCodigo* elem = (RegIndiceCodigo*)list_get(pecebe->IndiceDeCodigo, i);
		serializar(&pcbSerializado,elem, sizeof(RegIndiceCodigo), &tamTotal);
	}
	//Serialización IndiceDelStack
	serializar(&pcbSerializado,&sizeIndStack, sizeof(uint32_t), &tamTotal);

	for (i = 0; i < sizeIndStack; i++){
		regIndiceStack indice = *((regIndiceStack*)(list_get(pecebe->IndiceDelStack, i)));
		uint32_t sizeVariables = list_size(indice.Variables);
		uint32_t sizeArgumentos = list_size(indice.Argumentos);
		int j;

		serializar(&pcbSerializado,&indice.DireccionDeRetorno, sizeof(uint32_t), &tamTotal);
		serializar(&pcbSerializado,&indice.PosVariableDeRetorno, sizeof(PosicionDeMemoria), &tamTotal);

		serializar(&pcbSerializado,&sizeVariables, sizeof(uint32_t), &tamTotal);
		for (j = 0; j < sizeVariables; j++){
			Variable* elem = list_get(indice.Variables, j);
			serializar(&pcbSerializado,elem, sizeof(Variable), &tamTotal);
		}

		serializar(&pcbSerializado,&sizeArgumentos, sizeof(uint32_t), &tamTotal);
		for (j = 0; j < sizeArgumentos; j++){
			Variable* elem = list_get(indice.Argumentos, j);
			serializar(&pcbSerializado,elem, sizeof(Variable), &tamTotal);
		}
	}
	//Lo envio
	EnviarDatosTipo(socketCliente,emisor,pcbSerializado,tamTotal,ESPCB);
	free(pcbSerializado);
}


void deserializar(void** pcbSerializado, void* destino, uint32_t tam,uint32_t* tamTotal){
	*tamTotal+=tam;
	memcpy(destino,*pcbSerializado, tam);
	*pcbSerializado+=tam;
}

void RecibirPCB(BloqueControlProceso* pecebe, void* payload, uint32_t tamPayload,char receptor[11]){
	pcb_Create(pecebe, 0);
	//void * pcbSerializado = malloc(tamPayload);
	void *pcbSerializado = payload;
	//cargar pcb
	int i = 0;
	uint32_t sizeIndStack;
	t_size sizeIndCod;
	uint32_t tamTotal = 0;

	IntsDelPCB ints;
	deserializar(&pcbSerializado,&ints, sizeof(IntsDelPCB), &tamTotal);
	pecebe->etiquetas_size = ints.etiquetas_size;
	pecebe->ExitCode = ints.ExitCode;
	pecebe->PID = ints.PID;
	pecebe->ProgramCounter = ints.ProgramCounter;
	pecebe->PaginasDeCodigo = ints.PaginasDeCodigo;
	pecebe->cantidad_de_etiquetas = ints.cantidad_de_etiquetas;
	pecebe->cantidad_de_funciones = ints.cantidad_de_funciones;
	pecebe->cantidadDeRafagasAEjecutar = ints.cantidadDeRafagasAEjecutar;
	pecebe->cantidadDeRafagasEjecutadasHistorica = ints.cantidadDeRafagasEjecutadasHistorica;
	pecebe->cantidadDeRafagasEjecutadas = ints.cantidadDeRafagasEjecutadas;
	pecebe->cantidadSyscallEjecutadas = ints.cantidadSyscallEjecutadas;
	pecebe->cantTotalVariables = ints.cantTotalVariables;
	pecebe->cantidadAccionesAlocar = ints.cantidadAccionesAlocar;
	pecebe->cantidadAccionesLiberar = ints.cantidadAccionesLiberar;
	pecebe->cantBytesAlocados = ints.cantBytesAlocados;
	pecebe->cantBytesLiberados = ints.cantBytesLiberados;

	//deserialización IndiceDeEtiquetas
	pecebe->IndiceDeEtiquetas = malloc(pecebe->etiquetas_size);
	deserializar(&pcbSerializado,pecebe->IndiceDeEtiquetas, pecebe->etiquetas_size, &tamTotal);

	//deserialización IndiceDeCodigo
	deserializar(&pcbSerializado,&sizeIndCod, sizeof(uint32_t), &tamTotal);
	for (i = 0; i < sizeIndCod; i++){
		RegIndiceCodigo* elem = malloc(sizeof(RegIndiceCodigo));
		deserializar(&pcbSerializado,elem, sizeof(RegIndiceCodigo), &tamTotal);
		list_add(pecebe->IndiceDeCodigo, elem);
	}
	//deserialización IndiceDelStack
	deserializar(&pcbSerializado,&sizeIndStack, sizeof(uint32_t), &tamTotal);

	for (i = 0; i < sizeIndStack; i++){

		regIndiceStack* indice = malloc(sizeof(regIndiceStack));
		list_add(pecebe->IndiceDelStack, indice);
		uint32_t sizeVariables;
		uint32_t sizeArgumentos;
		int j;

		deserializar(&pcbSerializado,&(indice->DireccionDeRetorno), sizeof(uint32_t), &tamTotal);
		deserializar(&pcbSerializado,&(indice->PosVariableDeRetorno), sizeof(PosicionDeMemoria), &tamTotal);

		indice->Variables = list_create();
		deserializar(&pcbSerializado,&sizeVariables, sizeof(uint32_t), &tamTotal);
		for (j = 0; j < sizeVariables; j++){
			Variable* elem = malloc(sizeof(Variable));
			deserializar(&pcbSerializado,elem, sizeof(Variable), &tamTotal);
			list_add(indice->Variables, elem);
		}

		indice->Argumentos = list_create();
		deserializar(&pcbSerializado,&sizeArgumentos, sizeof(uint32_t), &tamTotal);
		for (j = 0; j < sizeArgumentos; j++){
			Variable* elem = malloc(sizeof(Variable));
			deserializar(&pcbSerializado,elem, sizeof(Variable), &tamTotal);
			list_add(indice->Argumentos, elem);
		}
	}
}

