#include "Primitivas.h"
#include "CPU.c"
//agregar lista de variables?

t_puntero primitiva_definirVariable(t_nombre_variable identificador_variable){
	//Obtengo el ultimo contexto de ejecucion, donde guardara la/s variable/s a definir
	IndiceStack is= list_get(pcb.IndiceDelStack,list_size(pcb.IndiceDelStack)-1);
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
	return pointerToReturn;
}


t_puntero primitiva_obtenerPosicionVariable(t_nombre_variable variable) {
	void* result = NULL;
	int j=0;
	while(j< list_size(pcb.IndiceDelStack) && result==NULL){
		IndiceStack* is = (IndiceStack*)list_get(pcb.IndiceDelStack,j);
		result = (Variable*)list_find(is->Variables,
						LAMBDA(bool _(void*item){return ((Variable*)item)->ID==variable;}));
		i++;
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
	void* dato = IM_LeerDatos(socketMemoria,CPU,pcb.PID,1,puntero,sizeof(int));
	t_valor_variable val = *(t_valor_variable*)dato;
	return val;
}
void primitiva_asignar(t_puntero puntero, t_valor_variable variable) {
	t_valor_variable val = variable;
	IM_GuardarDatos(socketMemoria,CPU,pcb.PID,PAGINASTACK,puntero,sizeof(int),&val);
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
	//La proxima instruccion a ejecutar es la de la linea donde esta la etiqueta
	pcb.ProgramCounter = *(int*)dictionary_get(pcb.IndiceDeEtiquetas,t_nombre_etiqueta);

}

void primitiva_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	IndiceStack nuevoIs;
	//Entrada del indice de codigo a donde se debe regresar
	nuevoIs.DireccionDeRetorno = pcb.ProgramCounter;
	//La posicion donde se almacenara es aquella que empieza en el puntero donde_retornar
	void result = NULL;
	int i=0;
	while(i< list_size(pcb.IndiceDelStack) && result == NULL){
		IndiceStack* is = (IndiceStack*)list_get(pcb.IndiceDelStack,i);
		result = list_find(is->Variables,
				LAMBDA(bool _(void*item){return ((Variable*)item)->Posicion.Offset==donde_retornar;}));
		i++;
	}

	nuevoIs.PosVariableDeRetorno = *(Variable*)result;
	//La proxima instruccion a ejecutar es la de la funcion en cuestion
	primitiva_irAlLabel(etiqueta);
}
//TODAS LAS FUNCIONES RETORNAR UN VALOR, NO EXISTE EL CONCEPTO DE PROCEDIMIENTO: LO DICE EL TP
void primitiva_finalizar(void){
	//Si hay un solo registro de stack y se llama esta funcion, hay que finalizar el programa
	if(list_size(pcb.IndiceDelStack)==1){
		FinDeEjecucionPrograma();
	}
}
void primitiva_retornar(t_valor_variable retorno){
	//Obtengo la ultima instruccion de retorno del stack
	int i=0;
	void result=NULL;
	while(i< list_size(pcb.IndiceDelStack)){
		IndiceStack* is = (IndiceStack*)list_get(pcb.IndiceDelStack,i);
		if(is->DireccionDeRetorno!=NULL){
			result = is->DireccionDeRetorno;
		}
		i++;
	}
	pcb.ProgramCounter = *(uint32_t*)result;
	//Le asigno a la variable de retorno, el valor de retorno correspondiente

	//Obtengo la variable en cuestion
	int j=0;
	void res=NULL;
	while(j< list_size(pcb.IndiceDelStack)){
		IndiceStack* is = (IndiceStack*)list_get(pcb.IndiceDelStack,j);
		if(is->PosVariableDeRetorno!=NULL){
			res = is->PosVariableDeRetorno;
		}
		j++;
	}
	t_valor_variable* val=&retorno;
	PosicionDeMemoria pos = *(PosicionDeMemoria*)res;
	//Guardo el valor de retorno en la variable correspondiente
	primitiva_asignar(pos.Offset,retorno);

	/*Elimino el registro del stack de la funcion
	TODO: EN PRINCIPIO ELIMINO EL ULTIMO REGISTRO DE STACK, VER QUE PASA SI PARA UNA FUNCION HAY MAS DE UN REGISTRO
	*/
	list_remove(pcb.IndiceDelStack,list_size(pcb.IndiceDelStack)-1);

}







//KERNEL
t_puntero primitiva_reservar(t_valor_variable espacio){
	t_puntero pointer = *(t_puntero*)ReservarBloqueMemoriaDinamica(espacio);
	return pointer;
}
void primitiva_liberar(t_puntero puntero){
	LiberarBloqueMemoriaDinamica(puntero);
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
