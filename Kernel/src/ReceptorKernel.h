#ifndef RECEPTORKERNEL_H_
#define RECEPTORKERNEL_H_

#include "Service.h"
#include "ThreadsKernel.h"

void receptorKernel(Paquete* paquete, int socketConectado, bool* entroSignal, Semaforo** semSignal);

#endif /* RECEPTORKERNEL_H_ */
