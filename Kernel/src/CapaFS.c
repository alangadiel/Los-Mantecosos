#include "CapaFS.h"

//Tipos de ExitCode
#define ACCEDERAARCHVIOQUENOEXISTE -2
#define LEERARCHIVOSINPERMISOS -3
#define ESCRIBIRARCHIVOSINPERMISO -4
#define CREARARCHIVOSINPERMISOS -10

int ultimoGlobalFD = 0;

typedef struct {
	t_list* listaArchivo;
	uint32_t PID;
} ListaArchivosProceso;


char* armarPath(char* path)
{
	int desde = 0;
	int hasta = 0;
	char* subsrt;

	while(desde < string_length(path) && path[desde] != '/')
	{
		desde++;
	}

	while(hasta < string_length(path) && path[hasta] != '\n' && path[hasta] != '\t' && path[hasta] != '\b' && path[hasta] != '\r' && path[hasta] != '\a' && path[hasta] != '\f' && path[hasta] != '\'' && path[hasta] != '\"')
	{
		hasta++;
	}

	subsrt = string_substring(path, desde, hasta - desde);

	char* ultimosCuatro = string_substring(subsrt, string_length(subsrt) - 4, 5);

	if(ultimosCuatro[4] != '\0')
	{
		string_append(&ultimosCuatro, "");
		printf("no hay barra cero");
		//ultimosCuatro[4] = '\0';
	}

	if(strcmp(ultimosCuatro, ".bin") != 0)
	{
		string_append(&subsrt, ".bin");
	}

	free(ultimosCuatro);

	//free(*path);

	return subsrt;
	//*path = string_duplicate(subsrt);

	/*if(strcmp(string_substring_from(*path, string_length(*path) - 8), ".bin.bin") == 0)
	{
		*path = string_duplicate(string_substring_until(*path, string_length(*path) - 4));
	}*/

	//free(subsrt);
}


uint32_t cargarEnTablasArchivos(char* path, uint32_t PID, BanderasPermisos permisos)
{
	void* result = NULL;

	char* pathLimpio = armarPath(path);

	free(path);

	result = (archivoGlobal*) list_find(ArchivosGlobales, LAMBDA(bool _(void* item) { return strcmp(((archivoGlobal*) item)->pathArchivo, pathLimpio) == 0; }));

	archivoGlobal* archivoGlob;
	archivoProceso* archivoProc;

	if(result == NULL) //No hay un archivo global con ese path
	{
		archivoGlob = malloc(sizeof(uint32_t) * 2 + string_length(pathLimpio) + 1); //El free se hace en limpiar listas

		archivoGlob->pathArchivo = pathLimpio;
		archivoGlob->cantAperturas = 1;
		archivoGlob->archivoGlobalFD = ultimoGlobalFD;

		ultimoGlobalFD++;

		list_add(ArchivosGlobales, archivoGlob);
	}
	else
	{
		archivoGlob = (archivoGlobal*)result;

		archivoGlob->cantAperturas++;
	}

	ListaArchivosProceso* lista = NULL;

	lista = (ListaArchivosProceso*)list_find(ArchivosProcesos, LAMBDA(bool _(void* item) { return ((ListaArchivosProceso*) item)->PID == PID; }));

	archivoGlobal* archGlob = (archivoGlobal*)list_find(ArchivosGlobales, LAMBDA(bool _(void* item) { return strcmp(((archivoGlobal*) item)->pathArchivo, pathLimpio) == 0; }));

	if(lista == NULL) //No hay ninguna lista de archivos para ese proceso porque no habia abierto ningun archivo todavia
	{
		archivoProc = malloc(sizeof(archivoProceso)); //El free se hace en limpiar listas
		archivoProc->PID = PID;
		archivoProc->FD = 3;
		archivoProc->flags = permisos;
		archivoProc->offsetArchivo = 0;
		archivoProc->globalFD = archGlob->archivoGlobalFD;

		ListaArchivosProceso* listaArchivoProceso;
		listaArchivoProceso = malloc(sizeof(listaArchivoProceso));

		listaArchivoProceso->listaArchivo = list_create();
		listaArchivoProceso->PID = PID;


		list_add(listaArchivoProceso->listaArchivo, archivoProc);

		list_add(ArchivosProcesos, listaArchivoProceso);
	}
	else //Hay una lista para ese proceso, entonces solo agrego un archivo
	{
		archivoProceso* archProc = list_get(lista->listaArchivo, list_size(lista->listaArchivo)-1);

		archivoProc = malloc(sizeof(archivoProceso)); //El free se hace en limpiar listas

		archivoProc->PID = PID;
		archivoProc->FD = archProc->FD+1;
		archivoProc->flags = permisos;
		archivoProc->offsetArchivo = 0;
		archivoProc->globalFD = archGlob->archivoGlobalFD;

		ListaArchivosProceso* listaArchivoProceso = lista;

		list_add(listaArchivoProceso->listaArchivo, archivoProc);

		//list_replace(ArchivosProcesos, i-1, listaArchivoProceso);
	}

	return archivoProc->FD;
}


