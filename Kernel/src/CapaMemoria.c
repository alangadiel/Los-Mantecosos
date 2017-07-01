#include "CapaMemoria.h"

uint32_t ActualizarMetadata(uint32_t PID, uint32_t nroPagina, uint32_t cantAReservar, int socketFD)
{
	uint32_t cantTotal = cantAReservar + sizeof(HeapMetadata);
	//Obtengo la pagina en cuestion donde actualizar la metadata
	void* datosPagina = IM_LeerDatos(socketFD, KERNEL, PID, nroPagina, 0, TamanioPagina);
	if(datosPagina!=NULL){
		uint32_t offset = 0;
		bool estado;
		uint32_t sizeBloque;
		bool encontroLibre = false;
		//uint32_t punteroAlPrimerBloqueDisponible;

		/*int offsetOcupado = RecorrerHastaEncontrarUnMetadataUsed(datosPagina);
		if(offsetOcupado<0) //HUBO ERROR
			punteroAlPrimerBloqueDisponible = -1;
		else
			punteroAlPrimerBloqueDisponible= offsetOcupado + sizeof(HeapMetadata);*/

		//Recorro hasta encontrar el primer bloque libre
		while(offset < TamanioPagina - sizeof(HeapMetadata) && encontroLibre == false)
		{
			//Recorro el buffer obtenido
			sizeBloque = *(uint32_t*)datosPagina + offset;
			estado = *(bool*)(datosPagina + offset + sizeof(uint32_t));

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

		if(encontroLibre == true)
		{
			// Se encontro un bloque libre
			uint32_t diferencia= sizeBloque - cantTotal;

			//Actualizo el metadata de acuerdo a la cantidad de bytes a reservar
			HeapMetadata metaOcupado;
			metaOcupado.isFree = false;
			metaOcupado.size = cantAReservar;

			if(IM_GuardarDatos(socketFD, KERNEL, PID, nroPagina, offset, sizeof(HeapMetadata), &metaOcupado)==false){
				FinalizarPrograma(PID,EXCEPCIONDEMEMORIA);
				return -1;
			}


			//Creo el metadata para lo que queda libre del espacio que use
			uint32_t offsetMetadataLibre = offset + sizeof(HeapMetadata) + cantAReservar;

			HeapMetadata metaLibre;
			metaLibre.isFree = true;
			metaLibre.size = diferencia;

			IM_GuardarDatos(socketFD, KERNEL, PID, nroPagina, offsetMetadataLibre, sizeof(HeapMetadata), &metaLibre);

			return offset + sizeof(HeapMetadata);
		}
		else
		{
			//No se encontro ningun bloque donde reservar memoria dinamica
			return -1;
		}
	}
	else{
		FinalizarPrograma(PID,EXCEPCIONDEMEMORIA);
		return -1;
	}

}


uint32_t SolicitarHeap(uint32_t PID,uint32_t cantAReservar,int socket){
	uint32_t cantTotal = cantAReservar+sizeof(HeapMetadata);
	uint32_t punteroAlPrimerDisponible=0;
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
			paginaObtenida->espacioDisponible -= cantTotal;
			//Obtengo la pagina en cuestion y actualizo el metadata
			punteroAlPrimerDisponible = ActualizarMetadata(PID,paginaObtenida->nroPagina,cantTotal,socket);

		}
		else  		//No hay una pagina del proceso utilizable
		{
			//Obtengo el ultimo numero de pagina, de ese PID
			pthread_mutex_lock(&mutexPaginasPorProceso);
			t_list* pagesProcess= list_filter(PaginasPorProceso,
					LAMBDA(bool _(void* item) {return ((PaginaDelProceso*)item)->pid == PID;}));
			pthread_mutex_unlock(&mutexPaginasPorProceso);
			int i=0;
			int maximoNroPag=0;
			pthread_mutex_lock(&mutexPaginasPorProceso);
			for (i = 0; i < list_size(pagesProcess); i++) {
				PaginaDelProceso* elem = (PaginaDelProceso*)list_get(PaginasPorProceso,i);
				if(maximoNroPag< elem->nroPagina)	maximoNroPag = elem->nroPagina;
			}
			pthread_mutex_unlock(&mutexPaginasPorProceso);
			PaginaDelProceso nuevaPPP;
			nuevaPPP.nroPagina = maximoNroPag+1;
			nuevaPPP.espacioDisponible = TamanioPagina;
			nuevaPPP.pid = PID;
			pthread_mutex_lock(&mutexPaginasPorProceso);
			list_add(PaginasPorProceso,&nuevaPPP);
			pthread_mutex_unlock(&mutexPaginasPorProceso);
			//Le pido al Proceso Memoria que me guarde esta pagina para el proceso en cuestion
			bool resultado = IM_AsignarPaginas(socket,KERNEL,PID,1);
			if(resultado==true){
				punteroAlPrimerDisponible = ActualizarMetadata(PID,nuevaPPP.nroPagina,cantTotal,socket);
			}
			else{
				FinalizarPrograma(PID,NOSEPUEDENASIGNARMASPAGINAS);
			}
			//Destruyo la lista PagesProcess
			list_destroy_and_destroy_elements(pagesProcess,free);
		}
	}
	else{ //Debe finalizar el programa pq quiere reservar mas de lo permitido
		FinalizarPrograma(PID,SOLICITUDMASGRANDEQUETAMANIOPAGINA);
		//TODO: Avisar a la CPU del programa finalizado
		// EL TP DICE QUE HAY QUE PEDIR OTRA PAGINA
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


void SolicitudLiberacionDeBloque(int socketFD,uint32_t pid,PosicionDeMemoria pos)
{
	void* datosPagina = IM_LeerDatos(socketFD,KERNEL,pid,pos.NumeroDePagina,0,TamanioPagina);
	uint32_t offSetMetadataAActualizar = pos.Offset- sizeof(HeapMetadata);
	uint32_t sizeBloqueALiberar = *(uint32_t*)(datosPagina+offSetMetadataAActualizar);
	HeapMetadata heapMetaAActualizar;
	heapMetaAActualizar.isFree = true;
	heapMetaAActualizar.size = sizeBloqueALiberar;
	//Pongo el bloque como liberado
	//Me fijo el estado del siguiente Metadata(si esta Free o Used)
	uint32_t offsetMetadataSiguiente = pos.Offset+sizeBloqueALiberar;
	HeapMetadata heapMetedataSiguiente =*(HeapMetadata*)(datosPagina+offsetMetadataSiguiente);
	bool resultado;

	if(heapMetedataSiguiente.isFree==true) {
		//Si esta ocupado: solo actualizo el metadata del bloque que me liberaron
		//Si esta libre: puedo compactarlos como un metadata solo
		heapMetaAActualizar.size += heapMetedataSiguiente.size;
	}
	resultado = IM_GuardarDatos(socketFD,KERNEL,pid,pos.NumeroDePagina,offSetMetadataAActualizar,sizeof(HeapMetadata),&heapMetaAActualizar);
	if(resultado==true){
		//Si estan todos los bloques de la pagina libres, hay que liberar la pagina entera
		bool hayAlgunBloqueUsado= RecorrerHastaEncontrarUnMetadataUsed(datosPagina);
		if(hayAlgunBloqueUsado==false){
			//No se encontro algun bloque ocupado: Hay que liberar la pagina
			if(IM_LiberarPagina(socketFD,KERNEL,pid,pos.NumeroDePagina)==false){
				FinalizarPrograma(pid,EXCEPCIONDEMEMORIA);
			}
			else {
				BloqueControlProceso* PCB = buscarProcesoEnColas(pid);

				if(PCB != NULL)
				{
					PCB->cantBytesLiberados += sizeBloqueALiberar;
				}
			}
		}
	}
	else{
		FinalizarPrograma(pid,EXCEPCIONDEMEMORIA);
	}

}

int RecorrerHastaEncontrarUnMetadataUsed(void* datosPagina)
{
	bool encontroOcupado=false;
	uint32_t offsetOcupado=0;
	//Recorro hasta encontrar el primer bloque ocupado
		while(offsetOcupado<TamanioPagina-sizeof(HeapMetadata) && encontroOcupado == false){
			//Recorro el buffer obtenido
			HeapMetadata heapMD = *(HeapMetadata*)(datosPagina + offsetOcupado);
			if(heapMD.isFree==false){
				//Si encuentra un metadata free, freno
				encontroOcupado = true;
			}
			else{
				//Aumento el puntero de acuerdo al tamaño correspondiente al bloque existente
				offsetOcupado+=(sizeof(HeapMetadata)+ heapMD.size);
			}
		}
	return encontroOcupado;
}
