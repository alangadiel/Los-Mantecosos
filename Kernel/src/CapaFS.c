#include "CapaFS.h"

//Tipos de ExitCode
#define ACCEDERAARCHVIOQUENOEXISTE -2
#define LEERARCHIVOSINPERMISOS -3
#define ESCRIBIRARCHIVOSINPERMISO -4
#define CREARARCHIVOSINPERMISOS -10



void armarPath(char** path)
{
	int finWhile = 0;
	int hasta = 0;
	char* subsrt;
	printf("estoy aca");
	string_trim(path);

	printf("el path con trim es %s", *path);

	while(hasta < string_length(*path) && (*path)[hasta] != '\n' && (*path)[hasta] != '\t' && (*path)[hasta] != '\b')
	{
		hasta++;
	}

	subsrt = string_substring_until(*path, hasta);
	printf("substring es %s", subsrt);
	string_append(&subsrt, ".bin");
	printf(" y con .bin %s", subsrt);
	free(*path);

	*path = string_duplicate(subsrt);

	free(subsrt);
}


uint32_t cargarEnTablasArchivos(char* path, uint32_t PID, permisosArchivo permisos)
{
	void* result = NULL;

	armarPath(&path);

	result = (archivoGlobal*) list_find(ArchivosGlobales, LAMBDA(bool _(void* item) { return strcmp(((archivoGlobal*) item)->pathArchivo, path) == 0; }));

	archivoGlobal* archivoGlob = malloc(sizeof(uint32_t) + string_length(path) + 1); //El free se hace en limpiar listas
	archivoProceso* archivoProc = malloc(sizeof(archivoProceso)); //El free se hace en limpiar listas

	if(result == NULL) //No hay un archivo global con ese path
	{
		archivoGlob->pathArchivo = string_new();

		string_append(&archivoGlob->pathArchivo, path);
		archivoGlob->cantAperturas = 1;

		list_add(ArchivosGlobales, archivoGlob);

		archivoGlobal* a = (archivoGlobal*) list_find(ArchivosGlobales, LAMBDA(bool _(void* item) { return strcmp(((archivoGlobal*) item)->pathArchivo, path) == 0; }));
		printf("el path global es %s", a->pathArchivo);
	}
	else
	{
		archivoGlob = (archivoGlobal*)result;

		archivoGlob->cantAperturas++;

		int indice = 0;
		int i;

		for(i = 0; i < list_size(ArchivosGlobales); i++)
		{
			archivoGlobal* arch = (archivoGlobal*)list_get(ArchivosGlobales, i);

			if(strcmp(arch->pathArchivo, path) == 0)
			{
				indice = i;
			}
		}

		list_replace(ArchivosGlobales, indice, archivoGlob);
	}

	int i = 0;

	result = NULL;
	t_list* lista;

	while(i < list_size(ArchivosProcesos) && result == NULL)
	{
		lista = (t_list*)list_get(ArchivosProcesos, i);

		result = (archivoProceso*)list_find(lista, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID; }));

		i++;
	}

	int indice;
	int j;

	for(j = 0; j < list_size(ArchivosGlobales); j++)
	{
		archivoGlobal* arch = list_get(ArchivosGlobales, j);

		if(strcmp(arch->pathArchivo, path) == 0)
		{
			indice = j;
		}
	}

	if(result == NULL) //No hay ninguna lista de archivos para ese proceso porque no habia abierto ningun archivo todavia
	{
		archivoProc->PID = PID;
		archivoProc->FD = 3;
		archivoProc->flags = permisos;
		archivoProc->offsetArchivo = 0;
		archivoProc->globalFD = indice;

		t_list* listaArchivoProceso = list_create();

		list_add(listaArchivoProceso, archivoProc);

		list_add(ArchivosProcesos, listaArchivoProceso);
	}
	else //Hay una lista para ese proceso, entonces solo agrego un archivo
	{
		archivoProceso* arch = list_get(lista, sizeof(lista)-1);

		archivoProc->PID = PID;
		archivoProc->FD = arch->FD+1;
		archivoProc->flags = permisos;
		archivoProc->offsetArchivo = 0;
		archivoProc->globalFD = indice;

		t_list* listaArchivoProceso = (t_list*)list_get(ArchivosProcesos, i-1);

		list_add(listaArchivoProceso, archivoProc);

		list_replace(ArchivosProcesos, i-1, listaArchivoProceso);
	}

	return archivoProc->FD;
}


