#include "CPU.h"


BloqueControlProceso pcb;
DatosCPU datosCpu;
uint32_t cantRafagasAEjecutar;
uint32_t cantRafagasEjecutadas=0;
bool primitivaWait = false;
bool huboError = false;
bool progTerminado = false;

bool ejecutando;
bool DesconectarCPU = false;
typedef struct {
	bool ejecutando;
	BloqueControlProceso pcb;
} EstadoActualDeLaCpu;

EstadoActualDeLaCpu estadoActual;

AnSISOP_funciones functions = {
  .AnSISOP_asignar = primitiva_asignar,
  .AnSISOP_asignarValorCompartida = primitiva_asignarValorCompartida,
  .AnSISOP_definirVariable = primitiva_definirVariable,
  .AnSISOP_dereferenciar = primitiva_dereferenciar,
  .AnSISOP_finalizar = primitiva_finalizar,
  .AnSISOP_irAlLabel = primitiva_irAlLabel,
  .AnSISOP_llamarConRetorno = primitiva_llamarConRetorno,
  .AnSISOP_llamarSinRetorno = primitiva_llamarSinRetorno,
  .AnSISOP_obtenerPosicionVariable = primitiva_obtenerPosicionVariable,
  .AnSISOP_obtenerValorCompartida = primitiva_obtenerValorCompartida,
  .AnSISOP_retornar = primitiva_retornar
};


AnSISOP_kernel kernel_functions = {
	.AnSISOP_abrir = primitiva_abrir,
	.AnSISOP_borrar = primitiva_borrar,
	.AnSISOP_cerrar = primitiva_cerrar,
	.AnSISOP_escribir = primitiva_escribir,
	.AnSISOP_leer = primitiva_leer,
	.AnSISOP_liberar = primitiva_liberar,
	.AnSISOP_moverCursor = primitiva_moverCursor,
	.AnSISOP_reservar = primitiva_reservar,
	.AnSISOP_signal = primitiva_signal,
	.AnSISOP_wait = primitiva_wait
};
void sleepCpu(uint32_t milliseconds){
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
}
void obtenerValoresArchivoConfiguracion(){
	t_config* arch = config_create("ArchivoConfiguracion.txt");
	IP_KERNEL = string_duplicate(config_get_string_value(arch, "IP_KERNEL"));
	PUERTO_KERNEL = config_get_int_value(arch, "PUERTO_KERNEL");
	IP_MEMORIA = string_duplicate(config_get_string_value(arch, "IP_MEMORIA"));
	PUERTO_MEMORIA = config_get_int_value(arch, "PUERTO_MEMORIA");
	config_destroy(arch);
}

void obtenerLinea(char* instruccion, uint32_t* registro){
	uint32_t cantQueFaltaLeer = registro[1];
	uint32_t cantPaginasALeer = registro[0]/TamanioPaginaMemoria;
	uint32_t offset = registro[0] % TamanioPaginaMemoria;
	char* datos;
	int i;
	//TODO: incompleta
	for (i = 0; i < cantPaginasALeer; ++i) {
		datos = (char*)IM_LeerDatos(socketMemoria,CPU,pcb.PID,i, offset, cantQueFaltaLeer);
		string_append(&instruccion, datos);
		cantQueFaltaLeer-=TamanioPaginaMemoria;
	}
	free(datos);
}
void obtenerLineaAEjecutar(char *instruccion,RegIndiceCodigo*registro){
	/*Suponiendo que una instruccion nunca nos va a ocupar mas de 2 paginas
	TODO: puede pasar que una instruccion ocupe mas de 2 paginas?
	Si es asi, habria que hacer otra funcion */
	uint32_t paginaInicial = registro->start/TamanioPaginaMemoria;
	uint32_t offsetPaginaInicial = registro->start%TamanioPaginaMemoria;
	uint32_t cantTotalALeer = registro->offset;
	uint32_t cantPaginasALeer;

	if(offsetPaginaInicial + cantTotalALeer > TamanioPaginaMemoria)
	{
		cantPaginasALeer = 1 + ceil((cantTotalALeer - (TamanioPaginaMemoria - offsetPaginaInicial)) / TamanioPaginaMemoria); //El 1 es por la primer pagina
	}
	else
	{
		cantPaginasALeer = 1;
	}


	char* datos = string_new();

	uint32_t offsetPaginaALeer = offsetPaginaInicial;
	int i;
	for(i = 0; i < cantPaginasALeer; i++)
	{
		uint32_t cantALeerEnPagina;

		if(cantPaginasALeer > 1)
		{
			if(i == 0) //Es la primer pagina
			{
				cantALeerEnPagina = TamanioPaginaMemoria - offsetPaginaALeer;
			}
			else
			{
				if(cantTotalALeer > TamanioPaginaMemoria)
				{
					cantALeerEnPagina = TamanioPaginaMemoria;
				}
				else
				{
					cantALeerEnPagina = cantTotalALeer;
				}
			}
		}
		else
		{
			cantALeerEnPagina = cantTotalALeer;
		}
		char* linea = IM_LeerDatos(socketMemoria, CPU, pcb.PID, paginaInicial + i, offsetPaginaALeer, cantALeerEnPagina);
		string_append(&datos,linea);

		cantTotalALeer -= cantALeerEnPagina;

		offsetPaginaALeer = 0;
	}

	strcpy(instruccion, datos);

	free(datos);
}

