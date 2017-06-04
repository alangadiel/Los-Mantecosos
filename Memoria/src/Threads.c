#include "Threads.h"

uint32_t Hash(uint32_t pid, uint32_t pag){

}
unsigned cuantasPagTiene(uint32_t pid){
	unsigned c, i;
	for(i=0; i <= cantPagAsignadas; i++){
		if(TablaDePagina[i].PID == pid)
			c++;
	}
	return c;
}

void IniciarPrograma(uint32_t pid, uint32_t cantPag, int socketFD) {
	if (cuantasPagTiene(pid) == 0)//no existe el proceso
		AsignarPaginas(pid, cantPag, socketFD);
	else
		EnviarDatos(socketFD, MEMORIA, 0, sizeof(uint32_t)); //hubo un error porque el proceso ya existe
}

void SolicitarBytes(uint32_t pid, uint32_t numPag, uint32_t offset,
		uint32_t tam, int socketFD) {

}
void AlmacenarBytes(Paquete paquete, int socketFD) {
	//esperar tiempo definido por arch de config
	sleep(RETARDO_MEMORIA);
	//buscar pagina
	void* pagina = ContenidoMemoria + MARCO_SIZE * Hash(DATOS[1], DATOS[2]);
	//escribir en pagina
	memcpy(pagina + DATOS[3],(void*) DATOS[5], DATOS[4]);
	printf("Datos Almacenados: %*s", DATOS[4],(char*) DATOS[5]);
	//TODO: actualizar cache

}

void AsignarPaginas(uint32_t pid, uint32_t cantPagParaAsignar, int socketFD) {
	int r = 0;
	if (cantPagAsignadas + cantPagParaAsignar < MARCOS && cuantasPagTiene(pid) > 0) {
		int i;
		for (i = cuantasPagTiene(pid); i < cantPagParaAsignar; i++) {
			//lo agregamos a la tabla
			TablaDePagina[Hash(pid, i)].PID = pid;
			TablaDePagina[Hash(pid, i)].Pag = i;
			cantPagAsignadas++;
		}
		r=1;
	}
	EnviarDatos(socketFD, MEMORIA, &r, sizeof(uint32_t));
}

void LiberarPaginas(uint32_t pid, uint32_t numPag, int socketFD) {
	if(cuantasPagTiene(pid) > 0) {
	cantPagAsignadas -= numPag;
	//TODO
	} else
		EnviarDatos(socketFD, MEMORIA, 0, sizeof(uint32_t)); //hubo un error porque el proceso no existe
/*Ante un pedido de liberación de página por parte del kernel, el proceso memoria deberá liberar
  la página que corresponde con el número solicitado. En caso de que dicha página no exista
  o no pueda ser liberada, se deberá informar de la imposibilidad de realizar dicha operación
  como una excepcion de memoria.
 */
}

void FinalizarPrograma(uint32_t pid, int socketFD) {
	if(cuantasPagTiene(pid) > 0) {
	//join de hilo correspondiente TODO
	//list_remove_and_destroy_by_condition(procesosActivos, LAMBDA(bool _(void* pidAEliminar) { return *(uint32_t*)pidAEliminar != pid;}), free);
	} else
		EnviarDatos(socketFD, MEMORIA, 0, sizeof(uint32_t)); //hubo un error porque el proceso no existe
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
