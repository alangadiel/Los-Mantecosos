#include "CPU.h"


BloqueControlProceso pcb;
DatosCPU datosCpu;
uint32_t cantRafagasAEjecutar;
uint32_t cantRafagasEjecutadas=0;;

bool ejecutando;
typedef struct {
	uint32_t ejecutando;
	BloqueControlProceso pcb;
} EstadoActualDeLaCpu;

EstadoActualDeLaCpu estadoActual;

static const char* PROGRAMA =
		"begin\n"
		"variables a, b\n"
		"a = 3\n"
		"b = 5\n"
		"a = b + 12\n"
		"end\n"
		"\n";

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

bool terminoElPrograma(void){
	return !estadoActual.ejecutando;
}

void informadorDeUsoDeCpu() {
	Paquete* paquete;
	while (true) {
		RecibirPaqueteServidor(socketKernel, CPU, paquete);
		if (paquete->header.tipoMensaje == ESTAEJECUTANDO) {
			int tamDatos = sizeof(uint32_t) * 2;
			void* datos = malloc(tamDatos);
			((uint32_t*) datos)[0] = terminoElPrograma();
			((uint32_t*) datos)[1] = estadoActual.pcb.PID;

			EnviarDatos(socketKernel, CPU, datos, tamDatos);
		}
	}
}

int main(void) {
	indiceDeCodigo = list_create();
	bool DesconectarCPU = false;
	pcb_Create(&pcb, 0); //TODO: sacar el 0 despues de mergear de newMaster

	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();

	socketKernel = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CPU, RecibirHandshake); //TODO: Recibir datos
	socketMemoria = ConectarAServidor(PUERTO_MEMORIA, IP_MEMORIA, MEMORIA, CPU, RecibirHandshake_DeMemoria);

	pthread_t hiloInformadorDeEstado;
	pthread_create(&hiloInformadorDeEstado, NULL, (void*)informadorDeUsoDeCpu, NULL);


	while(!DesconectarCPU) {
		Paquete paquete;
		while (RecibirPaqueteCliente(socketKernel, CPU, &paquete)<=0);
		switch(paquete.header.tipoMensaje) {
		case KILLPROGRAM: //reemplazar KILLPROGRAM por algo acorde, es la señal SIGUSR1 para deconectar la CPU
			DesconectarCPU = true;
			break;
		case ESPCB:	{

				pcb_Receive(&datosCpu,&pcb, socketKernel,cantRafagasAEjecutar);
				estadoActual.pcb = pcb;
				estadoActual.ejecutando = true;
				int i=0;
				while(i< cantRafagasAEjecutar){
					uint32_t* registro = (uint32_t*)list_get(pcb.IndiceDeCodigo,pcb.ProgramCounter);
					char instruccion[registro[1]];
					obtenerLinea(instruccion, registro);
					analizadorLinea(instruccion,&functions,&kernel_functions);
					pcb.cantidadDeRafagasEjecutadas++;
					cantRafagasEjecutadas++;
					i++;
				}
				// Avisar al kernel que terminaste de ejecutar la instruccion
				pcb_Send(CPU, &pcb,socketKernel,cantRafagasEjecutadas);
				estadoActual.pcb = pcb;
				estadoActual.ejecutando = false;
			}
			break;

		}
	}
	pcb_Destroy(&pcb);
	pthread_join(hiloInformadorDeEstado, NULL);
	return 0;
}
