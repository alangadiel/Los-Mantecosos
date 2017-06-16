/*
 * CapaFS.h
 *
 *  Created on: 13/6/2017
 *      Author: utnso
 */

#ifndef CAPAFS_H_
#define CAPAFS_H_
#include "Kernel.h"
#include "SocketsL.h"


uint32_t cerrarArchivo(uint32_t FD, uint32_t PID);
uint32_t escribirArchivo(uint32_t FD, uint32_t PID, uint32_t sizeArchivo, uint32_t datosAGrabar, char* permisos);
void cargarEnTablasArchivos(char* path, uint32_t PID, char* permisos);
uint32_t abrirArchivo(char* path, uint32_t PID, char* permisos);
uint32_t leerArchivo(uint32_t FD, uint32_t PID, uint32_t sizeArchivo, char* permisos);


#endif /* CAPAFS_H_ */
