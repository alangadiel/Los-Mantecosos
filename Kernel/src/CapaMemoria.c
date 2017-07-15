#include "CapaMemoria.h"

uint32_t ActualizarMetadata(uint32_t PID, uint32_t nroPagina, uint32_t cantAReservar,int32_t* tipoError)
{
	uint32_t cantTotal = cantAReservar + sizeof(HeapMetadata);
	//Obtengo la pagina en cuestion donde actualizar la metadata
	void* datosPagina = IM_LeerDatos(socketConMemoria, KERNEL, PID, nroPagina, 0, TamanioPagina);
	if(datosPagina!=NULL){
		uint32_t offset = 0;
		bool estado;
		uint32_t sizeBloque;
		bool encontroLibre = false;

		//Recorro hasta encontrar el primer bloque libre
		while(offset < TamanioPagina - sizeof(HeapMetadata) && encontroLibre == false)
		{
			//Recorro el buffer obtenido
			/*sizeBloque = *(uint32_t*)(datosPagina + offset);
			estado = *(bool*)(datosPagina + offset + sizeof(uint32_t));*/
			HeapMetadata* heapMD = datosPagina+offset;
			sizeBloque = heapMD->size;
			estado = heapMD->isFree;
			if(estado == true)
			{
				//Si encuentra un metadata free, freno
				encontroLibre = true;
			}
			else
			{
				//Aumento el puntero de acuerdo al tamaño correspondiente al bloque existente
				offset += (sizeof(HeapMetadata) + sizeBloque);
			}

		}
		free(datosPagina);
		if(encontroLibre == true)
		{
			// Se encontro un bloque libre
			uint32_t diferencia= sizeBloque - cantTotal;
			//Actualizo el metadata de acuerdo a la cantidad de bytes a reservar
			HeapMetadata metaOcupado;
			metaOcupado.isFree = false;
			metaOcupado.size = cantAReservar;
			printf("Cant que quiero alocar: %u\n",metaOcupado.size);
			if(IM_GuardarDatos(socketConMemoria, KERNEL, PID, nroPagina, offset, sizeof(HeapMetadata), &metaOcupado)==false){
				perror("hubo error\n");
				*tipoError = EXCEPCIONDEMEMORIA;
				return -1;
			}
			//Creo el metadata para lo que queda libre del espacio que use
			uint32_t offsetMetadataLibre = offset + sizeof(HeapMetadata) + cantAReservar;
			HeapMetadata metaLibre;
			metaLibre.isFree = true;
			metaLibre.size = diferencia;
			if(IM_GuardarDatos(socketConMemoria, KERNEL, PID, nroPagina, offsetMetadataLibre, sizeof(HeapMetadata), &metaLibre)==false){
				perror("hubo error\n");
				*tipoError = EXCEPCIONDEMEMORIA;
				return -1;
			}

			uint32_t punteroADevolver = nroPagina*TamanioPagina+( offset + sizeof(HeapMetadata));
			printf("En el puntero %u se alocaron %u bytes\n ",punteroADevolver,cantAReservar);
			return punteroADevolver;
		}
		else
		{
			printf("No se encontro ningun bloque donde reservar memoria dinamica\n");
			*tipoError = NOSEPUDIERONRESERVARRECURSOS;
			return -1;
		}
	}
	else{
		*tipoError = EXCEPCIONDEMEMORIA;
		return -1;
	}

}


