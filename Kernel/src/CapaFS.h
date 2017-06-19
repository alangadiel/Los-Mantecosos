#ifndef CAPAFS_H_
#define CAPAFS_H_

#include "Service.h"

typedef struct {
	uint32_t PID;
	uint32_t FD; //Empieza a partir de FD = 3. El 0, 1 y 2 estan reservados por convencion.
	uint32_t offsetArchivo;
	permisosArchivo flags; //[0] es Creacion, [1] es Escritura, [2] es Lectura
	uint32_t globalFD;
} archivoProceso;

typedef struct {
	char* pathArchivo;
	uint32_t cantAperturas;
} archivoGlobal;

typedef struct {
	bool creacion;
	bool escritura;
	bool lectura;
} permisosArchivo;

uint32_t cerrarArchivo(uint32_t FD, uint32_t PID);
uint32_t escribirArchivo(uint32_t FD, uint32_t PID, uint32_t sizeArchivo, uint32_t datosAGrabar, char* permisos);
void cargarEnTablasArchivos(char* path, uint32_t PID, char* permisos);
uint32_t abrirArchivo(char* path, uint32_t PID, char* permisos);
uint32_t leerArchivo(uint32_t FD, uint32_t PID, uint32_t sizeArchivo, char* permisos);

#endif /* CAPAFS_H_ */
