#include "SocketsL.h"
#include "Helper.h"
#include "Service.h"


//Tipos de ExitCode
#define FINALIZACIONNORMAL
#define NOSEPUDIERONRESERVARRECURSOS -1
#define DESCONECTADODESDECOMANDOCONSOLA -7
#define SOLICITUDMASGRANDEQUETAMANIOPAGINA -8

#define SIZEMETADATA 5


int pidAFinalizar;
int ultimoPID=0;
int socketConMemoria;
int socketConFS;
int ultimoFD = 3;
double TamanioPagina;


typedef struct  {
 uint32_t size;
 bool isFree;
} HeapMetadata;

typedef struct {
	FILE * file;
	BloqueControlProceso pcb;
} PeticionTamanioStack;

typedef struct {
	uint32_t pid;
	uint32_t nroPagina; 		//Numero de pagina DEL PROCESO
	uint32_t espacioDisponible;
} PaginaDelProceso;

typedef struct {
	uint32_t numero;
	t_list* HeapsMetadata;
} Pagina;

typedef struct {
	uint32_t PID;
	uint32_t FD; //Empieza a partir de FD = 3. El 0, 1 y 2 estan reservados por convencion.
	uint32_t offsetArchivo;
	char* flag;
	uint32_t globalFD;
} archivoProceso;

typedef struct {
	char* pathArchivo;
	int cantAperturas;
} archivoGlobal;

//Variables archivo de configuracion
int PUERTO_PROG;
int PUERTO_CPU;
char* IP_MEMORIA;
int PUERTO_MEMORIA;
char* IP_FS;
int PUERTO_FS;
int QUANTUM;
int QUANTUM_SLEEP;
char* ALGORITMO;
int GRADO_MULTIPROG;
char* SEM_IDS[4];
int SEM_INIT[100];
char* SHARED_VARS[100];
int STACK_SIZE;
char* IP_PROG;
uint32_t PidAComparar;
t_list* Nuevos;
t_list* Finalizados;
t_list* Bloqueados;
t_list* Ejecutando;
t_list* Listos;
t_list* Estados;
t_list* ListaPCB;
t_list* EstadosConProgramasFinalizables;
t_list* PaginasPorProceso;
t_list* Paginas;
t_list* ArchivosGlobales;
t_list* ArchivosProcesos;


int RecorrerHastaEncontrarUnMetadataUsed(void* datosPagina){
	bool encontroOcupado=false;
	uint32_t offsetOcupado=0;
	//Recorro hasta encontrar el primer bloque ocupado
		while(offsetOcupado<TamanioPagina-sizeof(HeapMetadata) && encontroOcupado == false){
			//Recorro el buffer obtenido
			uint32_t size;
			bool isfree;
			size = *(uint32_t*)(datosPagina+offsetOcupado);
			isfree = *(bool*)(datosPagina+offsetOcupado+sizeof(uint32_t));
			if(isfree==false){
				//Si encuentra un metadata free, freno
				encontroOcupado = true;
			}
			else{
				//Aumento el puntero de acuerdo al tamaño correspondiente al bloque existente
				offsetOcupado+=(sizeof(HeapMetadata)+ size);
			}
		}
		if(encontroOcupado==true)
			return offsetOcupado;
		else
			return -1;
}
BloqueControlProceso* FinalizarPrograma(t_list* lista,int pid,int tipoFinalizacion, int index, int socketFD) {
	BloqueControlProceso* pcbRemovido = NULL;
	bool hayEstructurasNoLiberadas=false;
	pcbRemovido = (BloqueControlProceso*)list_remove_by_condition(lista, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pid; }));
	if(pcbRemovido!=NULL) {
		pcbRemovido->ExitCode=tipoFinalizacion;
		list_add(Finalizados,pcbRemovido);
		//Analizo si el proceso tiene Memory Leaks o no
		t_list* pagesProcess= list_filter(PaginasPorProceso,
							LAMBDA(bool _(void* item) {return ((PaginaDelProceso*)item)->pid == pid;}));
		if(list_size(pagesProcess)>0){
			int i=0;
			while(i < list_size(pagesProcess) && hayEstructurasNoLiberadas==false){
				PaginaDelProceso* elem = (PaginaDelProceso*)list_get(PaginasPorProceso,i);
				//Me fijo si hay metadatas en estado "used" en cada pagina
				void* datosPagina = IM_LeerDatos(socketFD,KERNEL,elem->pid,elem->nroPagina,0,TamanioPagina);
				int result = RecorrerHastaEncontrarUnMetadataUsed(datosPagina);
				if(result>=0){
					//Hay algun metadata que no se libero
					hayEstructurasNoLiberadas=true;
				}
			}
			if(hayEstructurasNoLiberadas==true)
				printf("MEMORY LEAKS: El proceso %d no liberó todas las estructuras de memoria dinámica que solicitó",pid);
			else
				printf("El proceso %d liberó todas las estructuras de memoria dinamica",pid);
		}

		if(index == INDEX_LISTOS) {
			IM_FinalizarPrograma(socketFD, KERNEL, pid);
		}
	}
	return pcbRemovido;
}

