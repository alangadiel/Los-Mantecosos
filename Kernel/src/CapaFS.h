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
	BanderasPermisos flags; //[0] es Creacion, [1] es Escritura, [2] es Lectura
	uint32_t globalFD;
} archivoProceso;

typedef struct {
	char* pathArchivo;
	uint32_t cantAperturas;
	uint32_t archivoGlobalFD;
} archivoGlobal;

void cerrarArchivo(uint32_t FD, uint32_t PID);
uint32_t escribirArchivo(uint32_t FD, uint32_t PID, uint32_t sizeArchivo, void* datosAGrabar);
uint32_t cargarEnTablasArchivos(char* path, uint32_t PID, BanderasPermisos permisos);
uint32_t abrirArchivo(char* path, uint32_t PID, BanderasPermisos permisos, int socketConectado, int32_t* tipoError);
void* leerArchivo(uint32_t FD, uint32_t PID, uint32_t sizeArchivo, uint32_t punteroArchivo);
void moverCursor(uint32_t FD, uint32_t PID, uint32_t posicion);
void borrarArchivo(uint32_t FD, uint32_t PID, int socketConectado);
void finalizarProgramaCapaFS(int PID);
t_list* obtenerTablaArchivosDeUnProceso(uint32_t PID);
t_list* obtenerTablaArchivosGlobales();

#endif /* CAPAFS_H_ */
