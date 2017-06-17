#include "CapaFS.h"
int ultimoFD = 3;

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

		int index = 0;
		int i;
		for(i= 0; i < list_size(ArchivosGlobales); i++)
		{
			archivoGlobal* arch = (archivoGlobal*)list_get(ArchivosGlobales, i);

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
		t_list* lista = (t_list*)list_get(ArchivosProcesos, i);

		result = (archivoProceso*) list_find(lista, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID; }));

		i++;
	}

	archivoProceso* archivoProc = malloc(sizeof(archivoProceso)); //El free se hace en limpiar listas

	int index;
	int j;
	for(j= 0; j < list_size(ArchivosGlobales); j++)
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

		t_list* listaArchivoProceso = (t_list*)list_get(ArchivosProcesos, i-1);

		list_add(listaArchivoProceso, archivoProc);

		list_replace(ArchivosProcesos, i-1, listaArchivoProceso);
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
		if(strstr(permisos, "c") != NULL)
		{
			FS_CrearPrograma(socketConFS, KERNEL, path);
			cargarEnTablasArchivos(path, PID, permisos);
		}
		else
		{
			//TODO: Avisarle a la CPU que termino
			//FinalizarPrograma(Ejecutando, PID, -20, INDEX_EJECUTANDO, socketConMemoria);
			//Finalizar ejecucion del proceso, liberar recursos y poner exitCode = -20 (El programa intentó crear un archivo sin permisos)
		}
	}

	return archivoEstaCreado;
}

uint32_t leerArchivo(uint32_t FD, uint32_t PID, uint32_t sizeArchivo, char* permisos)
{
	if(strstr(permisos, "r") != NULL)
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

			archProc->offsetArchivo += sizeArchivo;

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

		list_remove_and_destroy_by_condition(listaProcesoACerrar, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }),free);

		archivoGlobal* archivoGlob = malloc(sizeof(archivoGlobal));

		archivoGlob = list_get(ArchivosGlobales, FD);

		archivoGlob->cantAperturas--;

		if(archivoGlob->cantAperturas == 0)
		{
			list_remove_and_destroy_by_condition(ArchivosGlobales, LAMBDA(bool _(void* item) { return ((archivoGlobal*) item)->pathArchivo == archivoGlob->pathArchivo; }),free);
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
