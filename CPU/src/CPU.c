#include "CPU.h"


BloqueControlProceso pcb;
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

void obtenerLinea(char** instruccion, uint32_t* registro){
	uint32_t cantQueFaltaLeer = registro[1];
	uint32_t cantPaginasALeer = registro[0]/TamanioPaginaMemoria;
	uint32_t offset = registro[0] % TamanioPaginaMemoria;
	char* datos;
	int i;
	//TODO: incompleta
	for (i = 0; i < cantPaginasALeer; ++i) {
		datos = (char*)IM_LeerDatos(socketMemoria,CPU,pcb.PID,i, offset, cantQueFaltaLeer);
		string_append(instruccion, datos);
		cantQueFaltaLeer-=TamanioPaginaMemoria;
	}
	free(datos);
}

bool terminoElPrograma(void){
	return !estadoActual.ejecutando;
}

void informadorDeUsoDeCpu() {
	Paquete* paquete;
	RecibirPaqueteServidor(socketKernel, CPU, paquete);
	while (paquete == NULL) {
		RecibirPaqueteServidor(socketKernel, CPU, paquete);
	}
	if (paquete->header.tipoMensaje == ESTAEJECUTANDO) {
		int tamDatos = sizeof(uint32_t) * 2;
		void* datos = malloc(tamDatos);

		((uint32_t*) datos)[0] = terminoElPrograma();
		((uint32_t*) datos)[1] = estadoActual.pcb.PID;

		EnviarDatos(socketKernel, CPU, datos, tamDatos);
	}
}

void crearIndiceDeCodigo(t_metadata_program* metaProgram){
	int i;
	for (i = 0; i < metaProgram->instrucciones_size; i++) {
		Instruccion instruccion;
		instruccion.longitud = metaProgram->instrucciones_serializado[i*sizeof(t_intructions)]->offset;
		instruccion.byteComienzo = metaProgram->instrucciones_serializado[i*sizeof(t_intructions)]->start;
		list_add(indiceDeCodigo, instruccion);
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

	pthread_t userInterface;
	pthread_create(&userInterface, NULL, (void*)informadorDeUsoDeCpu, NULL);
	pthread_join(userInterface, NULL);

	while(!DesconectarCPU) {
		Paquete paquete;
		while (RecibirPaqueteCliente(socketKernel, CPU, &paquete)<=0);
		switch(paquete.header.tipoMensaje) {
		case KILLPROGRAM: //reemplazar KILLPROGRAM por algo acorde, es la señal SIGUSR1 para deconectar la CPU
			DesconectarCPU = true;
			break;
		case ESPCB:	{

			//Ejecutar linea:
			/*if (QUANTUM==0){ //FIFO
			 * while(!end of codigo){
			 * }
			 *
			} else { //RR
			while(!end of codigo && !end of quantum){
			}
			}
			*/
			//Recibo el PCB del Kernel
			pcb_Receive(&pcb, socketKernel);
			estadoActual.pcb = pcb;
			estadoActual.ejecutando = true;

			uint32_t* registro = (uint32_t*)list_get(pcb.IndiceDeCodigo,pcb.ProgramCounter);

			char instruccion[registro[1]];
			obtenerLinea(&instruccion, registro);

			analizadorLinea(instruccion,&functions,&kernel_functions);
			//pc++
			// Avisar al kernel que terminaste de ejecutar la instruccion
			pcb_Send(socketKernel, CPU, &pcb);
			estadoActual.pcb = pcb;
			estadoActual.ejecutando = false;
		}
			break;

		case ESARCHIVO: {//reemplazar ESARCHIVO por algo acorde, porque en realidad manda un PCB.

			//Iniciar el programa:
			//Recibo el PCB del Kernel
			pcb_Receive(&pcb, socketKernel);
			//Leo el codigo de la Memoria
			char programa[pcb.PaginasDeCodigo * TamanioPaginaMemoria];
			int i;
			for (i=0; i < pcb.PaginasDeCodigo; i++){
				char* datos = (char*)IM_LeerDatos(socketMemoria,CPU,pcb.PID,i,0,TamanioPaginaMemoria);
				memccpy(&programa[i*TamanioPaginaMemoria], datos, '\0', TamanioPaginaMemoria); //tal vez hay que usar una funcion diferente para copiarlo
				free(datos);
			}
			//Parseo el codigo
			t_metadata_program* metaProgram = metadata_desde_literal(programa);
			//Convertir el instrucciones_Serializer en la lista del indice de codigo
			crearIndiceDeCodigo(metaProgram);
			//enviar PCB lleno a kernel
			pcb_Send(socketKernel, CPU, &pcb);
		}
			break;

		}
	}
	pcb_Destroy(&pcb);

	return 0;
}
