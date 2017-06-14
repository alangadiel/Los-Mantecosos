/*
 * CapaMemoria.h
 *
 *  Created on: 13/6/2017
 *      Author: utnso
 */

#ifndef CAPAMEMORIA_H_
#define CAPAMEMORIA_H_
#include "Kernel.h"
#include "SocketsL.h"

uint32_t ActualizarMetadata(uint32_t PID, uint32_t nroPagina, uint32_t cantAReservar, int socketFD);
uint32_t SolicitarHeap(uint32_t PID,uint32_t cantAReservar,int socket);
void SolicitudLiberacionDeBloque(int socketFD,uint32_t pid,PosicionDeMemoria pos);



#endif /* CAPAMEMORIA_H_ */
