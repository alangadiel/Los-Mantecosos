/*
 * socket.c
 *
 *  Created on: 4/8/2015
 *      Author: utnso
 */
/*
 * socket.c
 *
 *  Created on: 4/8/2015
 *      Author: utnso
 */


#include "socket.h"

int realizarConexion(int socketEscucha) {
	struct sockaddr_in addr;			// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);
	int socketCliente = accept(socketEscucha, (struct sockaddr *) &addr, &addrlen);
	return socketCliente;
}

socket_t iniciarServidor(int puerto) {
	int sSocket;
		struct sockaddr_in sAddr;
		int yes = 1;

		sSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (sSocket == -1) {
			//"Error en socket"
			return -1;
		}

		sAddr.sin_family = AF_INET;
		sAddr.sin_port = htons(puerto);
		sAddr.sin_addr.s_addr = htonl(INADDR_ANY );

		memset(&(sAddr.sin_zero), '\0', 8);

		/* Para no esperar 1 minute a que el socket se libere */

		if (setsockopt(sSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1) {
			//"Error en setsockopt"
			return -1;
		}

		if (bind(sSocket, (struct sockaddr *) &sAddr, sizeof(sAddr)) == -1) {
			//"Error en bind"
			return -1;
		}

		if (listen(sSocket, 1) == -1) {
			//"Error en listen"
			return -1;
		}

		return sSocket;
}

socket_t conectarAlServidor(char *IPServidor, int PuertoServidor) {
	struct sockaddr_in Servidor;
	socket_t Socket = socket(AF_INET, SOCK_STREAM, 0);

	if (Socket == -1)
		return -1;

	Servidor.sin_family = AF_INET;
	Servidor.sin_addr.s_addr = inet_addr((char*) IPServidor);
	Servidor.sin_port = htons(PuertoServidor);
	memset(Servidor.sin_zero, '\0', sizeof(Servidor.sin_zero));

	if (connect(Socket, (struct sockaddr *) &Servidor, sizeof(struct sockaddr))
			== -1)
		return -1;

	return Socket;
}

int recibirInfo(int Socket, buffer_t Buffer, int CantidadARecibir) {
	long Recibido = 0;
	long TotalRecibido = 0;

	while (TotalRecibido < CantidadARecibir) {

		Recibido = recv(Socket, Buffer + TotalRecibido,
				CantidadARecibir - TotalRecibido, MSG_WAITALL);

		switch (Recibido) {
		case 0:
			return TotalRecibido;
			break;

		case -1:
			return -1;
			break;

		default:
			TotalRecibido += Recibido;
			break;
		}
	}

	return TotalRecibido;
}

int enviarInfo(int Socket, buffer_t Buffer, int CantidadAEnviar) {
	int Enviados = 0;
	int TotalEnviados = 0;

	while (TotalEnviados < CantidadAEnviar) {
		Enviados = send(Socket, Buffer + TotalEnviados,
				CantidadAEnviar - TotalEnviados, 0);
		switch (Enviados) {
		case 0:
			return TotalEnviados;
			break;

		case -1:
			return -1;
			break;

		default:
			TotalEnviados += Enviados;
			break;
		}
	};

	return TotalEnviados;
}

int enviarMensaje(socket_t Socket, MPS_MSG *mensaje) {
	int retorno;
	MPS_MSG_HEADER cabecera;
	unsigned char *BufferEnviar;
	int largoHeader = sizeof(cabecera.PayloadDescriptor)
			+ sizeof(cabecera.PayLoadLength);
	int largoTotal = largoHeader + mensaje->PayLoadLength;

	BufferEnviar = malloc(sizeof(unsigned char) * largoTotal);
	cabecera.PayloadDescriptor = mensaje->PayloadDescriptor;
	cabecera.PayLoadLength = mensaje->PayLoadLength;
	//Copia la cabecera al buffer.
	memcpy(BufferEnviar, &cabecera, largoHeader);
	//copia el mensaje seguido de la cabezera dentro del buffer.
	memcpy(BufferEnviar + largoHeader, mensaje->Payload,
			mensaje->PayLoadLength);

	setsockopt(Socket, SOL_SOCKET, SO_SNDBUF, &largoTotal, sizeof(int));
	retorno = enviarInfo(Socket, BufferEnviar, largoTotal);

	free(BufferEnviar);
	if (retorno < largoTotal) {
		return -1;
	};

	return 1;
}

int recibirMensaje(int Socket, MPS_MSG *mensaje) {
	int retorno;
	MPS_MSG_HEADER cabecera;
	mensaje->PayLoadLength = 0;
	int largoHeader = sizeof cabecera.PayloadDescriptor
			+ sizeof cabecera.PayLoadLength;

	// Recibo Primero el Header

	retorno = recibirInfo(Socket, (unsigned char *) &cabecera, largoHeader);
	if (retorno == 0) {
		return 0;
	};

	if (retorno < largoHeader) {
		return -1;
	};

	// Desarmo la cabecera
	mensaje->PayloadDescriptor = cabecera.PayloadDescriptor;
	mensaje->PayLoadLength = cabecera.PayLoadLength;

	// Si es necesario recibo Primero el PayLoad.
	if (mensaje->PayLoadLength != 0) {
		mensaje->Payload = malloc(mensaje->PayLoadLength + 1);
		memset(mensaje->Payload, '\0', mensaje->PayLoadLength + 1);

		retorno = recibirInfo(Socket, mensaje->Payload,
				(mensaje->PayLoadLength));
	} else {
		mensaje->Payload = NULL;
	}

	return retorno;
}




t_stream* operacion_serializer(mensajeOperacion* self) {
	char *data = malloc(strlen(self->bloque) + strlen(self->ip) + strlen(self->nodo) + strlen(self->puerto) + strlen(self->resultado) + 5);
	t_stream *stream = malloc(sizeof(t_stream));
	int offset = 0, tmp_size = 0;
	memcpy(data, self->bloque, tmp_size = strlen(self->bloque) + 1);
	offset = tmp_size;

	memcpy(data + offset, self->ip, tmp_size = strlen(self->ip) + 1);
	offset += tmp_size;

	memcpy(data + offset, self->nodo, tmp_size = strlen(self->nodo) + 1);
	offset += tmp_size;

	memcpy(data + offset, self->puerto, tmp_size = strlen(self->puerto) + 1);
	offset += tmp_size;

	memcpy(data + offset, self->resultado, tmp_size = strlen(self->resultado) + 1);
	offset += tmp_size;

	stream->length = offset;
	stream->data = data;

	return stream;
}





t_rutina* rutina_desserializer(char *stream) {
	t_rutina* self = malloc(sizeof(t_rutina));
	int offset = 0, tmp_size = 0;

	for (tmp_size = 1; (stream)[tmp_size - 1] != '\0'; tmp_size++);
	self->archivos = malloc(tmp_size);
	memcpy(self->archivos, stream, tmp_size);
	offset = tmp_size;

	for (tmp_size = 1; (stream+ offset)[tmp_size - 1] != '\0';tmp_size++);
	self->combiner = malloc(tmp_size);
	memcpy(self->combiner, stream + offset, tmp_size);
	offset += tmp_size;

	for (tmp_size = 1; (stream+ offset)[tmp_size - 1] != '\0';tmp_size++);
	self->map = malloc(tmp_size);
	memcpy(self->map, stream + offset, tmp_size);
	offset += tmp_size;

	for (tmp_size = 1; (stream+ offset)[tmp_size - 1] != '\0';tmp_size++);
	self->reduce = malloc(tmp_size);
	memcpy(self->reduce, stream + offset, tmp_size);
	offset += tmp_size;

	for (tmp_size = 1; (stream+ offset)[tmp_size - 1] != '\0';tmp_size++);
	self->resultado = malloc(tmp_size);
	memcpy(self->resultado, stream + offset, tmp_size);

	return self;
}


t_resultadoMapper* resultadoMapper_desserializer(char *stream) {
	t_resultadoMapper* self = malloc(sizeof(t_resultadoMapper));
	int offset = 0, tmp_size = 0;

	for (tmp_size = 1; (stream)[tmp_size - 1] != '\0'; tmp_size++);
	self->bloque = malloc(tmp_size);
	memcpy(self->bloque, stream, tmp_size);
	offset = tmp_size;

	for (tmp_size = 1; (stream+ offset)[tmp_size - 1] != '\0';tmp_size++);
	self->nodo = malloc(tmp_size);
	memcpy(self->nodo, stream + offset, tmp_size);
	offset += tmp_size;

	return self;
}

t_out* out_desserializer(char *stream) {
	t_out* self = malloc(sizeof(t_out));
	int offset = 0, tmp_size = 0;

	for (tmp_size = 1; (stream)[tmp_size - 1] != '\0'; tmp_size++);
	self->nodo= malloc(tmp_size);
	memcpy(self->nodo, stream, tmp_size);
	offset = tmp_size;
	return self;
}


void armarMensaje(MPS_MSG* mensaje,int descriptor,int payloadLength,void* payload){
	mensaje->PayloadDescriptor=descriptor;
	mensaje->PayLoadLength = payloadLength;
	mensaje->Payload = payload;
}

char* obtenerIpLocal()
{
    int fd;
    struct ifreq ifr;
    char iface[] = "eth0";

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);
    return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}


