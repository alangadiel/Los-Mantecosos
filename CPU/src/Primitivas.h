#ifndef PRIMITIVAS_H_
#define PRIMITIVAS_H_

	#include <parser/parser.h>
	#include <stdbool.h>
	#include "SocketsL.h"

	t_puntero primitiva_definirVariable(t_nombre_variable variable);
	t_puntero primitiva_obtenerPosicionVariable(t_nombre_variable variable);
	t_valor_variable primitiva_dereferenciar(t_puntero puntero);
	void primitiva_asignar(t_puntero puntero, t_valor_variable variable);
	void primitiva_finalizar(void);

	bool terminoElPrograma(void);
#endif
