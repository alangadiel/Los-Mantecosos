#ifndef THREADSKERNEL_H_
#define THREADSKERNEL_H_

#include "Service.h"
#include "CapaFS.h"
#include "CapaMemoria.h"
#include "ReceptorKernel.h"

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
bool ProcesoEstaEjecutandoseActualmente(int pidAFinalizar);
void AgregarAListadePidsPorSocket(uint32_t PID, int socket);
void CargarInformacionDelCodigoDelPrograma(BloqueControlProceso* pcb,Paquete* paquete);
void GuardarCodigoDelProgramaEnLaMemoria(BloqueControlProceso* bcp, Paquete* paquete);
void PonerElProgramaComoListo(BloqueControlProceso* pcb, Paquete* paquete, int socketFD, double tamanioTotalPaginas);
void sem_signal(Semaforo* semaf);

#endif /* THREADSKERNEL_H_ */
