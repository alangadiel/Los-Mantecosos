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
	//TODO: Programar en kernel para que me devuelva el valor de una variable compartida
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
	//TODO: Programar en kernel para que me asigne el valor de una variable compartida y me devuelta el valor
	int tamDatos = sizeof(uint32_t)*3+ string_length(nombre)+1;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = ASIGNARSHAREDVAR;
	((uint32_t*) datos)[1] = pcb.PID;
	((uint32_t*) datos)[2] = valor;
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

void SolicitarWaitSemaforo(t_nombre_semaforo semaforo){
	//TODO: Programar en kernel para que decida si bloquear o no el semaforo
	int tamDatos = sizeof(uint32_t)*2+ string_length(semaforo)+1;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = WAITSEM;
	((uint32_t*) datos)[1] = pcb.PID;
	memcpy(datos+sizeof(uint32_t)*2, semaforo, string_length(semaforo)+1);
	t_valor_variable result = *(t_valor_variable*)EnviarAServidorYEsperarRecepcion(datos,tamDatos);
	free(datos);
}

void SolicitarSignalSemaforo(t_nombre_semaforo semaforo){
	//TODO: Programar en kernel para que decida si desbloquear o no los procesos frenados
	int tamDatos = sizeof(uint32_t)*2+ string_length(semaforo)+1;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = SIGNALSEM;
	((uint32_t*) datos)[1] = pcb.PID;
	memcpy(datos+sizeof(uint32_t)*2, semaforo, string_length(semaforo)+1);
	uint32_t* result = *(t_valor_variable*)EnviarAServidorYEsperarRecepcion(datos,tamDatos);
	free(datos);
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

void LiberarBloqueMemoriaDinamica(t_puntero puntero){
	int tamDatos = sizeof(uint32_t)*3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = LIBERARHEAP;
	((uint32_t*) datos)[1] = pcb.PID;
	((uint32_t*) datos)[2] = puntero;
	//Este result es para indicar si salio todo bien o no,pero no seria necesario
	uint32_t result = *(uint32_t*)EnviarAServidorYEsperarRecepcion(datos,tamDatos);
	free(datos);
}
t_descriptor_archivo SolicitarAbrirArchivo(t_direccion_archivo direccion, t_banderas flags){
	//TODO: Programar en kernel para que abra el archivo
	int tamDatos = sizeof(uint32_t)*2+sizeof(t_banderas)+string_length(direccion)+1;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] =ABRIRARCHIVO;
	((uint32_t*) datos)[1] = pcb.PID;
	((bool*) datos)[sizeof(uint32_t)*2] =flags.creacion;
	((bool*) datos)[sizeof(uint32_t)*2+sizeof(bool)] =flags.escritura;
	((bool*) datos)[sizeof(uint32_t)*2+sizeof(bool)*2] =flags.lectura;
	memcpy(datos+sizeof(uint32_t)*2+sizeof(t_banderas), direccion, string_length(direccion)+1);
	t_descriptor_archivo result = *(t_valor_variable*)EnviarAServidorYEsperarRecepcion(datos,tamDatos);
	free(datos);
	return result;
}
void SolicitarBorrarArchivo(t_descriptor_archivo desc){
	int tamDatos = sizeof(uint32_t)*3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = BORRARARCHIVO;
	((uint32_t*) datos)[1] = pcb.PID;
	((uint32_t*) datos)[2] = desc;
	//Este result es para indicar si salio todo bien o no,pero no seria necesario
	uint32_t result = *(uint32_t*)EnviarAServidorYEsperarRecepcion(datos,tamDatos);
	free(datos);
}
void SolicitarCerrarArchivo(t_descriptor_archivo desc){
	int tamDatos = sizeof(uint32_t)*3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = CERRARARCHIVO;
	((uint32_t*) datos)[1] = pcb.PID;
	((uint32_t*) datos)[2] = desc;
	//Este result es para indicar si salio todo bien o no,pero no seria necesario
	uint32_t result = *(uint32_t*)EnviarAServidorYEsperarRecepcion(datos,tamDatos);
	free(datos);
}
void SolicitarMoverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){
	int tamDatos = sizeof(uint32_t)*3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = MOVERCURSOSARCHIVO;
	((uint32_t*) datos)[1] = pcb.PID;
	((uint32_t*) datos)[2] = descriptor_archivo;
	//Posicion a donde mover el cursor
	((uint32_t*) datos)[3] = posicion;
	//Este result es para indicar si salio todo bien o no,pero no seria necesario
	uint32_t result = *(uint32_t*)EnviarAServidorYEsperarRecepcion(datos,tamDatos);
	free(datos);
}


//agregar lista de variables?

