#include "CPU.h"


BloqueControlProceso pcb;


static const char* PROGRAMA =
		"begin\n"
		"variables a, b\n"
		"a = 3\n"
		"b = 5\n"
		"a = b + 12\n"
		"end\n"
		"\n";

AnSISOP_funciones functions = {
  .AnSISOP_definirVariable = primitiva_definirVariable,
  /*.AnSISOP_obtenerPosicionVariable= dummy_obtenerPosicionVariable,
  .AnSISOP_finalizar     = dummy_finalizar,
  .AnSISOP_dereferenciar   = dummy_dereferenciar,
  .AnSISOP_asignar    = dummy_asignar,*/

};


AnSISOP_kernel kernel_functions = { };

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
	uint32_t cantQueFaltaLeer = registro[0] - registro[1];
	uint32_t cantPaginasALeer = registro[0]/TamanioPaginaMemoria;
	uint32_t offset = registro[0] % TamanioPaginaMemoria;
	char* datos;
	int i;
	//TODO: incompleta
	for (i = 0; i < cantPaginasALeer; ++i) {
		datos = (char*)IM_LeerDatos(socketMemoria,CPU,pcb.PID,i, offset, cantQueFaltaLeer);
		string_append(instruccion, datos);
		//cantQueFaltaLeer-=
	}
	free(datos);
}


int main(void) {
	indiceDeCodigo = list_create();
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();

	socketKernel = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CPU, RecibirHandshake);
	socketMemoria = ConectarAServidor(PUERTO_MEMORIA, IP_MEMORIA, MEMORIA, CPU, RecibirHandshake_DeMemoria);

	BloqueControlProceso pcb;
	uint32_t ultimoPID; //TODO: recibirlo por handshake del kernel? o directamente recibir el pid y restarle 1? o cambiar la funcion para q no pida ningun pid y q el kernel lo ponga?
	pcb_Create(&pcb, ultimoPID);
	pcb_Receive(&pcb, socketKernel); //Recibir PCB del Kernel

	//Me mandan un programa a ejecutar
	//Primero tengo que correr la funcion MetadataProgram
	char programa[pcb.PaginasDeCodigo * TamanioPaginaMemoria];
	int i;
	for (i=0; i < pcb.PaginasDeCodigo; i++){
		char* datos = (char*)IM_LeerDatos(socketMemoria,CPU,pcb.PID,i,0,TamanioPaginaMemoria);
		memccpy(&programa[i*TamanioPaginaMemoria], datos, '\0', TamanioPaginaMemoria); //tal vez hay que usar una funcion diferente para copiarlo
		free(datos);
	}

	t_metadata_program* metaProgram = metadata_desde_literal(programa);

	//TODO: ¿mandarle el metaProgram al kernel y recibir el pcb otra vez?
	pcb_Send(socketKernel, CPU, &pcb);
	pcb_Receive(&pcb, socketKernel);
	//Convertir el instrucciones_Serializer en la lista del indice de codigo
	uint32_t* registro = (uint32_t*)list_get(pcb.IndiceDeCodigo,pcb.ProgramCounter);

	char instruccion[registro[1]-registro[0]];
	obtenerLinea(&instruccion, registro);

	analizadorLinea(instruccion,&functions,&kernel_functions);

	//TODO: Avisar al kernel que terminaste de ejecutar la instruccion

	pcb_Destroy(&pcb);

	return 0;
}
