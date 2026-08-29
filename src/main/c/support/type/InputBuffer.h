#ifndef INPUT_BUFFER_HEADER
#define INPUT_BUFFER_HEADER

#include "../logging/Logger.h"
#include "LexicalAnalyzer.h"
#include <stdio.h>

/**
 * A lexical-analyzer input buffer.
 */
typedef struct {
	FILE * file;
	LexicalAnalyzer * lexicalAnalyzer;
	unsigned int bufferSizeInBytes;
	void * buffer;
} InputBuffer;

#endif