uint32_t ActualizarMetadata(uint32_t PID,uint32_t nroPagina,uint32_t cantAReservar,int socketFD){
	uint32_t cantTotal =cantAReservar + sizeof(HeapMetadata);
	//Obtengo la pagina en cuestion donde actualizar la metadata
	void* datosPagina = IM_LeerDatos(socketFD,KERNEL,PID,nroPagina,0,TamanioPagina);
	uint32_t offset = 0;
	bool estado;
	uint32_t sizeBloque;
	bool encontroLibre = false;
	uint32_t punteroAlPrimerBloqueDisponible;

	int offsetOcupado = RecorrerHastaEncontrarUnMetadataUsed(datosPagina);
	if(offsetOcupado<0) //HUBO ERROR
		punteroAlPrimerBloqueDisponible = -1;
	else
		punteroAlPrimerBloqueDisponible= offsetOcupado + sizeof(HeapMetadata);

	//Recorro hasta encontrar el primer bloque libre
	while(offset<TamanioPagina-sizeof(HeapMetadata) && encontroLibre == false){
		//Recorro el buffer obtenido
		sizeBloque = *(uint32_t*)(datosPagina+offset);
		estado = *(bool*)(datosPagina+offset+sizeof(uint32_t));
		if(estado==true){
			//Si encuentra un metadata free, freno
			encontroLibre = true;
		}
		else{
			//Aumento el puntero de acuerdo al tamaño correspondiente al bloque existente
			offset+=(sizeof(HeapMetadata)+ sizeBloque);
		}

	}
	if(encontroLibre ==true) {
		// Se encontro un bloque libre
		uint32_t diferencia= sizeBloque-cantTotal;
		//Actualizo el metadata de acuerdo a la cantidad de bytes a reservar
		HeapMetadata metaOcupado;
		metaOcupado.isFree = false;
		metaOcupado.size = cantAReservar;
		IM_GuardarDatos(socketFD,KERNEL,PID,nroPagina,offset,sizeof(HeapMetadata),&metaOcupado);


		//Creo el metadata para lo que queda libre del espacio que use
		uint32_t offsetMetadataLibre = offset+sizeof(HeapMetadata)+cantAReservar;
		HeapMetadata metaLibre;
		metaLibre.isFree=true;
		metaLibre.size = diferencia;
		IM_GuardarDatos(socketFD,KERNEL,PID,nroPagina,offsetMetadataLibre,sizeof(HeapMetadata),&metaLibre);

		return punteroAlPrimerBloqueDisponible;
	}
	else {
		//No se encontro ningun bloque donde reservar memoria dinamica
		return -1;
	}


}

