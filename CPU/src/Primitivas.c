#include "Primitivas.h"

void* EnviarAServidorYEsperarRecepcion(void* datos,int tamDatos){
	EnviarDatos(socketKernel,CPU,datos,tamDatos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketKernel, CPU, paquete) <= 0);
	void* r;
	if(paquete->header.tipoMensaje == ESERROR)
		r = NULL;
	else if(paquete->header.tipoMensaje == ESDATOS)
		r = paquete->Payload;
	free(paquete);
	return r;
}

t_valor_variable PedirValorVariableCompartida(t_nombre_variable* nombre){
	int tamDatos = sizeof(uint32_t)*2+ string_length(nombre)+1;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = PEDIRSHAREDVAR;
	((uint32_t*) datos)[1] = pcb.PID;
	memcpy(datos+sizeof(uint32_t)*2, nombre, string_length(nombre)+1);
	t_valor_variable result = *(t_valor_variable*)EnviarAServidorYEsperarRecepcion(datos,tamDatos);
	free(datos);
	return result;
}
t_valor_variable AsignarValorVariableCompartida(t_nombre_variable* nombre,t_valor_variable valor ){
	int tamDatos = sizeof(uint32_t)*3+ string_length(nombre)+1;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = ASIGNARSHAREDVAR;
	((uint32_t*) datos)[1] = pcb.PID;
	((int32_t*) datos)[2] = valor;
	memcpy(datos+sizeof(uint32_t)*3, nombre, string_length(nombre)+1);
	t_valor_variable result = *(t_valor_variable*)EnviarAServidorYEsperarRecepcion(datos,tamDatos);
	free(datos);
	return result;
}
/*Al ejecutar la última sentencia, el CPU deberá notificar al Kernel que el proceso finalizó para que este
se ocupe de solicitar la eliminación de las estructuras utilizadas por el sistema*/
void FinDeEjecucionPrograma(){
	int tamDatos = sizeof(uint32_t)*2;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = FINEJECUCIONPROGRAMA;
	((uint32_t*) datos)[1] = pcb.PID;
	EnviarDatos(socketKernel,CPU,datos,tamDatos);
}



t_puntero ReservarBloqueMemoriaDinamica(t_valor_variable espacio){
	int tamDatos = sizeof(uint32_t)*3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = RESERVARHEAP;
	((uint32_t*) datos)[1] = pcb.PID;
	((uint32_t*) datos)[2] = espacio;
	t_puntero result = *(t_puntero*)EnviarAServidorYEsperarRecepcion(datos,tamDatos);
	free(datos);
	return result;
}

t_descriptor_archivo SolicitarAbrirArchivo(t_direccion_archivo direccion, t_banderas flags){
	//TODO: Programar en kernel para que abra el archivo
	int tamDatos = sizeof(uint32_t)*2+sizeof(t_banderas)+string_length(direccion)+1;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] =ABRIRARCHIVO;
	((uint32_t*) datos)[1] = pcb.PID;
	((bool*) datos)[2] =flags.creacion;
	((bool*) datos)[3] =flags.escritura;
	((bool*) datos)[4] =flags.lectura;
	memcpy(datos+sizeof(uint32_t)*2+sizeof(t_banderas), direccion, string_length(direccion)+1);
	t_descriptor_archivo result = *(t_valor_variable*)EnviarAServidorYEsperarRecepcion(datos,tamDatos);
	free(datos);
	return result;
}

void CrearRegistroStack(regIndiceStack* is){
	is->Argumentos = list_create();
	is->Variables = list_create();
}

