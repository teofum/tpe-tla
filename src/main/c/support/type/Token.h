#ifndef TOKEN_HEADER
#define TOKEN_HEADER

#include "FlexContext.h"
#include "SemanticValue.h"
#include "TokenLabel.h"

/**
 * The unit of information to transfer between a lexical-analyzer and a
 * syntactic-analyzer.
 */
typedef struct {
	char * lexeme;
	FlexContext context;
	SemanticValue * semanticValue;
	TokenLabel label;
	unsigned int length;
	unsigned int line;
} Token;

#endif
