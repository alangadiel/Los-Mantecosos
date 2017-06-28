#ifndef USERINTERFACE_H_
#define USERINTERFACE_H_

#include "Service.h"
#include "Threads.h"

void ConsultarEstado(int pidAConsultar);
void MostrarTodosLosProcesos();
void MostrarProcesosDeUnaCola(t_queue* cola,char* discriminator);
void userInterfaceHandler(uint32_t* socketFD);

#endif /* USERINTERFACE_H_ */
