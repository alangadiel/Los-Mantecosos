#ifndef CAPAFS_H_
#define CAPAFS_H_

#include "Service.h"
#include "ThreadsKernel.h"

typedef struct {
	bool creacion;
	bool escritura;
	bool lectura;
} permisosArchivo;

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

uint32_t cerrarArchivo(uint32_t FD, uint32_t PID);
uint32_t escribirArchivo(uint32_t FD, uint32_t PID, uint32_t sizeArchivo, char* datosAGrabar);
void cargarEnTablasArchivos(char* path, uint32_t PID, permisosArchivo permisos);
uint32_t abrirArchivo(char* path, uint32_t PID, permisosArchivo permisos, int socketConectado);
void leerArchivo(uint32_t FD, uint32_t PID, uint32_t sizeArchivo);
uint32_t moverCursor(uint32_t FD, uint32_t PID, uint32_t posicion);
uint32_t borrarArchivo(uint32_t FD, uint32_t PID);
void finalizarProgramaCapaFS(int PID);
t_list* obtenerTablaArchivosDeUnProceso(uint32_t PID);
t_list* obtenerTablaArchivosGlobales();

#endif /* CAPAFS_H_ */