void finalizarProgramaCapaFS(int PID)
{
	int indice = 0;
	ListaArchivosProceso* listaProcesoAFinalizar = NULL;

	while(indice < list_size(ArchivosProcesos))
	{
		listaProcesoAFinalizar = list_get(ArchivosProcesos, indice);

		if(listaProcesoAFinalizar->PID == PID)
		{
			break;
		}

		indice++;
	}

	if(listaProcesoAFinalizar != NULL)
	{
		int j;

		for(j = 0; j < list_size(listaProcesoAFinalizar->listaArchivo); j++)
		{
			archivoProceso* archivoProc = (archivoProceso*)list_get(listaProcesoAFinalizar->listaArchivo, j);

			archivoGlobal* archivoGlob = (archivoGlobal*)list_find(ArchivosGlobales, LAMBDA(bool _(void* item) { return ((archivoGlobal*) item)->archivoGlobalFD == archivoProc->globalFD; }));

			archivoGlob->cantAperturas--;

			if(archivoGlob->cantAperturas == 0)
			{
				list_remove_and_destroy_by_condition(ArchivosGlobales,
						LAMBDA(bool _(void* item) { return strcmp(((archivoGlobal*) item)->pathArchivo, archivoGlob->pathArchivo) == 0; }),
						LAMBDA(void* _(void* item) { free(((archivoGlobal*) item)->pathArchivo); free(item); }));
			}
		}

		list_remove_and_destroy_element(ArchivosProcesos, indice, free);
	}
}


