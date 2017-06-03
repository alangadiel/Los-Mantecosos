#include "Primitivas.h"
//agregar lista de variables?

t_puntero primitiva_definirVariable(t_nombre_variable identificador_variable){
	// acá debería usar alguna de las funciones de IM para hacer una especie de malloc con la memoria
	// es decir solicitar el espacio en memoria (tiene que estar cargado con basura igual q hace malloc)
	// Asignar paginas? Leer Datos? Inicializar programa?
	return 1;
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
