#include "CPU.h"


BloqueControlProceso pcb;
DatosCPU datosCpu;
uint32_t cantRafagasAEjecutar;
uint32_t cantRafagasEjecutadas=0;
bool primitivaBloqueante = false;

bool ejecutando;
bool DesconectarCPU = false;
typedef struct {
	uint32_t ejecutando;
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

void obtenerValoresArchivoConfiguracion(){
	int contadorDeVariables = 0;
	int c;
	FILE *file;

	file = fopen("ArchivoConfiguracion.txt", "r");

	if (file)
	{
		while ((c = getc(file)) != EOF)
		{
			if (c == '=')
			{
				if (contadorDeVariables == 3)
				{
					fscanf(file, "%i", &PUERTO_MEMORIA);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 2)
				{
					char buffer[10000];

					IP_MEMORIA = fgets(buffer, sizeof buffer, file);
					strtok(IP_MEMORIA, "\n");

					contadorDeVariables++;
				}
				if (contadorDeVariables == 1)
				{
					fscanf(file, "%i", &PUERTO_KERNEL);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 0)
				{
					char buffer[10000];

					IP_KERNEL = fgets(buffer, sizeof buffer, file);
					strtok(IP_KERNEL, "\n");

					contadorDeVariables++;
				}
			}
		}

		fclose(file);
	}
}


void imprimirArchivoConfiguracion() {
	int c;
	FILE *file;

	file = fopen("ArchivoConfiguracion.txt", "r");

	if (file) {
		while ((c = getc(file)) != EOF) {
			putchar(c);
		}
		fclose(file);
	}
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
void obtenerLineaAEjecutar(char *instruccion,uint32_t*registro){
	/*Suponiendo que una instruccion nunca nos va a ocupar mas de 2 paginas
	TODO: puede pasar que una instruccion ocupe mas de 2 paginas?
	Si es asi, habria que hacer otra funcion */
	uint32_t paginaInicial = registro[0]/TamanioPaginaMemoria;
	uint32_t offsetPaginaInicial = registro[0]%TamanioPaginaMemoria;
	uint32_t cantTotalALeer = registro[1];
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

		string_append(&datos,(char*)IM_LeerDatos(socketMemoria, CPU, pcb.PID, paginaInicial + i, offsetPaginaALeer, cantALeerEnPagina));

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

	socketKernel = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CPU, RecibirHandshake);
	socketMemoria = ConectarAServidor(PUERTO_MEMORIA, IP_MEMORIA, MEMORIA, CPU, RecibirHandshake_DeMemoria);

	pthread_t consola;
	pthread_create(&consola,NULL,(void*)userInterfaceHandler,NULL);

	while(!DesconectarCPU) {
		Paquete paquete;
		while (RecibirPaqueteCliente(socketKernel, CPU, &paquete)<=0);
		switch(paquete.header.tipoMensaje) {
			case KILLPROGRAM: //reemplazar KILLPROGRAM por algo acorde, es la señal SIGUSR1 para deconectar la CPU
				DesconectarCPU = true;
			break;
			case ESTAEJECUTANDO: {
				int tamDatos = sizeof(uint32_t) * 2;
				void* datos = malloc(tamDatos);
				((uint32_t*) datos)[0] = terminoElPrograma();
				((uint32_t*) datos)[1] = estadoActual.pcb.PID;
				EnviarDatos(socketKernel, CPU, datos, tamDatos);
			}
			break;
			case ESPCB:	{
				RecibirPCB(&pcb, socketKernel, CPU);
				estadoActual.pcb = pcb;
				estadoActual.ejecutando = true;
				int i=0;
				primitivaBloqueante = false;
				while(i< cantRafagasAEjecutar && !primitivaBloqueante) {
					uint32_t* registro = (uint32_t*)list_get(pcb.IndiceDeCodigo,pcb.ProgramCounter);
					char instruccion[registro[1]];
					obtenerLineaAEjecutar(instruccion, registro);
					analizadorLinea(instruccion,&functions,&kernel_functions);
					pcb.cantidadDeRafagasEjecutadas++;
					cantRafagasEjecutadas++;
					i++;
				}
				// Avisar al kernel que terminaste de ejecutar la instruccion
				EnviarPCB(socketKernel, CPU, &pcb);
				estadoActual.pcb = pcb;
				estadoActual.ejecutando = false;
			}
			break;

		}
	}

	pcb_Destroy(&pcb);
	pthread_join(consola, NULL);
	return 0;
}
