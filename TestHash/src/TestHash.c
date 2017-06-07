#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <limits.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <math.h>
#define LAMBDA(c_) ({ c_ _;}) //Para funciones lamda

int Hash(int x, int y, int MARCOS){
	x--;
	int r = ((x + y) * (x + y + 1)) / 2 + y;
	while(r>MARCOS)
		r -= MARCOS;
	return r;
}

int main(void) {
	t_list* lista;
	lista = list_create();
	int pid, pag, MARCOS, r, i, j, k, l;
	puts("ingrese maximos de: pid pag marcos");
	scanf("%i",&pid);
	scanf("%i",&pag);
	scanf("%i",&MARCOS);

	//harcodeados
	/*
	pid= 60; //Dominio de X
	pag= 50; //Dominio de Y
	MARCOS=120; //Dominio de F(X,Y)
	*/
	printf("\n");
	for(i=1; i<pid; i++){
		printf("\n");
		for(j=0; j<pag; j++){
			r=Hash(i, j, MARCOS);
			int* elem = malloc(sizeof(int));
			*elem = r;
			list_add(lista, elem);
			printf("%i ",r);
		}
	}
	int cont = 0;
	printf("\n\n ");
	list_sort(lista, LAMBDA(bool _(void* elem1, void* elem2) { return *(int*)elem1 < *(int*)elem2; }));

	for(l=0; l < list_size(lista); l++){
		printf("%i. ", *(int*)list_get(lista, l));
	}
	printf("\n\n");
	for(k=1; k < *(int*)list_get(lista, list_size(lista)-1); k++){
		unsigned p = list_count_satisfying(lista, LAMBDA(bool _(void* elem) { return *(int*)elem == k; }));
		if(p!=0)
			printf("%u, ",p);
		else
			cont++;
	}
	printf("\n\n%i",cont);
	list_destroy_and_destroy_elements(lista, free);
	return 0;
}