int32_t obtenerUltimoOffset(regIndiceStack* regIS){
	int32_t r = -sizeof(uint32_t);
	Variable* ultimoArg = (Variable*)list_get(regIS->Argumentos,list_size(regIS->Argumentos)-1);
	Variable* ultimoVar = (Variable*)list_get(regIS->Variables,list_size(regIS->Variables)-1);
	if(ultimoVar!=NULL)
		r = ultimoVar->Posicion->NumeroDePagina*ultimoVar->Posicion->Tamanio + ultimoVar->Posicion->Offset;
	if(ultimoArg!=NULL){
		uint32_t ultimaDirVirtualArg = ultimoArg->Posicion->NumeroDePagina*ultimoArg->Posicion->Tamanio + ultimoArg->Posicion->Offset;
		if(ultimaDirVirtualArg>r)
			r = ultimaDirVirtualArg;
	}
	return r;
}
//primitivas basicas
t_puntero primitiva_definirVariable(t_nombre_variable identificador_variable){
	//Obtengo el ultimo contexto de ejecucion, donde guardara la/s variable/s a definir
	regIndiceStack* is=malloc(sizeof(regIndiceStack));
	void* res = list_get(pcb.IndiceDelStack,list_size(pcb.IndiceDelStack)-1);
	if(res!=NULL){
		is=list_get(pcb.IndiceDelStack,list_size(pcb.IndiceDelStack)-1);
	}
	else {
		//No hay ningun registro de stack hasta el momento
		is = malloc(sizeof(regIndiceStack));
		CrearRegistroStack(is);
		list_add(pcb.IndiceDelStack, is);
	}
	ultimoOffSetVariablesStack = obtenerUltimoOffset(is);
	ultimoOffSetVariablesStack += sizeof(int);
	Variable* varNueva = malloc(sizeof(Variable));
	PosicionDeMemoria* pos = malloc(sizeof(PosicionDeMemoria));
	pos->NumeroDePagina=ultimoOffSetVariablesStack/TamanioPaginaMemoria;
	pos->Tamanio = sizeof(int);
	//Como se que cada variable son enteros bytes, el offset siempre se incrementa en 4
	pos->Offset = ultimoOffSetVariablesStack % TamanioPaginaMemoria;
	varNueva->Posicion=pos;
	varNueva->ID=identificador_variable;
	if(isdigit(identificador_variable)){//si es un numero
		list_add(is->Argumentos,varNueva);
	} else {
		list_add(is->Variables,varNueva);
	}
	/*int k;
	for (k = 0; k < list_size(is->Variables); k++) {
			Variable* var = (Variable*)list_get(is->Variables,k);
			printf("Id: %c\n",var->ID);
	}*/

	//pcb.ProgramCounter++;
	return ultimoOffSetVariablesStack;
}


t_puntero primitiva_obtenerPosicionVariable(t_nombre_variable variable) {
	void* result = NULL;
	int j=0;
	while(j< list_size(pcb.IndiceDelStack) && result==NULL){
		regIndiceStack* is = (regIndiceStack*)list_get(pcb.IndiceDelStack,j);
		/*int k;
		for (k = 0; k < list_size(is->Variables); k++) {
			Variable* var = (Variable*)list_get(is->Variables,k);
			printf("Id: %c\n",var->ID);
		}*/
		result = (Variable*)list_find(is->Variables,LAMBDA(bool _(void*item){return ((Variable*)item)->ID==variable;}));
		j++;
	}
	//pcb.ProgramCounter++;
	if(result==NULL)
		return -1;
	else
	{
		Variable* var = (Variable*)result;
		return var->Posicion->NumeroDePagina*TamanioPaginaMemoria+ var->Posicion->Offset;
	}

}
t_valor_variable primitiva_dereferenciar(t_puntero puntero) {
	int nroPag = puntero/TamanioPaginaMemoria;
	int offset = puntero%TamanioPaginaMemoria;
	void* dato = IM_LeerDatos(socketMemoria,CPU,pcb.PID,nroPag,offset,sizeof(int));
	t_valor_variable val = *(t_valor_variable*)dato;
	//pcb.ProgramCounter++;

	return val;
}

void primitiva_asignar(t_puntero puntero, t_valor_variable variable) {
	t_valor_variable val = variable;
	int nroPag = puntero/TamanioPaginaMemoria;
	int offset = puntero%TamanioPaginaMemoria;
	IM_GuardarDatos(socketMemoria,CPU,pcb.PID,nroPag,offset,sizeof(int),&val);
	//pcb.ProgramCounter++;


}

t_valor_variable primitiva_obtenerValorCompartida(t_nombre_compartida variable){
	t_valor_variable result = PedirValorVariableCompartida(variable);
	t_valor_variable val = *(t_valor_variable*)result;
	pcb.cantidadSyscallEjecutadas++;
	//pcb.ProgramCounter++;
	return val;
}

