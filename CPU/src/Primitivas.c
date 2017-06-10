#include "Primitivas.h"
#include "CPU.c"
//agregar lista de variables?

t_puntero primitiva_definirVariable(t_nombre_variable identificador_variable){
	IndiceStack is;
	Variable varNueva;
	PosicionDeMemoria pos;
	pos.NumeroDePagina=0;
	pos.Tamanio = sizeof(int);
	//Como se que cada variable son enteros bytes, el offset siempre se incrementa en 4
	pos.Offset = ultimoOffSetVariablesStack + sizeof(int);
	uint32_t pointerToReturn = pos.Offset;
	varNueva.Posicion=pos;
	varNueva.ID=identificador_variable;
	list_add(is.Variables,&varNueva);
	list_add(pcb.IndiceDelStack,&is);
	return pointerToReturn;
}


t_puntero primitiva_obtenerPosicionVariable(t_nombre_variable variable) {
	void* result = NULL;
	int j=0;
	while(j< list_size(pcb.IndiceDelStack) && result==NULL){
		IndiceStack* is = (IndiceStack*)list_get(pcb.IndiceDelStack,j);
		result = (Variable*)list_find(is->Variables,
						LAMBDA(bool _(void*item){return ((Variable*)item)->ID==variable;}));
	}
	if(result==NULL)
		return -1;
	else
	{
		Variable* var = (Variable*)result;
		return var->Posicion.Offset;
	}

}
t_valor_variable primitiva_dereferenciar(t_puntero puntero) {
	void* dato = IM_LeerDatos(socketMemoria,CPU,pcb.PID,0,puntero,sizeof(int));
	t_valor_variable val = *(t_valor_variable*)dato;
	return val;
}
void primitiva_asignar(t_puntero puntero, t_valor_variable variable) {
	t_valor_variable val = variable;
	IM_GuardarDatos(socketMemoria,CPU,pcb.PID,0,puntero,sizeof(int),&val);
}
t_valor_variable primitiva_obtenerValorCompartida(t_nombre_compartida variable){
	void* result = PedirValorVariableCompartida(variable);
	t_valor_variable val = *(t_valor_variable*)result;
	return val;
}
t_valor_variable primitiva_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	void result = AsignarValorVariableCompartida(variable,valor);
	t_valor_variable val = *(t_valor_variable*)result;
	return val;
}
void primitiva_irAlLabel(t_nombre_etiqueta t_nombre_etiqueta){

}

/*
void primitiva_finalizar(void){
	termino = true;
	printf("Finalizar\n");
}

bool terminoElPrograma(void){
	return termino;
}


*/