uint32_t SolicitarHeap(uint32_t PID,uint32_t cantAReservar,int socket){
	uint32_t cantTotal = cantAReservar+sizeof(HeapMetadata);
	uint32_t punteroAlPrimerDisponible=0;
	//Como maximo, podes solicitar reservar: TamañoPagina -10(correspondiente a los metedatas iniciales)
	if(cantTotal <= TamanioPagina-sizeof(HeapMetadata)*2){
		void* result = NULL;
		//Busco en las paginas asignadas a ese proceso, a ver si hay alguna con tamaño suficiente
		result = list_find(PaginasPorProceso,
				LAMBDA(bool _(void* item) {
			return ((PaginaDelProceso*)item)->pid == PID && ((PaginaDelProceso*)item)->espacioDisponible >= cantTotal;
		}));
		if(result!=NULL){ 		//Se encontro la pagina con tamaño disponible
			//Actualizar el tamaño disponible
			PaginaDelProceso* paginaObtenida = (PaginaDelProceso*)result;
			paginaObtenida->espacioDisponible -= cantTotal;
			//Obtengo la pagina en cuestion y actualizo el metadata
			punteroAlPrimerDisponible = ActualizarMetadata(PID,paginaObtenida->nroPagina,cantTotal,socket);

		}
		else  		//No hay una pagina del proceso utilizable
		{
			//Obtengo el ultimo numero de pagina, de ese PID
			t_list* pagesProcess= list_filter(PaginasPorProceso,
					LAMBDA(bool _(void* item) {return ((PaginaDelProceso*)item)->pid == PID;}));
			int i=0;
			int maximoNroPag=0;
			for (i = 0; i < list_size(pagesProcess); i++) {
				PaginaDelProceso* elem = (PaginaDelProceso*)list_get(PaginasPorProceso,i);
				if(maximoNroPag< elem->nroPagina)	maximoNroPag = elem->nroPagina;
			}
			PaginaDelProceso nuevaPPP;
			nuevaPPP.nroPagina = maximoNroPag+1;
			nuevaPPP.espacioDisponible = TamanioPagina;
			nuevaPPP.pid = PID;
			list_add(PaginasPorProceso,&nuevaPPP);
			//Le pido al Proceso Memoria que me guarde esta pagina para el proceso en cuestion
			IM_AsignarPaginas(socket,KERNEL,PID,1);
			punteroAlPrimerDisponible = ActualizarMetadata(PID,nuevaPPP.nroPagina,cantTotal,socket);
			//Destruyo la lista PagesProcess
			list_destroy_and_destroy_elements(pagesProcess,free);
		}
	}
	else{ //Debe finalizar el programa pq quiere reservar mas de lo permitido
		FinalizarPrograma(Ejecutando,PID,SOLICITUDMASGRANDEQUETAMANIOPAGINA,INDEX_EJECUTANDO,socket);
		//TODO: Avisar a la CPU del programa finalizado
	}
	return punteroAlPrimerDisponible;
}
void SolicitudLiberacionDeBloque(int socketFD,uint32_t pid,PosicionDeMemoria pos){
	void* datosPagina = IM_LeerDatos(socketFD,KERNEL,pid,pos.NumeroDePagina,0,TamanioPagina);
	uint32_t offSetMetadataAActualizar = pos.Offset- sizeof(HeapMetadata);
	uint32_t sizeBloqueALiberar = *(uint32_t*)(datosPagina+offSetMetadataAActualizar);
	HeapMetadata heapMetaAActualizar;
	heapMetaAActualizar.isFree = true;
	heapMetaAActualizar.size = sizeBloqueALiberar;
	//Pongo el bloque como liberado
	//Me fijo el estado del siguiente Metadata(si esta Free o Used)
	uint32_t offsetMetadataSiguiente = pos.Offset+sizeBloqueALiberar;
	uint32_t sizeDelMetadataSiguiente = *(uint32_t*)(datosPagina+offsetMetadataSiguiente);
	bool estadoDelMetadataSiguiente = *(uint32_t*)(datosPagina+offsetMetadataSiguiente+sizeof(uint32_t));
	if(estadoDelMetadataSiguiente==false) {
		//Esta ocupado: solo actualizo el metadata del bloque que me liberaron
		IM_GuardarDatos(socketFD,KERNEL,pid,pos.NumeroDePagina,offSetMetadataAActualizar,sizeof(HeapMetadata),&heapMetaAActualizar);
	}
	else {
		//Esta libre: puedo compactarlos como un metadata solo
		heapMetaAActualizar.size += sizeDelMetadataSiguiente;
		IM_GuardarDatos(socketFD,KERNEL,pid,pos.NumeroDePagina,offSetMetadataAActualizar,sizeof(HeapMetadata),&heapMetaAActualizar);
		char* datosBasura= malloc(5);
		strncpy(datosBasura,"basur",5);
		IM_GuardarDatos(socketFD,KERNEL,pid,pos.NumeroDePagina,offsetMetadataSiguiente,sizeof(heapMetaAActualizar),datosBasura);
		free(datosBasura);
	}
	//Si estan todos los bloques de la pagina libres, hay que liberar la pagina entera
	uint32_t offSet= RecorrerHastaEncontrarUnMetadataUsed(datosPagina);
	if(offSet< 0){
		//No se encontro algun bloque ocupado: Hay que liberar la pagina
		IM_LiberarPagina(socketFD,KERNEL,pid,pos.NumeroDePagina);
	}


}

char* ObtenerTextoDeArchivoSinCorchetes(FILE* f) //Para obtener los valores de los arrays del archivo de configuracion
{
	char buffer[10000];
	char *line = fgets(buffer,sizeof buffer,f);
	int length = string_length(line)-3;
	char *texto = string_substring(line,1,length);

	texto  = strtok(texto,",");

	return texto;
}


void ObtenerTamanioPagina(int socketFD){
	Paquete* datosInicialesMemoria = malloc(sizeof(Paquete));
	uint32_t datosRecibidos = RecibirPaqueteCliente(socketFD,KERNEL,datosInicialesMemoria);
	if(datosRecibidos>0){
		TamanioPagina = *((uint32_t*)datosInicialesMemoria->Payload);
	}
	free(datosInicialesMemoria->Payload+1);
	free(datosInicialesMemoria);

}

