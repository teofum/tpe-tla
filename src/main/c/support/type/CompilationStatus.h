#ifndef COMPILATION_STATUS_HEADER
#define COMPILATION_STATUS_HEADER

/**
 * The general status of a compilation. Add more status codes if needed, but
 * be aware that values under 99 are reserved for internal infrastructure and
 * functionality.
 */
typedef enum {
	/**
	 * The input program was rejected.
	 */
	FAILED = 1,

	/**
	 * The compilation is going on as expected.
	 */
	IN_PROGRESS = 4,

	/**
	 * The compiler is out of memory and cannot proceed as expected.
	 */
	OUT_OF_MEMORY = 2,

	/**
	 * The input program was accepted.
	 */
	SUCCEEDED = 0,

	/**
	 * An unknown or unexpected error occurred.
	 */
	UNKNOWN_ERROR = 3
} CompilationStatus;

#endif
