#include "Helper.h"
#include "SocketsL.h"

char* getWord(char* string, int pos) {
	char delimiter[] = " ";
	char *word, *context;
	int inputLength = strlen(string);
	char *inputCopy = (char*) calloc(inputLength + 1, sizeof(char));
	strncpy(inputCopy, string, inputLength);
	if (pos == 0) {
		return strtok_r(inputCopy, delimiter, &context);
	} else {
		int i;
		for (i = 1; i <= pos; i++) {
			word = strtok_r(NULL, delimiter, &context);
		}
	}
	return word;
}

char* integer_to_string(int x) {
	char* buffer = malloc(sizeof(char) * sizeof(int) * 4 + 1);
	if (buffer) {
		sprintf(buffer, "%d", x);
	}
	return buffer; // caller is expected to invoke free() on this buffer to release memory
}

int GetTamanioArchivo(FILE * f) {
	fseek(f, 0L, SEEK_END);
	int size = ftell(f);
	return size;
}

void pcb_Create(BloqueControlProceso* pcb,int* ultimoPid){
	pcb->PID = *ultimoPid+1;
	pcb->ProgramCounter = 0;
	pcb->PaginasDeCodigo=0;
	pcb->IndiceDeCodigo = list_create();
	pcb->IndiceDeEtiquetas = dictionary_create();
	pcb->IndiceDelStack = list_create();
	//pcb->ExitCode = ? si todavia no finalizó
	(*ultimoPid)++;
}

void pcb_Destroy(BloqueControlProceso* pcb){
	//Creo el pcb y lo guardo en la lista de nuevos
	list_destroy_and_destroy_elements(pcb->IndiceDeCodigo,free);
	list_destroy_and_destroy_elements(pcb->IndiceDelStack,free);
	dictionary_destroy_and_destroy_elements(pcb->IndiceDeEtiquetas,free);
	free(pcb);
}