uint32_t abrirArchivo(char* path, uint32_t PID, BanderasPermisos permisos, int socketConectado, int32_t* tipoError)
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
	archivoProceso* archivoProc = NULL;
	ListaArchivosProceso* listaArchivos = NULL;

	listaArchivos = (ListaArchivosProceso*)list_find(ArchivosProcesos, LAMBDA(bool _(void* item) { return ((ListaArchivosProceso*) item)->PID == PID; }));

	archivoProc = (archivoProceso*)list_find(listaArchivos->listaArchivo, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

	if(archivoProc != NULL)
	{
		if(archivoProc->flags.lectura == true)
		{
			archivoGlobal* archGlob = (archivoGlobal*)list_find(ArchivosGlobales, LAMBDA(bool _(void* item) { return ((archivoGlobal*) item)->archivoGlobalFD == archivoProc->globalFD; }));

			void* dato = FS_ObtenerDatos(socketConFS, KERNEL, archGlob->pathArchivo, archivoProc->offsetArchivo, sizeArchivo);

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
	archivoProceso* archivoProc = NULL;
	ListaArchivosProceso* listaArchivos = NULL;

	listaArchivos = (ListaArchivosProceso*)list_find(ArchivosProcesos, LAMBDA(bool _(void* item) { return ((ListaArchivosProceso*) item)->PID == PID; }));

	archivoProc = (archivoProceso*)list_find(listaArchivos->listaArchivo, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

	if(archivoProc != NULL)
	{
		if(archivoProc->flags.escritura == true)
		{
			archivoGlobal* archGlob = (archivoGlobal*)list_find(ArchivosGlobales, LAMBDA(bool _(void* item) { return ((archivoGlobal*) item)->archivoGlobalFD == archivoProc->globalFD; }));

			uint32_t archivoFueEscrito = FS_GuardarDatos(socketConFS, KERNEL, archGlob->pathArchivo, archivoProc->offsetArchivo, sizeArchivo, datosAGrabar);

			//archivoProc->offsetArchivo += sizeArchivo;

			return archivoFueEscrito;
		}
		else
		{
			FinalizarPrograma(PID, ESCRIBIRARCHIVOSINPERMISO);

			return 0;
		}
	}
	else
	{
		FinalizarPrograma(PID, ACCEDERAARCHVIOQUENOEXISTE);

		return 0;
	}
}


void cerrarArchivo(uint32_t FD, uint32_t PID)
{
	archivoProceso* archivoProcesoACerrar = NULL;
	ListaArchivosProceso* listaProcesoACerrar = NULL;

	listaProcesoACerrar = (ListaArchivosProceso*)list_find(ArchivosProcesos, LAMBDA(bool _(void* item) { return ((ListaArchivosProceso*) item)->PID == PID; }));

	archivoProcesoACerrar = (archivoProceso*)list_find(listaProcesoACerrar->listaArchivo, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

	if(archivoProcesoACerrar != NULL)
	{
		archivoGlobal* archivoGlob = (archivoGlobal*)list_find(ArchivosGlobales, LAMBDA(bool _(void* item) { return ((archivoGlobal*) item)->archivoGlobalFD == archivoProcesoACerrar->globalFD; }));

		list_remove_and_destroy_by_condition(listaProcesoACerrar->listaArchivo, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }), free);

		archivoGlob->cantAperturas--;

		if(archivoGlob->cantAperturas == 0)
		{
			printf("El archivo con path: %s, fue eliminado de la tabla de archivos globales ya que ningun proceso lo tiene abierto\n", archivoGlob->pathArchivo);

			list_remove_and_destroy_by_condition(ArchivosGlobales, LAMBDA(bool _(void* item) { return strcmp(((archivoGlobal*) item)->pathArchivo, archivoGlob->pathArchivo) == 0; }), free);
		}
		else
		{
			printf("El archivo con path: %s, fue cerrado. %d proceso/s lo tiene/n abierto\n", archivoGlob->pathArchivo, archivoGlob->cantAperturas);
		}
	}
	else
	{
		FinalizarPrograma(PID, ACCEDERAARCHVIOQUENOEXISTE);
	}
}


void borrarArchivo(uint32_t FD, uint32_t PID, int socketConectado)
{
	ListaArchivosProceso* listaProcesoABorrar = NULL;

	listaProcesoABorrar = (ListaArchivosProceso*)list_find(ArchivosProcesos, LAMBDA(bool _(void* item) { return ((ListaArchivosProceso*) item)->PID == PID; }));

	if(listaProcesoABorrar != NULL)
	{
		archivoProceso* archivoProcesoABorrar = list_find(listaProcesoABorrar->listaArchivo, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

		archivoGlobal* archivoGlob = (archivoGlobal*)list_find(ArchivosGlobales, LAMBDA(bool _(void* item) { return ((archivoGlobal*) item)->archivoGlobalFD == archivoProcesoABorrar->globalFD; }));

		list_remove_and_destroy_by_condition(listaProcesoABorrar->listaArchivo, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }), free);

		uint32_t fueBorrado = FS_BorrarArchivo(socketConectado, KERNEL, archivoGlob->pathArchivo);

		if(fueBorrado == 1)
		{
			printf("El archivo con path %s, fue borrado\n", archivoGlob->pathArchivo);

			list_remove_and_destroy_by_condition(ArchivosGlobales, LAMBDA(bool _(void* item) { return strcmp(((archivoGlobal*) item)->pathArchivo, archivoGlob->pathArchivo) == 0; }),free);
		}
		else
		{
			printf("No se pudo borrar el archivo con path %s\n", archivoGlob->pathArchivo);
		}
	}
	else
	{
		FinalizarPrograma(PID, ACCEDERAARCHVIOQUENOEXISTE);
	}
}


void moverCursor(uint32_t FD, uint32_t PID, uint32_t posicion)
{
	ListaArchivosProceso* listaProceso = NULL;
	archivoProceso* archivoProc = NULL;

	listaProceso = (ListaArchivosProceso*)list_find(ArchivosProcesos, LAMBDA(bool _(void* item) { return ((ListaArchivosProceso*) item)->PID == PID; }));

	archivoProc = (archivoProceso*)list_find(listaProceso->listaArchivo, LAMBDA(bool _(void* item) { return ((archivoProceso*) item)->PID == PID && ((archivoProceso*) item)->FD == FD; }));

	if(archivoProc != NULL)
	{
		archivoProc->offsetArchivo = posicion;

		printf("El archivo fue movido a la posicion %d\n", posicion);
	}
	else
	{
		FinalizarPrograma(PID, ACCEDERAARCHVIOQUENOEXISTE);
	}
}


t_list* obtenerTablaArchivosDeUnProceso(uint32_t PID)
{
	ListaArchivosProceso* listaArchivosProceso = NULL;

	listaArchivosProceso = (ListaArchivosProceso*)list_find(ArchivosProcesos, LAMBDA(bool _(void* item) { return ((ListaArchivosProceso*) item)->PID == PID; }));

	return listaArchivosProceso->listaArchivo;
}


t_list* obtenerTablaArchivosGlobales()
{
	return ArchivosGlobales;
}
