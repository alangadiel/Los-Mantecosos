#ifndef THREADS_H_
#define THREADS_H_

#include "Service.h"
#include "CapaFS.h"
#include "CapaMemoria.h"

typedef struct {
	uint32_t PID;
	int socketConsola;
} PIDporSocketConsola;

BloqueControlProceso* FinalizarPrograma(int PID, int tipoFinalizacion);
bool KillProgram(int pidAFinalizar, int tipoFinalizacion);
void PonerElProgramaComoListo(BloqueControlProceso* pcb,Paquete* paquete,int socketFD,double tamanioTotalPaginas);
int RecibirPaqueteServidorKernel(int socketFD, char receptor[11], Paquete* paquete);
void accion(void* socket);
void dispatcher();
bool ProcesoNoEstaEjecutandoseActualmente(int pidAFinalizar);

#endif /* THREADS_H_ */