void finalizarProgramaCapaFS(int PID)
{
	void* result = NULL;
	int i = 0;
	t_list* listaProcesoAFinalizar;

	while(i < list_size(ArchivosProcesos) && result == NULL)
	{
		listaProcesoAFinalizar = (t_list*)list_get(ArchivosProcesos, i);

		result = (archivoProceso*)list_find(listaProcesoAFinalizar, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID; }));

		i++;
	}

	if(result != NULL)
	{
		int j;

		for(j = 0; j < list_size(listaProcesoAFinalizar); j++)
		{
			archivoProceso* archivoProc = (archivoProceso*)list_get(listaProcesoAFinalizar, j);

			archivoGlobal* archivoGlob = (archivoGlobal*)list_get(ArchivosGlobales, archivoProc->globalFD);

			archivoGlob->cantAperturas--;

			if(archivoGlob->cantAperturas == 0)
			{
				list_remove_and_destroy_by_condition(ArchivosGlobales, LAMBDA(bool _(void* item) { return ((archivoGlobal*) item)->pathArchivo == archivoGlob->pathArchivo; }), free);
			}
			else
			{
				list_replace(ArchivosGlobales, archivoProc->globalFD, archivoGlob);
			}
		}

		list_remove_and_destroy_element(ArchivosProcesos, i-1, free);
	}
}


uint32_t abrirArchivo(char* path, uint32_t PID, permisosArchivo permisos, int socketConectado, int32_t* tipoError)
{
	uint32_t archivoEstaCreado = FS_ValidarPrograma(socketConFS, KERNEL, path);

	uint32_t FD;

	if(archivoEstaCreado == 1)
	{
		FD = cargarEnTablasArchivos(path, PID, permisos);

		EnviarDatos(socketConectado, KERNEL, &FD, sizeof(uint32_t));
	}
	else
	{
		if(permisos.creacion == true)
		{
			archivoEstaCreado = FS_CrearPrograma(socketConFS, KERNEL, path);
			FD = cargarEnTablasArchivos(path, PID, permisos);
			printf("Se creo el archivo? %d", archivoEstaCreado);
			EnviarDatos(socketConectado, KERNEL, &FD, sizeof(uint32_t));
		}
		else
		{
			*tipoError = ACCEDERAARCHIVOINEXISTENTE;
			EnviarDatosTipo(socketConectado, KERNEL, tipoError, sizeof(int32_t), ESERROR);
		}
	}

	return archivoEstaCreado;
}