void MostrarProcesosDeUnaLista(t_list* lista,char* discriminator){
	printf("Procesos de la lista: %s \n",discriminator);
	int index=0;
	for (index = 0; index < list_size(lista); index++) {
		BloqueControlProceso* proceso = (BloqueControlProceso*)list_get(lista,index);

		if (strcmp(discriminator, FINALIZADOS) == 0) {
			printf("\tProceso N°: %d",proceso->PID);
			obtenerError(proceso->ExitCode);
		}
		else {
			printf("\tProceso N°: %d\n",proceso->PID);
		}
	}
}

void obtenerValoresArchivoConfiguracion()
{
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
				if (contadorDeVariables == 14)
				{
					char buffer[10000];

					IP_PROG = fgets(buffer, sizeof buffer, file);
					strtok(IP_PROG, "\n");
					contadorDeVariables++;
				}

				if (contadorDeVariables == 13)
				{
					fscanf(file, "%i", &STACK_SIZE);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 12)
				{
					char* texto = ObtenerTextoDeArchivoSinCorchetes(file);
					int i = 0;

					while (texto != NULL)
					{
						SHARED_VARS[i++] = texto;
						texto = strtok (NULL, ",");
					}

					contadorDeVariables++;
				}

				if (contadorDeVariables == 11)
				{
					char* texto = ObtenerTextoDeArchivoSinCorchetes(file);
					int i = 0;

					while (texto != NULL)
					{
						SEM_INIT[i++] = atoi(texto);
						texto = strtok (NULL, ",");
					}

					contadorDeVariables++;
				}

				if (contadorDeVariables == 10){
					char * texto = ObtenerTextoDeArchivoSinCorchetes(file);
					int i = 0;

					while (texto != NULL)
					{
						SEM_IDS[i++] = texto;
						texto = strtok (NULL, ",");
					}

					contadorDeVariables++;
				}

				if (contadorDeVariables == 9)
				{
					fscanf(file, "%i", &GRADO_MULTIPROG);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 8)
				{
					char buffer[10000];

					ALGORITMO = fgets(buffer, sizeof buffer, file);
					strtok(ALGORITMO, "\n");

					contadorDeVariables++;
				}

				if (contadorDeVariables == 7)
				{
					fscanf(file, "%i", &QUANTUM_SLEEP);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 6)
				{
					fscanf(file, "%i", &QUANTUM);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 5)
				{
					fscanf(file, "%i", &PUERTO_FS);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 4)
				{
					char buffer[10000];

					IP_FS = fgets(buffer, sizeof buffer, file);
					strtok(IP_FS, "\n");

					contadorDeVariables++;
				}

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
					fscanf(file, "%i", &PUERTO_CPU);
					contadorDeVariables++;
				}

				if (contadorDeVariables == 0)
				{
					fscanf(file, "%i", &PUERTO_PROG);
					contadorDeVariables++;
				}
			}
		}

		fclose(file);
	}
}
void imprimirArchivoConfiguracion()
{
	int c;
	FILE *file;

	file = fopen("ArchivoConfiguracion.txt", "r");

	if (file)
	{
		while ((c = getc(file)) != EOF)
		{
			putchar(c);
		}

		fclose(file);
	}
}
void CrearListas(){
	Nuevos = list_create();
	Finalizados= list_create();
	Bloqueados= list_create();
	Ejecutando= list_create();
	Listos= list_create();
	//Creo una lista de listas
	Estados = list_create();
	EstadosConProgramasFinalizables = list_create();
	ArchivosGlobales = list_create();
	list_add(EstadosConProgramasFinalizables,Nuevos);
	list_add(EstadosConProgramasFinalizables,Listos);
	list_add(EstadosConProgramasFinalizables,Ejecutando);
	list_add(EstadosConProgramasFinalizables,Bloqueados);
	list_add_all(Estados,EstadosConProgramasFinalizables);
	list_add(Estados,Finalizados);
}
void LimpiarListas(){
	list_destroy_and_destroy_elements(Nuevos,free);
	list_destroy_and_destroy_elements(Listos,free);
	list_destroy_and_destroy_elements(Ejecutando,free);
	list_destroy_and_destroy_elements(Bloqueados,free);
	list_destroy_and_destroy_elements(Finalizados,free);
	list_destroy_and_destroy_elements(Estados,free);
	list_destroy_and_destroy_elements(EstadosConProgramasFinalizables,free);
	list_destroy_and_destroy_elements(ArchivosGlobales,free);
	list_destroy_and_destroy_elements(ArchivosProcesos,free);
}
void ConsultarEstado(int pidAConsultar){
	int i =0;
	void* result=NULL;
	//Busco el proceso en todas las listas
	while(i<list_size(Estados) && result==NULL){
		t_list* lista = (t_list* )list_get(Estados,i);
		result = list_find(lista,LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pidAConsultar; }));
		i++;
	}
	if(result==NULL)
		printf("No se encontro el proceso a consultar. Intente nuevamente");
	else{
		BloqueControlProceso* proceso = (BloqueControlProceso*)result;
		printf("Proceso N°: %d \n",proceso->PID);
		//printf("Indice de codigo: %d \n",proceso->IndiceDeCodigo);
		//printf("Tamaño del stack: %d \n",proceso->IndiceStack);
		printf("Paginas de codigo: %d \n",proceso->PaginasDeCodigo);
		printf("Contador de programa: %d \n",proceso->ProgramCounter);
	}
}

