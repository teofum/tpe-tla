#ifndef SEMANTIC_VALUE_HEADER
#define SEMANTIC_VALUE_HEADER

#include "TokenLabel.h"
#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../frontend/syntactic-analysis/BisonParser.h"

/**
 * The type of a Bison semantic value, that is, the meaning attached to a
 * token.
 */
typedef union SemanticValue SemanticValue;

#endif
