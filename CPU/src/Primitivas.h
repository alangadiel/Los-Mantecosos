#ifndef PRIMITIVAS_H_
#define PRIMITIVAS_H_

	#include <parser/parser.h>
	#include <parser/metadata_program.h>
	#include <stdbool.h>
	#include "SocketsL.h"
	#include "CPU.h"
uint32_t numeroDePagina;
int32_t ultimoOffSetVariablesStack;

	t_puntero primitiva_definirVariable(t_nombre_variable variable);
	t_puntero primitiva_obtenerPosicionVariable(t_nombre_variable variable);
	t_valor_variable primitiva_dereferenciar(t_puntero puntero);
	void primitiva_asignar(t_puntero puntero, t_valor_variable variable);
	t_valor_variable primitiva_obtenerValorCompartida(t_nombre_compartida variable);
	t_valor_variable primitiva_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
	void primitiva_irAlLabel(t_nombre_etiqueta etiqueta);
	void primitiva_llamarSinRetorno(t_nombre_etiqueta etiqueta);
	void primitiva_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
	void primitiva_finalizar(void);
	void primitiva_retornar(t_valor_variable retorno);



	//Kernel
	void primitiva_wait(t_nombre_semaforo identificador_semaforo);
	void primitiva_signal(t_nombre_semaforo identificador_semaforo);
	t_puntero primitiva_reservar(t_valor_variable espacio);
	void primitiva_liberar(t_puntero puntero);
	t_descriptor_archivo primitiva_abrir(t_direccion_archivo direccion, t_banderas flags);
	void primitiva_borrar(t_descriptor_archivo descriptor_archivo);
	void primitiva_cerrar(t_descriptor_archivo descriptor_archivo);
	void primitiva_moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion);
	void primitiva_escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio);
	void primitiva_leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio);


	//Otras
	PosicionDeMemoria NewPosicionDeMemoriaVacia();


#endif