t_valor_variable primitiva_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	t_valor_variable result = AsignarValorVariableCompartida(variable,valor);
	t_valor_variable val = *(t_valor_variable*)result;
	pcb.cantidadSyscallEjecutadas++;
	//pcb.ProgramCounter++;
	return val;
}

void primitiva_irAlLabel(t_nombre_etiqueta t_nombre_etiqueta){
	//La proxima instruccion a ejecutar es la de la linea donde esta la etiqueta
	pcb.ProgramCounter = *(int*)dictionary_get(pcb.IndiceDeEtiquetas,t_nombre_etiqueta);

}
void primitiva_llamarSinRetorno(t_nombre_etiqueta etiqueta){
		regIndiceStack* nuevoIs=malloc(sizeof(regIndiceStack));
		//Entrada del indice de codigo a donde se debe regresar
		nuevoIs->DireccionDeRetorno = pcb.ProgramCounter;
		//La proxima instruccion a ejecutar es la de la funcion en cuestion
		primitiva_irAlLabel(etiqueta);
		//agregar al indiceDeStack 
		list_add(pcb.IndiceDelStack, nuevoIs);
}

void primitiva_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	regIndiceStack* nuevoIs=malloc(sizeof(regIndiceStack));
	//Entrada del indice de codigo a donde se debe regresar
	nuevoIs->DireccionDeRetorno = pcb.ProgramCounter;
	//La posicion donde se almacenara es aquella que empieza en el puntero donde_retornar
	void *result = NULL;
	int i=0;
	while(i< list_size(pcb.IndiceDelStack) && result == NULL){
		regIndiceStack* is = (regIndiceStack*)list_get(pcb.IndiceDelStack,i);
		result = list_find(is->Variables,
				LAMBDA(bool _(void*item){
			return TamanioPaginaMemoria* ((Variable*)item)->Posicion->NumeroDePagina +((Variable*)item)->Posicion->Offset==donde_retornar;
		}));
		i++;
	}

	nuevoIs->PosVariableDeRetorno = ((Variable*)result)->Posicion;
	//La proxima instruccion a ejecutar es la de la funcion en cuestion
	primitiva_irAlLabel(etiqueta);
	//agregar al indiceDeStack
	list_add(pcb.IndiceDelStack, nuevoIs);
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
	regIndiceStack* is = list_get(pcb.IndiceDelStack,list_size(pcb.IndiceDelStack)-1);
	pcb.ProgramCounter = is->DireccionDeRetorno;
	//Guardo el valor de retorno en la variable correspondiente
	primitiva_asignar(is->PosVariableDeRetorno->Offset,retorno);
	list_remove(pcb.IndiceDelStack,list_size(pcb.IndiceDelStack)-1);
}

//PRIMITIVAS KERNEL
void primitiva_wait(t_nombre_semaforo identificador_semaforo){

		int tamDatos = sizeof(uint32_t)*2+ string_length(identificador_semaforo)+1;
		void* datos = malloc(tamDatos);
		((uint32_t*) datos)[0] = WAITSEM;
		((uint32_t*) datos)[1] = pcb.PID;
		memcpy(datos+sizeof(uint32_t)*2, identificador_semaforo, string_length(identificador_semaforo)+1);
		EnviarDatos(socketKernel,CPU,datos,tamDatos);
		free(datos);
		pcb.cantidadSyscallEjecutadas++;
		primitivaBloqueante = true;
}

void primitiva_signal(t_nombre_semaforo identificador_semaforo){

		int tamDatos = sizeof(uint32_t)*2+ string_length(identificador_semaforo)+1;
		void* datos = malloc(tamDatos);
		((uint32_t*) datos)[0] = SIGNALSEM;
		((uint32_t*) datos)[1] = pcb.PID;
		memcpy(datos+sizeof(uint32_t)*2, identificador_semaforo, string_length(identificador_semaforo)+1);
		EnviarDatos(socketKernel,CPU,datos,tamDatos);
		free(datos);
		pcb.cantidadSyscallEjecutadas++;
		//pcb.ProgramCounter++;
}

