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

char* obtenerTiempoString(time_t t) {
	struct tm *tm = localtime(&t);
	char s[64];
	strftime(s, sizeof(s), "%c", tm);
	return s;
}

int GetTamanioArchivo(FILE * f) {
	fseek(f, 0L, SEEK_END);
	int size = ftell(f);
	return size;
}