uint32_t SolicitarHeap(uint32_t PID,uint32_t cantAReservar,int32_t *tipoError){
	*tipoError = 0;
	uint32_t cantTotal = cantAReservar+sizeof(HeapMetadata);
	uint32_t punteroAlPrimerDisponible=0;
	pthread_mutex_lock(&mutexQueueEjecutando);
	BloqueControlProceso* pcb= list_find(Ejecutando->elements,
					LAMBDA(bool _(void* item) {return ((BloqueControlProceso*)item)->PID == PID;}));
	pthread_mutex_unlock(&mutexQueueEjecutando);
	//Como maximo, podes solicitar reservar: TamañoPagina -10(correspondiente a los metedatas iniciales)
	if(cantTotal <= TamanioPagina-sizeof(HeapMetadata)*2){
		void* result = NULL;
		//Busco en las paginas asignadas a ese proceso, a ver si hay alguna con tamaño suficiente
		pthread_mutex_lock(&mutexPaginasPorProceso);
		result = list_find(PaginasPorProceso,
				LAMBDA(bool _(void* item) {
			return ((PaginaDelProceso*)item)->pid == PID && ((PaginaDelProceso*)item)->espacioDisponible >= cantTotal;
		}));

		pthread_mutex_unlock(&mutexPaginasPorProceso);
		if(result!=NULL){ 		//Se encontro la pagina con tamaño disponible
			//Actualizar el tamaño disponible
			PaginaDelProceso* paginaObtenida = (PaginaDelProceso*)result;
			printf("pagina obtenida: %u\n",paginaObtenida->nroPagina);

			//Obtengo la pagina en cuestion y actualizo el metadata
			punteroAlPrimerDisponible = ActualizarMetadata(PID,paginaObtenida->nroPagina,cantAReservar,tipoError);
			paginaObtenida->espacioDisponible -= cantTotal;
			printf("Espacio que quedo disponible: %i\n",paginaObtenida->espacioDisponible);


		}
		else  		//No hay una pagina del proceso utilizable
		{
			//Obtengo el ultimo numero de pagina, de ese PID
			pthread_mutex_lock(&mutexPaginasPorProceso);
			t_list* pagesProcess= list_filter(PaginasPorProceso,
					LAMBDA(bool _(void* item) {return ((PaginaDelProceso*)item)->pid == PID;}));
			pthread_mutex_unlock(&mutexPaginasPorProceso);
			int maximoNroPag = 0;
			maximoNroPag = list_size(pagesProcess);
			PaginaDelProceso* nuevaPPP = malloc(sizeof(PaginaDelProceso));
			nuevaPPP->nroPagina = maximoNroPag+pcb->PaginasDeCodigo+STACK_SIZE;
			nuevaPPP->pid = PID;
			nuevaPPP->espacioDisponible=TamanioPagina;

			printf("Pagina nueva: %u\n",nuevaPPP->nroPagina);
			pthread_mutex_lock(&mutexPaginasPorProceso);
			list_add(PaginasPorProceso,nuevaPPP);
			pthread_mutex_unlock(&mutexPaginasPorProceso);
			//Le pido al Proceso Memoria que me guarde esta pagina para el proceso en cuestion
			bool resultado = IM_AsignarPaginas(socketConMemoria,KERNEL,PID,1);
			if(resultado==true){
				HeapMetadata metaInicial;
				metaInicial.isFree = true;
				metaInicial.size = TamanioPagina-sizeof(HeapMetadata);
				nuevaPPP->espacioDisponible -= sizeof(HeapMetadata);
				printf("espacio que quedo disponible: %i\n",nuevaPPP->espacioDisponible);

				if(IM_GuardarDatos(socketConMemoria, KERNEL, PID, nuevaPPP->nroPagina, 0, sizeof(HeapMetadata), &metaInicial)==false){
					*tipoError = EXCEPCIONDEMEMORIA;
					return -1;
				}

				nuevaPPP->espacioDisponible -= cantTotal;
				printf("Espacio que quedo disponible: %i\n",nuevaPPP->espacioDisponible);

				punteroAlPrimerDisponible = ActualizarMetadata(PID,nuevaPPP->nroPagina,cantAReservar,tipoError);
			}
			else{
				*tipoError = NOSEPUEDENASIGNARMASPAGINAS;

			}
			//Destruyo la lista PagesProcess
			list_destroy(pagesProcess);
		}
	}
	else{ //Debe finalizar el programa pq quiere reservar mas de lo permitido
		*tipoError = SOLICITUDMASGRANDEQUETAMANIOPAGINA;
	}
	return punteroAlPrimerDisponible;
}

