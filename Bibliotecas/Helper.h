#ifndef HELPER_
#define HELPER_
#define LAMBDA(c_) ({ c_ _;}) //Para funciones lamda

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>



typedef struct {
 	uint32_t NumeroDePagina;
 	uint32_t Offset; //Desplazamiento
	uint32_t Tamanio;
 }__attribute__((packed)) PosicionDeMemoria;

 typedef struct {
 	char ID; //el nombre de la variable
 	PosicionDeMemoria Posicion;
 }__attribute__((packed)) Variable; //de AnSISOP

 typedef struct {
 	t_list* Argumentos; //lista de uInt32
 	t_list* Variables; //Desplazamiento, lista de uInt32
 	uint32_t DireccionDeRetorno;
 	PosicionDeMemoria PosVariableDeRetorno;
 }__attribute__((packed)) IndiceStack;


 typedef struct {
   	uint32_t PID;
   	uint32_t ProgramCounter;
   	uint32_t PaginasDeCodigo;
  	uint32_t* IndiceDeCodigo[2]; //matriz de 2xCantInstrucciones
  	uint32_t* IndiceDeEtiquetas[2];//matriz de 2xCantEtiquetas
  	t_list* IndiceDelStack; //lista de IndiceStack
   	int ExitCode;
   }__attribute__((packed)) BloqueControlProceso;

typedef struct {
	uint32_t* Atributos;
	char* buffer;

}__attribute__((packed)) BufferArchivo;

char* getWord(char* string, int pos);
char* integer_to_string(int x);
char* obtenerTiempoString(time_t t);
int GetTamanioArchivo(FILE * f);

#endif /* HELPER_*/