t_puntero primitiva_reservar(t_valor_variable espacio){
	t_puntero pointer = *(t_puntero*)ReservarBloqueMemoriaDinamica(espacio);
	pcb.cantBytesAlocados += espacio + sizeof(bool) + sizeof(uint32_t); //Seria el tamanio a reservar + el tamanio del HeapMetadata que tiene un bool y un uint32_t
	pcb.cantidadAccionesAlocar++;
	pcb.cantidadSyscallEjecutadas++;
	//pcb.ProgramCounter++;
	return pointer;
}
void primitiva_liberar(t_puntero puntero){
	int tamDatos = sizeof(uint32_t)*3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = LIBERARHEAP;
	((uint32_t*) datos)[1] = pcb.PID;
	((uint32_t*) datos)[2] = puntero;
	//Este result es para indicar si salio todo bien o no,pero no seria necesario
	EnviarDatos(socketKernel,CPU,datos,tamDatos);
	free(datos);
	//pcb.cantBytesLiberados+=?
	pcb.cantidadAccionesLiberar++;
	pcb.cantidadSyscallEjecutadas++;
	//pcb.ProgramCounter++;
}
t_descriptor_archivo primitiva_abrir(t_direccion_archivo direccion, t_banderas flags){
	t_descriptor_archivo fd = SolicitarAbrirArchivo(direccion,flags);
	pcb.cantidadSyscallEjecutadas++;
	//pcb.ProgramCounter++;
	return fd;
}
void primitiva_borrar(t_descriptor_archivo descriptor_archivo){
	int tamDatos = sizeof(uint32_t)*3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = BORRARARCHIVO;
	((uint32_t*) datos)[1] = pcb.PID;
	((uint32_t*) datos)[2] = descriptor_archivo;
	EnviarDatos(socketKernel,CPU,datos,tamDatos);
	free(datos);
	pcb.cantidadSyscallEjecutadas++;
	//pcb.ProgramCounter++;

}
void primitiva_cerrar(t_descriptor_archivo descriptor_archivo){
	int tamDatos = sizeof(uint32_t)*3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = CERRARARCHIVO;
	((uint32_t*) datos)[1] = pcb.PID;
	((uint32_t*) datos)[2] = descriptor_archivo;
	EnviarDatos(socketKernel,CPU,datos,tamDatos);
	free(datos);
	pcb.cantidadSyscallEjecutadas++;
	//pcb.ProgramCounter++;

}
void primitiva_moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){
	int tamDatos = sizeof(uint32_t)*3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = MOVERCURSOSARCHIVO;
	((uint32_t*) datos)[1] = pcb.PID;
	((uint32_t*) datos)[2] = descriptor_archivo;
	//Posicion a donde mover el cursor
	((uint32_t*) datos)[3] = posicion;
	EnviarDatos(socketKernel,CPU,datos,tamDatos);
	free(datos);
	pcb.cantidadSyscallEjecutadas++;
	//pcb.ProgramCounter++;

}
void primitiva_escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio){
	int tamDatos = sizeof(uint32_t)*2 + sizeof(t_valor_variable) + sizeof(t_descriptor_archivo)+tamanio;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = ESCRIBIRARCHIVO;
	((uint32_t*) datos)[1] = pcb.PID;
	((uint32_t*) datos)[2] = descriptor_archivo;
	((uint32_t*) datos)[3] = tamanio;
	memcpy(datos+sizeof(uint32_t)*4,informacion,tamanio);
	EnviarDatos(socketKernel,CPU,datos,tamDatos);
	free(datos);
	pcb.cantidadSyscallEjecutadas++;
	//pcb.ProgramCounter++;

}
void primitiva_leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio){
	int tamDatos = sizeof(uint32_t)*2 + sizeof(t_valor_variable) + sizeof(t_descriptor_archivo) +sizeof(t_puntero);
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = LEERARCHIVO;
	((uint32_t*) datos)[1] = pcb.PID;
	((uint32_t*) datos)[2] = descriptor_archivo;
	((uint32_t*) datos)[3] = tamanio;
	EnviarDatos(socketKernel,CPU,datos,tamDatos);
	free(datos);
	pcb.cantidadSyscallEjecutadas++;
	//pcb.ProgramCounter++;
}
