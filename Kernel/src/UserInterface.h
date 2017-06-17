#ifndef USERINTERFACE_H_
#define USERINTERFACE_H_

#include "Service.h"

void ConsultarEstado(int pidAConsultar);
void MostrarTodosLosProcesos();
void MostrarProcesosDeUnaLista(t_list* lista,char* discriminator);
void userInterfaceHandler(uint32_t* socketFD);

#endif /* USERINTERFACE_H_ */
