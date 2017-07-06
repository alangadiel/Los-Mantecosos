/*
 * CPU.h
 *
 *  Created on: 12/6/2017
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include <parser/metadata_program.h>
#include "Primitivas.h"

typedef struct {
	uint32_t byteComienzo;
	uint32_t longitud;
} Instruccion;
t_list* indiceDeCodigo;
char* IP_KERNEL;
int PUERTO_KERNEL;
char* IP_MEMORIA;
int PUERTO_MEMORIA;
int socketKernel;
int socketMemoria;
extern BloqueControlProceso pcb;
extern bool primitivaBloqueante;
extern bool huboError;
extern bool progTerminado;

void obtenerValoresArchivoConfiguracion();
void* EnviarAServidorYEsperarRecepcion(void* datos,int tamDatos);
t_valor_variable PedirValorVariableCompartida(t_nombre_variable* nombre);
t_valor_variable AsignarValorVariableCompartida(t_nombre_variable* nombre,t_valor_variable valor );
void FinDeEjecucionPrograma();
void SolicitarWaitSemaforo(t_nombre_semaforo semaforo);
void SolicitarSignalSemaforo(t_nombre_semaforo semaforo);
t_puntero ReservarBloqueMemoriaDinamica(t_valor_variable espacio);
void LiberarBloqueMemoriaDinamica(t_puntero puntero);
t_descriptor_archivo SolicitarAbrirArchivo(t_direccion_archivo direccion, t_banderas flags);
void SolicitarBorrarArchivo(t_descriptor_archivo desc);
void SolicitarCerrarArchivo(t_descriptor_archivo desc);
void SolicitarMoverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion);
void imprimirArchivoConfiguracion();
int main(void);

#endif /* CPU_H_ */