BloqueControlProceso* buscarProcesoEnColas(uint32_t pid)
{
	BloqueControlProceso* pcb = (BloqueControlProceso*)list_find(Nuevos->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));

	if (pcb != NULL)
	{
		return pcb;
	}

	pcb = (BloqueControlProceso*)list_find(Ejecutando->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));

	if (pcb != NULL)
	{
		return pcb;
	}

	pcb = (BloqueControlProceso*)list_find(Bloqueados->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));

	if (pcb != NULL)
	{
		return pcb;
	}

	pcb = (BloqueControlProceso*)list_find(Listos->elements, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid ;}));

	if (pcb != NULL)
	{
		return pcb;
	}

	return NULL;
}
uint32_t ObtenerOffSetMetadataAnterior(void* datosPagina,uint32_t desplazamiento){
	HeapMetadata* heapALiberar = datosPagina + desplazamiento - sizeof(HeapMetadata);
	HeapMetadata* heapAnterior;
	HeapMetadata* heap;
	uint32_t offsetAnterior = 0;
	//if (heapALiberar->size!=TamanioPagina- sizeof(HeapMetadata)) //si no es el unico heap
	//HeapMetadata* heapAnterior = heapALiberar;
	HeapMetadata* PrimerHeap = datosPagina;
	uint32_t offset = PrimerHeap->size + sizeof(HeapMetadata);
	while(offset<TamanioPagina-sizeof(HeapMetadata)){
		//Recorro el HeapMetadata obtenido
		 heapAnterior = datosPagina + offsetAnterior;
		 heap = datosPagina + offset;
		//Lo leo
		if(heap==heapALiberar) break;
		//Aumento el puntero de acuerdo al tamaño correspondiente al bloque existente
		offset+=(sizeof(HeapMetadata)+ heap->size);
		offsetAnterior+=(sizeof(HeapMetadata)+ heapAnterior->size);
	}
	if(offset==desplazamiento-sizeof(HeapMetadata))
		return offsetAnterior;
	else
		return TamanioPagina;
}

