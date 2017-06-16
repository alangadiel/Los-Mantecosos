#include "Threads.h"

uint32_t Hash(uint32_t x, uint32_t y){
	x--;
	int r = ((x + y) * (x + y + 1)) / 2 + y;
	while(r>MARCOS)
		r -= MARCOS;
	return r;
}
uint32_t FrameLookup(uint32_t pid, uint32_t pag){
	uint32_t frame = Hash(pid, pag);
	while(TablaDePagina[frame].PID != pid || TablaDePagina[frame].Pag != pag)
		frame++;
	return frame;
}
uint32_t cuantasPagTiene(uint32_t pid){
	uint32_t c, i;
	for(i=0; i < cantPagAsignadas; i++){
		if(TablaDePagina[i].PID == pid)
			c++;
	}
	return c;
}
void IniciarPrograma(uint32_t pid, uint32_t cantPag, int socketFD) {
	uint32_t r = 0;
	if (pid != 0 && cuantasPagTiene(pid) == 0 && cantPagAsignadas + cantPag < MARCOS) {
		//si no existe el proceso y hay lugar en la memoria
		int i;
		for (i = 0; i < cantPag; i++) {
			//lo agregamos a la tabla
			uint32_t frame = Hash(pid, i);
			while(TablaDePagina[frame].PID != 0) frame++;
			TablaDePagina[frame].PID = pid;
			TablaDePagina[frame].Pag = i;
			cantPagAsignadas++;
		}
		r=1;
	}
	EnviarDatos(socketFD, MEMORIA, &r, sizeof(uint32_t));
}

void SolicitarBytes(uint32_t pid, uint32_t numPag, uint32_t offset,	uint32_t tam, int socketFD) {
	void* datosSolicitados = NULL;
	if(numPag>0 && cuantasPagTiene(pid)>=numPag && offset+tam < MARCO_SIZE) { //valido los parametros
		uint32_t frame = FrameLookup(pid, numPag);
		datosSolicitados = ContenidoMemoria + MARCO_SIZE * frame + offset;
	}
	EnviarDatos(socketFD, MEMORIA, datosSolicitados, tam);
}
void AlmacenarBytes(Paquete paquete, int socketFD) {
	uint32_t r = 0;
	if(DATOS[2]>0 && cuantasPagTiene(DATOS[1])>=DATOS[2] && DATOS[3]+DATOS[4] < MARCO_SIZE) { //valido los parametros
		//esperar tiempo definido por arch de config
		sleep(RETARDO_MEMORIA);
		//buscar pagina
		void* pagina = ContenidoMemoria + MARCO_SIZE * FrameLookup(DATOS[1], DATOS[2]);
		//escribir en pagina
		memcpy(pagina + DATOS[3],(void*) DATOS[5], DATOS[4]);
		printf("Datos Almacenados: %*s", DATOS[4],(char*) DATOS[5]);

		//TODO: actualizar cache

		r=1;
	}
	EnviarDatos(socketFD, MEMORIA, &r, sizeof(uint32_t));
}

void AsignarPaginas(uint32_t pid, uint32_t cantPagParaAsignar, int socketFD) {
	uint32_t r = 0;
	if (cantPagAsignadas + cantPagParaAsignar < MARCOS && cuantasPagTiene(pid) > 0) {
		int i;
		for (i = cuantasPagTiene(pid); i < cantPagParaAsignar; i++) {
			//lo agregamos a la tabla
			uint32_t frame = Hash(pid, i);
			while(TablaDePagina[frame].PID != 0) frame++;
			TablaDePagina[frame].PID = pid;
			TablaDePagina[frame].Pag = i;
			cantPagAsignadas++;
		}
		r=1;
	}
	EnviarDatos(socketFD, MEMORIA, &r, sizeof(uint32_t));
}

void LiberarPaginas(uint32_t pid, uint32_t numPag, int socketFD) {
	uint32_t r = 0;
	if(cuantasPagTiene(pid) > 0) {
		cantPagAsignadas--;
		TablaDePagina[FrameLookup(pid, numPag)].PID=0;
		r=1;
	}
	EnviarDatos(socketFD, MEMORIA, &r, sizeof(uint32_t));
}

void FinalizarPrograma(uint32_t pid, int socketFD) {
	uint32_t r = 0;
	uint32_t cantPag = cuantasPagTiene(pid);
	if(cantPag > 0) {
		//TODO: join de hilo correspondiente?
		uint32_t i;
		for(i=0; i < cantPag; i++){
			TablaDePagina[FrameLookup(pid, i)].PID=0;
			cantPagAsignadas--;
		}
		r=1;
	}
	EnviarDatos(socketFD, MEMORIA, &r, sizeof(uint32_t));
}

int RecibirPaqueteMemoria (int socketFD, char receptor[11], Paquete* paquete) {
	paquete->Payload = malloc(1);
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0) { //si no hubo error
		if (paquete->header.tipoMensaje == ESHANDSHAKE) { //vemos si es un handshake y le respondemos con el tam de pag
			if( !strcmp(paquete->header.emisor, KERNEL) || !strcmp(paquete->header.emisor, CPU)){
				printf("Se establecio conexion con %s\n", paquete->header.emisor);
				Paquete paquete;
				paquete.header.tipoMensaje = ESHANDSHAKE;
				paquete.header.tamPayload = sizeof(uint32_t);
				strcpy(paquete.header.emisor, MEMORIA);
				paquete.Payload=&MARCO_SIZE;
				EnviarPaquete(socketFD, &paquete);
			}
		} else { //recibimos un payload y lo procesamos (por ej, puede mostrarlo)
			paquete->Payload = realloc(paquete->Payload, paquete->header.tamPayload);
			resul = RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
		}
	}

	return resul;
}

void* accionHilo(void* socket){
	int socketFD = *(int*)socket;
	Paquete paquete;
	while (RecibirPaqueteMemoria(socketFD, MEMORIA, &paquete) > 0) {
		if (paquete.header.tipoMensaje == ESDATOS){
			switch ((*(uint32_t*)paquete.Payload)){
			case INIC_PROG:
				IniciarPrograma(DATOS[1],DATOS[2],socketFD);
			break;
			case SOL_BYTES:
				SolicitarBytes(DATOS[1],DATOS[2],DATOS[3],DATOS[4],socketFD);
			break;
			case ALM_BYTES:
				AlmacenarBytes(paquete,socketFD);
			break;
			case ASIG_PAG:
				AsignarPaginas(DATOS[1],DATOS[2],socketFD);
			break;
			case LIBE_PAG:
				LiberarPaginas(DATOS[1],DATOS[2],socketFD);
			break;
			case FIN_PROG:
				FinalizarPrograma(DATOS[1],socketFD);
			break;
			}
		}
		free(paquete.Payload);
	}
	close(socketFD);
	return NULL;
}