void syscallWrite(int socketFD) {
	if(socketFD == 1) {
		char orden[100];
		char texto[100];

		printf("\n\nIngrese una orden privilegiada: \n");
		scanf("%s", orden);
		char* command = getWord(orden, 0);

		if(strcmp(command,"write") == 0) {
			printf("\n\nIngrese texto a imprimir: \n");
			scanf("%s", texto);
			printf("%s", texto);
			char* imprimir;
			strcpy(imprimir,"imprimir ");
			string_append(&imprimir,texto);
			EnviarMensaje(socketFD,imprimir,KERNEL);
			free(imprimir);
		}
		else{
			printf("System Call no definida\n");
		}
	}
}
void MostrarTodosLosProcesos(){
	MostrarProcesosDeUnaLista(Nuevos,NUEVOS);
	MostrarProcesosDeUnaLista(Listos,LISTOS);
	MostrarProcesosDeUnaLista(Ejecutando,EJECUTANDO);
	MostrarProcesosDeUnaLista(Bloqueados,BLOQUEADOS);
	MostrarProcesosDeUnaLista(Finalizados,FINALIZADOS);
}

void KillProgramDeUnaLista(t_list* lista,BloqueControlProceso* pcb,int tipoFinalizacion, int index, int socket) {
	FinalizarPrograma(lista,pcb->PID,tipoFinalizacion, index, socket);
}
bool KillProgram(int pidAFinalizar,int tipoFinalizacion, int socket){
	int i =0;
	void* result = NULL;
	while(i<list_size(EstadosConProgramasFinalizables) && result==NULL){
		t_list* lista = list_get(EstadosConProgramasFinalizables,i);
		//TODO: Ver como eliminar un programa en estado ejecutando
		if(i!=2)  //2 == EJECUTANDO
			result = FinalizarPrograma(lista,pidAFinalizar,tipoFinalizacion, i, socket);
		i++;
	}
	if(result==NULL){
		printf("No se encontro el programa finalizar");
		return false;
	}
	return true;

}
void PonerElProgramaComoListo(BloqueControlProceso* pcb,Paquete* paquete,int socketFD,double tamanioTotalPaginas){

		pcb->PaginasDeCodigo = (uint32_t)tamanioTotalPaginas;
		printf("Cant paginas asignadas: %d \n",pcb->PaginasDeCodigo);
		//Saco el programa de la lista de NEW y  agrego el programa a la lista de READY
		list_remove_by_condition(Nuevos, LAMBDA(bool _(void* item) { return ((BloqueControlProceso*)item)->PID == pcb->PID; }));
		list_add(Listos, pcb);
		printf("El programa %d se cargo en memoria \n",pcb->PID);
}

