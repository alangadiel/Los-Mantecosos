#include <parser/metadata_program.h>
#include "Primitivas.h"

char* IP_KERNEL;
int PUERTO_KERNEL;
char* IP_MEMORIA;
int PUERTO_MEMORIA;
int socketKernel;
int socketMemoria;
uint32_t numeroDePagina;
BloqueControlProceso pcb;
uint32_t ultimoOffSetVariablesStack;
typedef struct {
	uint32_t byteComienzo;
	uint32_t longitud;
} Instruccion;
t_list* indiceDeCodigo;

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

	Paquete* paquete = malloc(sizeof(Paquete));
	uint32_t datosRecibidos = RecibirPaqueteCliente(socketKernel, CPU, paquete);
	while(datosRecibidos<=0){
		datosRecibidos = RecibirPaqueteCliente(socketKernel, CPU, paquete);
	}
	if (datosRecibidos > 0)
	{
		switch(paquete -> header.tipoMensaje)
		{
			case ESSTRING:
				if (strcmp(paquete -> header.emisor, MEMORIA) == 0)
				{
					/*printf("Texto recibido: %s", (char*)paquete -> Payload);
					analizadorLinea(PROGRAMA, &functions, &kernel_functions);
					//IM_GuardarDatos()
					pcb.ProgramCounter++;*/
				}
			break;

			case ESPCB:
				if (strcmp(paquete -> header.emisor, KERNEL) == 0)
				{
					//Me mandan un programa a ejecutar
					//Primero tengo que correr la funcion MetadataProgram
					char* programa = (char*)IM_LeerDatos(socketMemoria,CPU,pcb.PID,PAGINACODIGOPROGRAMA,0,512);
					t_metadata_program* metaProgram = metadata_desde_literal(programa);
					pcb = *(BloqueControlProceso*) paquete -> Payload;
					uint32_t* registro = (uint32_t*)list_get(pcb.IndiceDeCodigo,pcb.ProgramCounter);
					char* instruccion = (char*)IM_LeerDatos(socketMemoria,CPU,pcb.PID,PAGINACODIGOPROGRAMA,registro[0],registro[1]);
					analizadorLinea(instruccion,&functions,&kernel_functions);
					//TODO: Avisar al kernel que terminaste de ejecutar la instruccion

				}
			break;
		}
	}
	//Hago free
	free(paquete -> Payload);
	free(paquete);
	return 0;
}