t_puntero primitiva_definirVariable(t_nombre_variable identificador_variable){
	//Obtengo el ultimo contexto de ejecucion, donde guardara la/s variable/s a definir
	IndiceStack* is=(IndiceStack*)list_get(pcb.IndiceDelStack,list_size(pcb.IndiceDelStack)-1);
	Variable varNueva;
	PosicionDeMemoria pos;
	pos.NumeroDePagina=0;
	pos.Tamanio = sizeof(int);
	//Como se que cada variable son enteros bytes, el offset siempre se incrementa en 4
	pos.Offset = ultimoOffSetVariablesStack + sizeof(int);
	ultimoOffSetVariablesStack = pos.Offset;
	varNueva.Posicion=pos;
	varNueva.ID=identificador_variable;
	list_add(is->Variables,&varNueva);
	return ultimoOffSetVariablesStack;
}


t_puntero primitiva_obtenerPosicionVariable(t_nombre_variable variable) {
	void* result = NULL;
	int j=0;
	while(j< list_size(pcb.IndiceDelStack) && result==NULL){
		IndiceStack* is = (IndiceStack*)list_get(pcb.IndiceDelStack,j);
		result = (Variable*)list_find(is->Variables,
						LAMBDA(bool _(void*item){return ((Variable*)item)->ID==variable;}));
		j++;
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
	t_valor_variable result = PedirValorVariableCompartida(variable);
	t_valor_variable val = *(t_valor_variable*)result;
	return val;
}
t_valor_variable primitiva_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	t_valor_variable result = AsignarValorVariableCompartida(variable,valor);
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
	void *result = NULL;
	int i=0;
	while(i< list_size(pcb.IndiceDelStack) && result == NULL){
		IndiceStack* is = (IndiceStack*)list_get(pcb.IndiceDelStack,i);
		result = list_find(is->Variables,
				LAMBDA(bool _(void*item){return ((Variable*)item)->Posicion.Offset==donde_retornar;}));
		i++;
	}

	nuevoIs.PosVariableDeRetorno = ((Variable*)result)->Posicion;
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
PosicionDeMemoria NewPosicionDeMemoriaVacia(){
	PosicionDeMemoria res;
	res.NumeroDePagina =NULL;
	res.Offset=NULL;
	res.Tamanio = NULL;
	return res;
}
void primitiva_retornar(t_valor_variable retorno){
	//Obtengo la ultima instruccion de retorno del stack
	int i=0;
	void *result=NULL;
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
	PosicionDeMemoria res=NewPosicionDeMemoriaVacia();
	while(j< list_size(pcb.IndiceDelStack)){
		IndiceStack* is = (IndiceStack*)list_get(pcb.IndiceDelStack,j);
		//Forma de verificar si la estructura PosicionDeMemoria esta vacio o no
		if(is->PosVariableDeRetorno.NumeroDePagina != NULL && is->PosVariableDeRetorno.Offset != NULL && is->PosVariableDeRetorno.Tamanio != NULL){
			res = is->PosVariableDeRetorno;
		}
		j++;
	}
	PosicionDeMemoria pos = res;
	//Guardo el valor de retorno en la variable correspondiente
	primitiva_asignar(pos.Offset,retorno);

	/*Elimino el registro del stack de la funcion
	TODO: EN PRINCIPIO ELIMINO EL ULTIMO REGISTRO DE STACK, VER QUE PASA SI PARA UNA FUNCION HAY MAS DE UN REGISTRO
	*/
	list_remove(pcb.IndiceDelStack,list_size(pcb.IndiceDelStack)-1);

}



//KERNEL
void primitiva_wait(t_nombre_semaforo identificador_semaforo){
	SolicitarWaitSemaforo(identificador_semaforo);
}
void primitiva_signal(t_nombre_semaforo identificador_semaforo){
	SolicitarSignalSemaforo(identificador_semaforo);
}
t_puntero primitiva_reservar(t_valor_variable espacio){
	t_puntero pointer = *(t_puntero*)ReservarBloqueMemoriaDinamica(espacio);
	return pointer;
}
void primitiva_liberar(t_puntero puntero){
	LiberarBloqueMemoriaDinamica(puntero);
}
t_descriptor_archivo primitiva_abrir(t_direccion_archivo direccion, t_banderas flags){
	t_descriptor_archivo fd = SolicitarAbrirArchivo(direccion,flags);
	return fd;
}
void primitiva_borrar(t_descriptor_archivo descriptor_archivo){
	SolicitarBorrarArchivo(descriptor_archivo);
}
void primitiva_cerrar(t_descriptor_archivo descriptor_archivo){
	SolicitarCerrarArchivo(descriptor_archivo);
}
void primitiva_moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){
	SolicitarMoverCursor(descriptor_archivo,posicion);
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