void cargarEnTablasArchivos(char* path, uint32_t PID, char* permisos)
{
	void* result = NULL;

	result = (archivoGlobal*) list_find(ArchivosGlobales, LAMBDA(bool _(void* item) { return ((archivoGlobal*) item)->pathArchivo == path; }));

	archivoGlobal* archivoGlob = malloc(sizeof(archivoGlobal)); //El free se hace en limpiar listas

	if(result == NULL) //No hay un archivo global con ese path
	{
		archivoGlob->pathArchivo = path;
		archivoGlob->cantAperturas = 1;

		list_add(ArchivosGlobales, archivoGlob);
	}
	else
	{
		archivoGlob = (archivoGlobal*)result;

		archivoGlob->cantAperturas++;

		int index;

		for(int i = 0; i < list_size(ArchivosGlobales); i++)
		{
			archivoGlobal* arch = list_get(ArchivosGlobales, i);

			if(arch->pathArchivo == path)
			{
				index = i;
			}
		}

		list_replace(ArchivosGlobales, index, archivoGlob);
	}

	int i = 0;

	result = NULL;

	while(i < list_size(ArchivosProcesos) && result == NULL)
	{
		t_list* lista = list_get(ArchivosProcesos, i);

		result = (archivoProceso*) list_find(lista, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID; }));

		i++;
	}

	archivoProceso* archivoProc = malloc(sizeof(archivoProceso)); //El free se hace en limpiar listas

	int index;

	for(int j = 0; j < list_size(ArchivosGlobales); j++)
	{
		archivoGlobal* arch = list_get(ArchivosGlobales, j);

		if(arch->pathArchivo == path)
		{
			index = j;
		}
	}

	if(result == NULL) //No hay ninguna lista de archivos para ese proceso porque no habia abierto ningun archivo todavia
	{
		archivoProc->PID = PID;
		archivoProc->FD = ultimoFD;
		archivoProc->flag = permisos;
		archivoProc->offsetArchivo = 0;
		archivoProc->globalFD = index;

		ultimoFD++;

		t_list* listaArchivoProceso = list_create();

		list_add(listaArchivoProceso, archivoProc);

		list_add(ArchivosProcesos, listaArchivoProceso);
	}
	else //Hay una lista para ese proceso, entonces solo agrego un archivo
	{
		archivoProc->PID = PID;
		archivoProc->FD = ultimoFD;
		archivoProc->flag = permisos;
		archivoProc->offsetArchivo = 0;
		archivoProc->globalFD = index;

		ultimoFD++;

		t_list* listaArchivoProceso = list_get(ArchivosProcesos, i-1);

		list_add(listaArchivoProceso, archivoProc);
	}
}

uint32_t abrirArchivo(char* path, uint32_t PID, char* permisos)
{
	uint32_t archivoEstaCreado = FS_ValidarPrograma(socketConFS, KERNEL, path);

	if(archivoEstaCreado == 1)
	{
		cargarEnTablasArchivos(path, PID, permisos);
	}
	else
	{
		if(strstr(*permisos, "c") != NULL)
		{
			FS_CrearPrograma(socketConFS, KERNEL, path);
			cargarEnTablasArchivos(path, PID, permisos);
		}
		else
		{
			//Finalizar ejecucion del proceso, liberar recursos y poner exitCode = -20 (El programa intentó crear un archivo sin permisos)
		}
	}

	return archivoEstaCreado;
}

