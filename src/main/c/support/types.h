#ifndef TYPES_HEADER
#define TYPES_HEADER

#include <stddef.h>
#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;

typedef float f32;
typedef double f64;

// -----------------------------------------------------------------------------

typedef u32 FlexContext;

typedef enum {
  SUCCEEDED = 0,
  FAILED = 1,
  OUT_OF_MEMORY = 2,
  UNKNOWN_ERROR = 3,
  IN_PROGRESS = 4,
} CompilationStatus;

typedef union SemanticValue SemanticValue;

#endif
