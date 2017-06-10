#ifndef PRIMITIVAS_H_
#define PRIMITIVAS_H_

	#include <parser/parser.h>
	#include <parser/metadata_program.h>
	#include <stdbool.h>
	#include "SocketsL.h"

	t_puntero primitiva_definirVariable(t_nombre_variable variable);
	t_puntero primitiva_obtenerPosicionVariable(t_nombre_variable variable);
	t_valor_variable primitiva_dereferenciar(t_puntero puntero);
	void primitiva_asignar(t_puntero puntero, t_valor_variable variable);
	t_valor_variable primitiva_obtenerValorCompartida(t_nombre_compartida variable);
	t_valor_variable primitiva_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
	void primitiva_irAlLabel(t_nombre_etiqueta t_nombre_etiqueta);
	void primitiva_finalizar(void);

	bool terminoElPrograma(void);
#endif