uint32_t leerArchivo(uint32_t FD, uint32_t PID, uint32_t sizeArchivo, char* permisos)
{
	if(strstr(*permisos, "r") != NULL)
	{
		void* result = NULL;
		int i = 0;

		while(i < list_size(ArchivosProcesos) && result == NULL)
		{
			t_list* lista = list_get(ArchivosProcesos, i);

			result = (archivoProceso*) list_find(lista, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

			i++;
		}

		if(result != NULL)
		{
			archivoProceso* archProc = malloc(sizeof(archivoProceso));

			archProc = (archivoProceso*)result;

			archivoGlobal* archGlob = list_get(ArchivosGlobales, archProc->FD);

			uint32_t archivoFueLeido = FS_ObtenerDatos(socketConFS, KERNEL, archGlob->pathArchivo, archProc->offsetArchivo, sizeArchivo);

			free(archProc);

			return archivoFueLeido;
		}
		else
		{
			//Finalizar ejecucion del proceso, liberar recursos y poner exitCode = -2
		}
	}
	else
	{
		//Finalizar ejecucion del proceso, liberar recursos y poner exitCode = -3
	}
}

uint32_t escribirArchivo(uint32_t FD, uint32_t PID, uint32_t sizeArchivo, uint32_t datosAGrabar, char* permisos)
{
	if(strstr(*permisos, "w") != NULL)
	{
		void* result = NULL;
		int i = 0;

		while(i < list_size(ArchivosProcesos) && result == NULL)
		{
			t_list* lista = list_get(ArchivosProcesos, i);

			result = (archivoProceso*) list_find(lista, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

			i++;
		}

		if(result != NULL)
		{
			archivoProceso* archProc = malloc(sizeof(archivoProceso));

			archProc = (archivoProceso*)result;

			archivoGlobal* archGlob = list_get(ArchivosGlobales, archProc->FD);

			uint32_t archivoFueEscrito = FS_GuardarDatos(socketConFS, KERNEL, archGlob->pathArchivo, archProc->offsetArchivo, sizeArchivo, datosAGrabar);

			free(archProc);

			return archivoFueEscrito;
		}
		else
		{
			//Finalizar ejecucion del proceso, liberar recursos y poner exitCode = -2
		}
	}
	else
	{
		//Finalizar ejecucion del proceso, liberar recursos y poner exitCode = -4
	}
}

uint32_t cerrarArchivo(uint32_t FD, uint32_t PID)
{
	void* result = NULL;
	int i = 0;

	while(i < list_size(ArchivosProcesos) && result == NULL)
	{
		t_list* lista = list_get(ArchivosProcesos, i);

		result = (archivoProceso*) list_find(lista, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

		i++;
	}

	if(result != NULL)
	{
		t_list* listaProcesoACerrar = list_get(ArchivosProcesos, i-1); //El indice que me habia quedado del while anterior

		list_remove_and_destroy_by_condition(listaProcesoACerrar, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

		archivoGlobal* archivoGlob = malloc(sizeof(archivoGlobal));

		archivoGlob = list_get(ArchivosGlobales, FD);

		archivoGlob->cantAperturas--;

		if(archivoGlob->cantAperturas == 0)
		{
			list_remove_and_destroy_by_condition(ArchivosGlobales, LAMBDA(bool _(void* item) { return ((archivoGlobal*) item)->pathArchivo == archivoGlob->pathArchivo; }));
		}
		else
		{
			list_replace(ArchivosGlobales, FD, archivoGlob);
		}
	}
	else
	{
		//Finalizar ejecucion del proceso, liberar recursos y poner exitCode = -20 (El programa intentó cerrar un archivo que no existe)
	}
}

void accion(Paquete* paquete, int socketConectado){
	/*pthread_t hiloSyscallWrite;
	pthread_create(&hiloSyscallWrite,NULL, (void*)syscallWrite, &socketConectado);*/
	switch(paquete->header.tipoMensaje) {

		case ESSTRING:

				if(strcmp(paquete->header.emisor,CONSOLA)==0)
				{
					double tamanioArchivo = paquete->header.tamPayload/TamanioPagina;
					double tamanioTotalPaginas = ceil(tamanioArchivo)+STACK_SIZE;
					BloqueControlProceso* pcb = malloc(sizeof(BloqueControlProceso));
					//TODO: Falta free, pero OJO, hay que ver la forma de ponerlo y que siga andando
					CrearNuevoProceso(pcb,&ultimoPID,Nuevos);

					//Manejo la multiprogramacion
					if(GRADO_MULTIPROG - list_size(Ejecutando) - list_size(Listos) > 0 && list_size(Nuevos) >= 1){
						//Pregunta a la memoria si me puede guardar estas paginas
						uint32_t paginasConfirmadas = IM_InicializarPrograma(socketConMemoria,KERNEL,pcb->PID,tamanioTotalPaginas);
						if(paginasConfirmadas>0 ) // N° negativo significa que la memoria no tiene espacio
						{
							PonerElProgramaComoListo(pcb,paquete,socketConectado,tamanioTotalPaginas);
							//Solicito a la memoria que me guarde el codigo del programa
							IM_GuardarDatos(socketConMemoria, KERNEL, pcb->PID, 0, 0, paquete->header.tamPayload, paquete->Payload); //TODO: sacar harcodeo
							EnviarDatos(socketConectado,KERNEL,&(pcb->PID),sizeof(uint32_t));
							//TODO: Ejecutar en alguna CPU

							//TODO: Realizar acciones en caso de solicitud de reserva de memoria dinamica
						}
						else
						{
							//Sacar programa de la lista de nuevos y meterlo en la lista de finalizado con su respectivo codigo de error
							KillProgramDeUnaLista(Nuevos,pcb,NOSEPUDIERONRESERVARRECURSOS, INDEX_NUEVOS, socketConectado);
							EnviarMensaje(socketConectado,"No se pudo guardar el programa porque no hay espacio suficiente",KERNEL);
						}
					}
					else
					{
						//El grado de multiprogramacion no te deja agregar otro proceso
						KillProgramDeUnaLista(Nuevos,pcb,NOSEPUDIERONRESERVARRECURSOS, INDEX_NUEVOS, socketConectado);
						EnviarMensaje(socketConectado,"No se pudo guardar el programa porque se alcanzo el grado de multiprogramacion",KERNEL);

					}

				}

				if(strcmp(paquete->header.emisor, CPU)==0)
				{
					//Reservado para operaciones de File System
				}

		break;
		case KILLPROGRAM:
			if(strcmp(paquete->header.emisor,CONSOLA)==0){
				pidAFinalizar = *(uint32_t*)paquete->Payload;
				bool finalizadoConExito = KillProgram(pidAFinalizar, DESCONECTADODESDECOMANDOCONSOLA, socketConectado);
				if(finalizadoConExito==true){
					printf("El programa %d fue finalizado\n", pidAFinalizar);
					EnviarMensaje(socketConectado,"KILLEADO",KERNEL);
				}
				else {
					printf("Error al finalizar programa\n", pidAFinalizar);
					EnviarMensaje(socketConectado,"Error al finalizar programa",KERNEL);
				}
			}
		break;
	}
//	pthread_join(hiloSyscallWrite, NULL);

}

void RecibirHandshake_KernelDeMemoria(int socketFD, char emisor[11]) {
	Paquete* paquete =  malloc(sizeof(Paquete));
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0 && paquete->header.tipoMensaje == ESHANDSHAKE) { //si no hubo error y es un handshake
		if (strcmp(paquete->header.emisor, emisor) == 0) {
				printf("\nConectado con el servidor!\n");
				if(strcmp(paquete->header.emisor, MEMORIA) == 0){
					paquete->Payload = malloc(paquete->header.tamPayload);
					resul = RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
					TamanioPagina = *((uint32_t*)paquete->Payload);
					free(paquete->Payload);
				}
		} else
			perror("Error, no se recibio un handshake del servidor esperado\n");
	} else
	perror("Error de Conexion, no se recibio un handshake\n");


	free(paquete);
}
void userInterfaceHandler(uint32_t* socketFD) {
	int end = 1;
	while (end) {
		char orden[100];
		int pidConsulta=0;
		int nuevoGradoMP=0;

		char lista[100];
		printf("\n\nIngrese una orden: \n");
		scanf("%s", orden);
		char* command = getWord(orden, 0);
		if(strcmp(command,"exit")==0)
				exit(1);
		else if (strcmp(command, "disconnect") == 0) {
				end = 0;
		}
		//Para cambiar el grado de multiprogramacion
		else if(strcmp(command,"cambiar_gradomp")==0){
			scanf("%d", &nuevoGradoMP);
			GRADO_MULTIPROG = nuevoGradoMP;
			printf("El nuevo grado de multiprogramacion es: %d\n",GRADO_MULTIPROG);
		}
		else if (strcmp(command, "mostrar_procesos") == 0) {
			scanf("%s", lista);
			if(strcmp(lista,NUEVOS)==0)
				MostrarProcesosDeUnaLista(Nuevos,NUEVOS);
			else if(strcmp(lista,LISTOS)==0)
				MostrarProcesosDeUnaLista(Listos,LISTOS);
			else if(strcmp(lista,EJECUTANDO)==0)
				MostrarProcesosDeUnaLista(Ejecutando,EJECUTANDO);
			else if(strcmp(lista,BLOQUEADOS)==0)
				MostrarProcesosDeUnaLista(Bloqueados,BLOQUEADOS);
			else if(strcmp(lista,FINALIZADOS)==0)
				MostrarProcesosDeUnaLista(Finalizados,FINALIZADOS);

			else if(strcmp(lista,"TODAS")==0)
				MostrarTodosLosProcesos();
			else
				printf("No se reconoce la lista ingresada");
			}

		else if (strcmp(command, "consultar_programa") == 0) {
			scanf("%d", &pidConsulta);
			ConsultarEstado(pidConsulta);
		}
		else if (strcmp(command, "kill_program") == 0){
			scanf("%d", &pidConsulta);
			KillProgram(pidConsulta, DESCONECTADODESDECOMANDOCONSOLA, *socketFD);
		}
		else {
			printf("No se conoce el mensaje %s\n", orden);
		}
	}
}



int main(void)
{
	CrearListas();
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	while((socketConMemoria = ConectarAServidor(PUERTO_MEMORIA,IP_MEMORIA,MEMORIA,KERNEL, RecibirHandshake_KernelDeMemoria))<0);
	while((socketConFS = ConectarAServidor(PUERTO_FS,IP_FS,FS,KERNEL, RecibirHandshake))<0);

	pthread_t hiloConsola;
	//pthread_t hiloSyscallWrite;
	//pthread_create(&hiloSyscallWrite, NULL, (void*)syscallWrite, 2); //socket 2 hardcodeado
	//pthread_join(hiloSyscallWrite, NULL);
	pthread_create(&hiloConsola, NULL, (void*)userInterfaceHandler, &socketConMemoria);
	Servidor(IP_PROG, PUERTO_PROG, KERNEL, accion, RecibirPaqueteServidor);
	pthread_join(hiloConsola, NULL);
	LimpiarListas();

	return 0;
}