bool terminoElPrograma(void){
	return !estadoActual.ejecutando;
}

void userInterfaceHandler(){
	while (!DesconectarCPU) {
		char orden[100];
		printf("\n\nIngrese una orden: \n");
		scanf("%s", orden);
		char* command = getWord(orden, 0);
		if(strcmp(command,"SIGUSR1")==0){
			DesconectarCPU = true;
		}
	}
}

int main(void) {
	indiceDeCodigo = list_create();
	pcb_Create(&pcb, 0); //TODO: sacar el 0 despues de mergear de newMaster

	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();

	socketKernel = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CPU, RecibirHandshake_DeKernel);
	socketMemoria = ConectarAServidor(PUERTO_MEMORIA, IP_MEMORIA, MEMORIA, CPU, RecibirHandshake_DeMemoria);

	pthread_t consola;
	pthread_create(&consola,NULL,(void*)userInterfaceHandler,NULL);

	while(!DesconectarCPU) {
		Paquete paquete;
		while (RecibirPaqueteCliente(socketKernel, CPU, &paquete)<=0);
		switch(paquete.header.tipoMensaje) {
			/*case KILLPROGRAM: //reemplazar KILLPROGRAM por algo acorde, es la señal SIGUSR1 para deconectar la CPU
				DesconectarCPU = true;
				progTerminado = true;
			break;*/
			case ESTAEJECUTANDO: {
				int tamDatos = sizeof(uint32_t) +sizeof(bool);
				void* datos = malloc(tamDatos);
				memcpy(datos,&estadoActual.ejecutando,sizeof(bool));
				memcpy(datos+sizeof(bool),&estadoActual.pcb.PID,sizeof(uint32_t));
				EnviarDatos(socketKernel, CPU, datos, tamDatos);
			}
			break;
			case ESPCB:	{
				RecibirPCB(&pcb, paquete.Payload,paquete.header.tamPayload, CPU);
				estadoActual.pcb = pcb;
				estadoActual.ejecutando = true;
				int i=0;
				progTerminado = false;
				primitivaWait = false;
				huboError = false;
				while(!primitivaWait && !huboError && !progTerminado) {
					if(pcb.cantidadDeRafagasAEjecutar > 0 && i >= pcb.cantidadDeRafagasAEjecutar) break;
					sleepCpu(QuantumSleep);
					RegIndiceCodigo* registro = list_get(pcb.IndiceDeCodigo,pcb.ProgramCounter);
					char instruccion[registro->offset];
					obtenerLineaAEjecutar(instruccion, registro);
					printf("Ejecutando rafaga N°: %u de PID: %u\n", pcb.cantidadDeRafagasEjecutadas, pcb.PID);
					analizadorLinea(instruccion,&functions,&kernel_functions);
					pcb.ProgramCounter++;
					pcb.cantidadDeRafagasEjecutadasHistorica++;
					pcb.cantidadDeRafagasEjecutadas++;
					i++;
				}
				i=0;
				// Avisar al kernel que terminaste de ejecutar la instruccion
				printf("Fin de ejecucion de rafagas\n");
				if(primitivaWait)
					EnviarPCB(socketKernel, CPU, &pcb,ESPCBWAIT);
				else
					EnviarPCB(socketKernel, CPU, &pcb,ESPCB);
				estadoActual.pcb = pcb;
				estadoActual.ejecutando = false;
			}
			break;
		}
		free(paquete.Payload);
	}

	pcb_Destroy(&pcb);
	pthread_join(consola, NULL);
	free(IP_KERNEL); free(IP_MEMORIA);

	return 0;
}
