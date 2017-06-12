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


int main(void) {
	indiceDeCodigo = list_create();
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	socketKernel = ConectarAServidor(PUERTO_KERNEL, IP_KERNEL, KERNEL, CPU, RecibirHandshake);
	socketMemoria = ConectarAServidor(PUERTO_MEMORIA, IP_MEMORIA, MEMORIA, CPU, RecibirHandshake);

	//TODO: Recibir PCB del Kernel
	//pcb=Funcion

	//Me mandan un programa a ejecutar
	//Primero tengo que correr la funcion MetadataProgram
	int i;
	char* programa;
	for (i=0; i < pcb.PaginasDeCodigo; i++)
		string_append(&programa, (char*)IM_LeerDatos(socketMemoria,CPU,pcb.PID,i,0,tamPagina));
	t_metadata_program* metaProgram = metadata_desde_literal(programa);

	uint32_t* registro = (uint32_t*)list_get(pcb.IndiceDeCodigo,pcb.ProgramCounter);
	char* instruccion = (char*)IM_LeerDatos(socketMemoria,CPU,pcb.PID,PAGINACODIGOPROGRAMA,registro[0],registro[1]);
	analizadorLinea(instruccion,&functions,&kernel_functions);
	//TODO: Avisar al kernel que terminaste de ejecutar la instruccion

	return 0;
}
