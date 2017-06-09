/*
 * Service.h
 *
 *  Created on: 9/6/2017
 *      Author: utnso
 */

#ifndef SERVICE_H_
#define SERVICE_H_

#define INDEX_NUEVOS 0
#define INDEX_LISTOS 1
#define INDEX_EJECUTANDO 2
#define INDEX_BLOQUEADOS 3
#define INDEX_FINALIZADOS 4

#define NUEVOS "NUEVOS"
#define LISTOS "LISTOS"
#define EJECUTANDO "EJECUTANDO"
#define BLOQUEADOS "BLOQUEADOS"
#define FINALIZADOS "FINALIZADOS"



void CrearListasEstados();
void CrearNuevoProceso(BloqueControlProceso* pcb,int* ultimoPid,t_list* nuevos);
void LimpiarListas();
void obtenerError(int exitCode);
void obtenerValoresArchivoConfiguracion();
void imprimirArchivoConfiguracion();
#endif /* SERVICE_H_ */
