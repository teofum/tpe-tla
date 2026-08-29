#ifndef LEXICAL_ANALYZER_HEADER
#define LEXICAL_ANALYZER_HEADER

#include "../logging/Logger.h"

/**
 * A lexical-analyzer and its internal state.
 */
typedef struct {
	Logger * logger;
	void * location;
	void * parser;
	void * scanner;
} LexicalAnalyzer;

#endif