void SolicitudLiberacionDeBloque(uint32_t pid,uint32_t punteroALiberar,int32_t* tipoError)
{
	printf("\nPuntero a liberar %u\n",punteroALiberar);
	uint32_t nropag = punteroALiberar / TamanioPagina;
	printf("Nro pag: %u\n",nropag);
	bool buscarPag(void* elem){
			PaginaDelProceso* pag = elem;
			return pag->nroPagina==nropag && pag->pid==pid;
	}
	PaginaDelProceso* pagEncontrada = list_find(PaginasPorProceso, buscarPag);

	uint32_t desplazamiento = punteroALiberar % TamanioPagina;

	void* datosPagina = IM_LeerDatos(socketConMemoria,KERNEL,pid,nropag,0,TamanioPagina);
	uint32_t offSetMetadataAActualizar = desplazamiento- sizeof(HeapMetadata);
	//uint32_t sizeBloqueALiberar = *(uint32_t*)(datosPagina+offSetMetadataAActualizar);
	HeapMetadata* heapMetaAActualizar = datosPagina + offSetMetadataAActualizar;
	printf("size bloque a liberar: %u\n",heapMetaAActualizar->size);
	HeapMetadata* heapMetedataAnterior = NULL;
	uint32_t obtenerOffsetMetadataAnerior = ObtenerOffSetMetadataAnterior(datosPagina, desplazamiento);
	if(obtenerOffsetMetadataAnerior<TamanioPagina){
		heapMetedataAnterior = datosPagina + obtenerOffsetMetadataAnerior;
	}


	//Me fijo el estado del siguiente Metadata(si esta Free o Used)
	uint32_t offsetMetadataSiguiente = desplazamiento + heapMetaAActualizar->size;
	HeapMetadata* heapMetedataSiguiente =datosPagina+offsetMetadataSiguiente;
	printf("offsetMetadataSiguiente: %u\n", offsetMetadataSiguiente);

	/*if(heapMetedataSiguiente!=NULL)
		printf("Size bloque siguiente %u\n",heapMetedataSiguiente->size);*/
	bool resultado;
	printf("heapMetaAActualizar->size: %u\n", heapMetaAActualizar->size);
	if(heapMetedataAnterior != NULL && heapMetedataAnterior->isFree==true) {
		printf("heapMetedataAnterior->size: %u\n", heapMetedataAnterior->size);
		//Si esta ocupado: solo actualizo el metadata del bloque que me liberaron
		//Si esta libre: puedo compactarlos como un metadata solo
		heapMetedataAnterior->size += heapMetaAActualizar->size + sizeof(HeapMetadata);
		heapMetedataAnterior->isFree=true;
		resultado = IM_GuardarDatos(socketConMemoria,KERNEL,pid,nropag,obtenerOffsetMetadataAnerior,sizeof(HeapMetadata),heapMetedataAnterior);
	}
	else if(heapMetedataSiguiente->isFree==true)
	{
		//Si esta ocupado: solo actualizo el metadata del bloque que me liberaron
		//Si esta libre: puedo compactarlos como un metadata solo
		heapMetaAActualizar->size += heapMetedataSiguiente->size + sizeof(HeapMetadata);
		heapMetaAActualizar->isFree=true;
		resultado = IM_GuardarDatos(socketConMemoria,KERNEL,pid,nropag,offSetMetadataAActualizar,sizeof(HeapMetadata),heapMetaAActualizar);

	}
	else
	{
		heapMetaAActualizar->isFree=true;
		resultado = IM_GuardarDatos(socketConMemoria,KERNEL,pid,nropag,offSetMetadataAActualizar,sizeof(HeapMetadata),heapMetaAActualizar);
	}

	if(resultado==true){
		//Si estan todos los bloques de la pagina libres, hay que liberar la pagina entera
		bool hayAlgunBloqueUsado= RecorrerHastaEncontrarUnMetadataUsed(datosPagina);
		if(hayAlgunBloqueUsado==false){
			//No se encontro algun bloque ocupado: Hay que liberar la pagina
			if(IM_LiberarPagina(socketConMemoria,KERNEL,pid,nropag)==false){
				*tipoError = EXCEPCIONDEMEMORIA;
			}
			else {
				BloqueControlProceso* PCB = buscarProcesoEnColas(pid);

				if(PCB != NULL)
				{
					PCB->cantBytesLiberados += heapMetaAActualizar->size;
				}
			}
		}
		pagEncontrada->espacioDisponible += heapMetaAActualizar->size;
		printf("\nLos bloques de memoria dinámica se liberaron correctamente\n");

	}
	else{
		*tipoError = EXCEPCIONDEMEMORIA;
	}

}

bool RecorrerHastaEncontrarUnMetadataUsed(void* datosPagina)
{
	bool encontroOcupado=false;
	uint32_t offsetOcupado=0;
	//Recorro hasta encontrar el primer bloque ocupado
		while(offsetOcupado<TamanioPagina-sizeof(HeapMetadata) && encontroOcupado == false){
			//Recorro el buffer obtenido
			HeapMetadata* heapMD = datosPagina + offsetOcupado;
			if(heapMD->isFree==false){
				//Si encuentra un metadata free, freno
				encontroOcupado = true;
			}
			else{
				//Aumento el puntero de acuerdo al tamaño correspondiente al bloque existente
				offsetOcupado+=(sizeof(HeapMetadata)+ heapMD->size);
			}
		}
	return encontroOcupado;
}