void* leerArchivo(uint32_t FD, uint32_t PID, uint32_t sizeArchivo, uint32_t punteroArchivo)
{
	void* result = NULL;
	int i = 0;
	t_list* listaProceso;

	while(i < list_size(ArchivosProcesos) && result == NULL)
	{
		listaProceso = (t_list*)list_get(ArchivosProcesos, i);

		result = (archivoProceso*)list_find(listaProceso, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

		i++;
	}

	if(result != NULL)
	{
		archivoProceso* archivoProc = (archivoProceso*)result;

		printf("\nel FD de archivoProc es %d", archivoProc->FD);
		printf("\nel PID de archivoProc es %d", archivoProc->PID);
		printf("\nel globalFD de archivoProc es %d", archivoProc->globalFD);

		if(archivoProc->flags == NULL)
		{
			printf("\nel flag lectura de archivoProc es NULL");
		}
		else
		{
			printf("\n el flag de archivoProc no es NULL");
		}



		if(archivoProc->flags.lectura == true)
		{
			archivoGlobal* archGlob = (archivoGlobal*)list_get(ArchivosGlobales, archivoProc->globalFD);
			printf("el path del global es %s", archGlob->pathArchivo);

			void* dato = FS_ObtenerDatos(socketConFS, KERNEL, archGlob->pathArchivo, 0, sizeArchivo);

			return dato;
		}
		else
		{
			FinalizarPrograma(PID, LEERARCHIVOSINPERMISOS);
			return NULL;
		}
	}
	else
	{
		FinalizarPrograma(PID, ACCEDERAARCHVIOQUENOEXISTE);
		return NULL;
	}
}


uint32_t escribirArchivo(uint32_t FD, uint32_t PID, uint32_t sizeArchivo, void* datosAGrabar)
{
	void* result = NULL;
	int i = 0;

	while(i < list_size(ArchivosProcesos) && result == NULL)
	{
		t_list* listaProceso = (t_list*)list_get(ArchivosProcesos, i);

		result = (archivoProceso*)list_find(listaProceso, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

		i++;
	}

	if(result != NULL)
	{
		archivoProceso* archivoProc = malloc(sizeof(archivoProceso));

		archivoProc = (archivoProceso*)result;

		if(archivoProc->flags.escritura == true)
		{
			archivoGlobal* archGlob = (archivoGlobal*)list_get(ArchivosGlobales, archivoProc->FD - 3); //El -3 es porque los FD empiezan desde 3

			uint32_t archivoFueEscrito = FS_GuardarDatos(socketConFS, KERNEL, archGlob->pathArchivo, archivoProc->offsetArchivo, sizeArchivo, datosAGrabar);

			archivoProc->offsetArchivo += sizeArchivo;

			list_replace(ArchivosProcesos, FD - 3, archivoProc);

			return archivoFueEscrito;
		}
		else
		{
			free(archivoProc);

			FinalizarPrograma(PID, ESCRIBIRARCHIVOSINPERMISO);
		}
	}
	else
	{
		FinalizarPrograma(PID, ACCEDERAARCHVIOQUENOEXISTE);
	}
}


uint32_t cerrarArchivo(uint32_t FD, uint32_t PID)
{
	void* result = NULL;
	int i = 0;
	t_list* listaProcesoACerrar;

	while(i < list_size(ArchivosProcesos) && result == NULL)
	{
		listaProcesoACerrar = (t_list*)list_get(ArchivosProcesos, i);

		result = (archivoProceso*)list_find(listaProcesoACerrar, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

		i++;
	}

	if(result != NULL)
	{
		archivoProceso* procesoACerrar = (archivoProceso*)result;

		archivoGlobal* archivoGlob = (archivoGlobal*)list_get(ArchivosGlobales, procesoACerrar->globalFD);

		list_remove_and_destroy_by_condition(listaProcesoACerrar, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }), free);

		archivoGlob->cantAperturas--;

		if(archivoGlob->cantAperturas == 0)
		{
			list_remove_and_destroy_by_condition(ArchivosGlobales, LAMBDA(bool _(void* item) { return ((archivoGlobal*) item)->pathArchivo == archivoGlob->pathArchivo; }), free);
		}
	}
	else
	{
		FinalizarPrograma(PID, ACCEDERAARCHVIOQUENOEXISTE);
		//Finalizar ejecucion del proceso, liberar recursos y poner exitCode = -2
	}
}


uint32_t borrarArchivo(uint32_t FD, uint32_t PID)
{
	void* result = NULL;
	int i = 0;
	t_list* listaProcesoABorrar;

	while(i < list_size(ArchivosProcesos) && result == NULL)
	{
		listaProcesoABorrar = list_get(ArchivosProcesos, i);

		result = (archivoProceso*) list_find(listaProcesoABorrar, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

		i++;
	}

	if(result != NULL)
	{
		archivoProceso* procesoABorrar = (archivoProceso*)result;

		archivoGlobal* archivoGlob = list_get(ArchivosGlobales, procesoABorrar->globalFD);

		list_remove_and_destroy_by_condition(listaProcesoABorrar, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }), free);

		archivoGlob->cantAperturas--;

		if(archivoGlob->cantAperturas == 0)
		{
			uint32_t fueBorrado = FS_BorrarArchivo(FD, KERNEL, archivoGlob->pathArchivo);

			if(fueBorrado == 1)
			{
				list_remove_and_destroy_by_condition(ArchivosGlobales, LAMBDA(bool _(void* item) { return ((archivoGlobal*) item)->pathArchivo == archivoGlob->pathArchivo; }),free);
			}
			else
			{
				printf("No se pudo borrar el archivo global con path %s", archivoGlob->pathArchivo);
			}
		}
	}
	else
	{
		FinalizarPrograma(PID, ACCEDERAARCHVIOQUENOEXISTE);
		//Finalizar ejecucion del proceso, liberar recursos y poner exitCode = -2
	}
}


uint32_t moverCursor(uint32_t FD, uint32_t PID, uint32_t posicion)
{
	void* result = NULL;
	int i = 0;
	t_list* listaProceso;

	while(i < list_size(ArchivosProcesos) && result == NULL)
	{
		listaProceso = list_get(ArchivosProcesos, i);

		result = (archivoProceso*) list_find(listaProceso, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

		i++;
	}

	if(result != NULL)
	{
		archivoProceso* archivoProc = malloc(sizeof(archivoProceso));

		archivoProc = (archivoProceso*)result;

		archivoProc->offsetArchivo = posicion;

		list_replace(listaProceso, FD-3, archivoProc); //El -3 es porque los FD empiezan desde el 3

		free(archivoProc);
	}
	else
	{
		FinalizarPrograma(PID, ACCEDERAARCHVIOQUENOEXISTE);
		//Finalizar ejecucion del proceso, liberar recursos y poner exitCode = -2
	}
}


t_list* obtenerTablaArchivosDeUnProceso(uint32_t PID)
{
	int i = 0;
	void* result = NULL;
	t_list* tablaArchivosProceso = NULL;

	while(i < list_size(ArchivosProcesos) && result == NULL)
	{
		tablaArchivosProceso = list_get(ArchivosProcesos, i);

		result = (archivoProceso*) list_find(tablaArchivosProceso, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID; }));

		i++;
	}

	return tablaArchivosProceso;
}


t_list* obtenerTablaArchivosGlobales()
{
	return ArchivosGlobales;
}
