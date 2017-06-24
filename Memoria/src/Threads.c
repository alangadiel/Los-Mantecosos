#include "Threads.h"

uint32_t Hash(uint32_t x, uint32_t y){
	x--;
	uint32_t r = ((x + y) * (x + y + 1)) / 2 + y;
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

bool estaEnCache(uint32_t pid, uint32_t numPag){
	pthread_mutex_lock( &mutexTablaCache );
	bool esta = list_any_satisfy(tablaCache, LAMBDA(bool _(void* elem) {
		return ((entradaCache*)elem)->PID==pid && ((entradaCache*)elem)->Pag==numPag; }));
	if (esta)
	{//llevar al final de la pila
		entradaCache* entrada = list_remove_by_condition(tablaCache, LAMBDA(bool _(void* elem) {
			return ((entradaCache*)elem)->PID==pid && ((entradaCache*)elem)->Pag==numPag; }));
		list_add(tablaCache, entrada);
		pthread_mutex_unlock( &mutexTablaCache );
		return true;
	}
	else {
		pthread_mutex_unlock( &mutexTablaCache );
		return false;
	}

}
void agregarACache(uint32_t pid, uint32_t numPag){
	entradaCache* entrada = malloc(sizeof(entradaCache));
	entrada->PID=pid;
	entrada->Pag=numPag;
	entrada->Contenido=FrameLookup(pid, numPag);

	pthread_mutex_lock( &mutexTablaCache );
	int cantPagProc = list_count_satisfying(tablaCache, LAMBDA(bool _(void* elem) {
		return ((entradaCache*)elem)->PID==pid; }));
	if(cantPagProc >= CACHE_X_PROC){

		list_remove_and_destroy_by_condition(tablaCache, LAMBDA(bool _(void* elem) {
			return ((entradaCache*)elem)->PID==pid; }), free);

	}
	else if(list_size(tablaCache) >= ENTRADAS_CACHE)
		list_remove_and_destroy_element(tablaCache, 0, free);

	list_add(tablaCache, entrada);
	pthread_mutex_unlock( &mutexTablaCache );
}

void IniciarPrograma(uint32_t pid, uint32_t cantPag, int socketFD) {

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
		EnviarDatos(socketFD, MEMORIA, NULL, 0);
	} else
		EnviarDatosTipo(socketFD, MEMORIA, NULL, 0, ESERROR);
}

void SolicitarBytes(uint32_t pid, uint32_t numPag, uint32_t offset,	uint32_t tam, int socketFD) {
	//valido los parametros
	if(numPag>0 && cuantasPagTiene(pid)>=numPag && offset+tam < MARCO_SIZE) {
		//si no esta en chache, esperar tiempo definido por arch de config
		if (!estaEnCache(pid, numPag)) {
			sleep(RETARDO_MEMORIA);
			agregarACache(pid, numPag);
		}
		//buscar pagina
		uint32_t frame = FrameLookup(pid, numPag);
		void*  datosSolicitados = ContenidoMemoria + MARCO_SIZE * frame + offset;
		EnviarDatos(socketFD, MEMORIA, datosSolicitados, tam);
	} else
		EnviarDatosTipo(socketFD, MEMORIA, NULL, 0, ESERROR);
}
void AlmacenarBytes(Paquete paquete, int socketFD) {
	//Parámetros: PID, #página, offset, tamaño y buffer.

	if(DATOS[2]>0 && cuantasPagTiene(DATOS[1])>=DATOS[2] && DATOS[3]+DATOS[4] < MARCO_SIZE) { //valido los parametros
		//esperar tiempo definido por arch de config
		sleep(RETARDO_MEMORIA);
		//buscar pagina
		void* pagina = ContenidoMemoria + MARCO_SIZE * FrameLookup(DATOS[1], DATOS[2]);
		//escribir en pagina
		memcpy(pagina + DATOS[3],(void*) DATOS[5], DATOS[4]);
		printf("Datos Almacenados: %*s", DATOS[4],(char*) DATOS[5]);
		//actualizar cache
		if (!estaEnCache(DATOS[1], DATOS[2])) {
			agregarACache(DATOS[1], DATOS[2]);
		}
		EnviarDatos(socketFD, MEMORIA, NULL, 0);
	} else
		EnviarDatosTipo(socketFD, MEMORIA, NULL, 0, ESERROR);
}

void AsignarPaginas(uint32_t pid, uint32_t cantPagParaAsignar, int socketFD) {

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
		EnviarDatos(socketFD, MEMORIA, NULL, 0);
	} else
		EnviarDatosTipo(socketFD, MEMORIA, NULL, 0, ESERROR);
}

void LiberarPaginas(uint32_t pid, uint32_t numPag, int socketFD) {

	if(cuantasPagTiene(pid) > 0) {
		cantPagAsignadas--;
		TablaDePagina[FrameLookup(pid, numPag)].PID=0;
		//sacar de cache.
		list_remove_and_destroy_by_condition(tablaCache, LAMBDA(bool _(void* elem) {
				return ((entradaCache*)elem)->PID==pid && ((entradaCache*)elem)->Pag==numPag; }), free);

		EnviarDatos(socketFD, MEMORIA, NULL, 0);
	} else
		EnviarDatosTipo(socketFD, MEMORIA, NULL, 0, ESERROR);
}

void FinalizarPrograma(uint32_t pid, int socketFD) {

	uint32_t cantPag = cuantasPagTiene(pid);
	if(cantPag > 0) {
		//TODO: join de hilo correspondiente?
		uint32_t i;
		for(i=0; i < cantPag; i++){
			TablaDePagina[FrameLookup(pid, i)].PID=0;
			cantPagAsignadas--;
			//sacar de cache.
			list_remove_and_destroy_by_condition(tablaCache, LAMBDA(bool _(void* elem) { return
				 ((entradaCache*)elem)->PID==pid && ((entradaCache*)elem)->Pag==i; }), free);
		}
		EnviarDatos(socketFD, MEMORIA, NULL, 0);
	} else
		EnviarDatosTipo(socketFD, MEMORIA, NULL, 0, ESERROR);
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

void accion(void* socket){
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
}
