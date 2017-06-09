#include "Primitivas.h"
#include "CPU.c"
//agregar lista de variables?

t_puntero primitiva_definirVariable(t_nombre_variable identificador_variable){
	Instruccion* ins = (Instruccion*) list_get(indiceDeCodigo,pcb.ProgramCounter);
	void* datos = IM_LeerDatos(socketMemoria,CPU, pcb.PID, numeroDePagina, ins->byteComienzo, ins->longitud);
	return datos;
}

/*
t_puntero primitiva_obtenerPosicionVariable(t_nombre_variable variable) {
	printf("Obtener posicion de %c\n", variable);
	return POSICION_MEMORIA;
}

void primitiva_finalizar(void){
	termino = true;
	printf("Finalizar\n");
}

bool terminoElPrograma(void){
	return termino;
}

t_valor_variable primitiva_dereferenciar(t_puntero puntero) {
	printf("Dereferenciar %d y su valor es: %d\n", puntero, CONTENIDO_VARIABLE);
	return CONTENIDO_VARIABLE;
}

void primitiva_asignar(t_puntero puntero, t_valor_variable variable) {
	printf("Asignando en %d el valor %d\n", puntero, variable);
}*/
