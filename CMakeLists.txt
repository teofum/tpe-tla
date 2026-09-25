# Minimum required version fo CMake. Can be modified at your own risk.
# @see https://cmake.org/download/
cmake_minimum_required(VERSION 3.31.6)

# Name of the project and language to use (C, in this case).
project(Flex-Bison-Compiler C)

set(CMAKE_C_STANDARD 23)

# Selects the best strategy according to the available compiler in the system.
# @see https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  message(NOTICE "The C compiler is GCC.")

  # Compiles the parser with Bison, generating counter-examples in case S/R or
  # R/R conflicts exists.
  add_custom_command(
    OUTPUT
      ../src/frontend/bison/bison_parser.c
      ../src/frontend/bison/bison_parser.h
    COMMAND ../.script/bison.sh
  )

  # Options for GCC.
  add_compile_options(-fsanitize=address)
  add_compile_options(-O3)
  add_compile_options(-static-libgcc)
  add_link_options(-fsanitize=address)

  # Compiles the scanner with Flex.
  add_custom_command(
    OUTPUT
      ../src/frontend/flex/flex_scanner.c
      ../src/frontend/flex/flex_scanner.h
    COMMAND ../.script/flex.sh
    DEPENDS
      ../src/frontend/bison/bison_parser.c
      ../src/frontend/bison/bison_parser.h
  )

  add_executable(Flex-Bison-Compiler
    src/main.c
    src/ast/ast.c
    src/ast/dot.c
    src/error/error.c
    src/frontend/bison/bison_actions.c
    src/frontend/bison/bison_parser.c
    src/frontend/flex/flex_actions.c
    src/frontend/flex/flex_scanner.c
    src/frontend/frontend.c
    src/support/dyn_array.c
    src/support/logger.c
    src/support/str.c
  )
  target_include_directories(Flex-Bison-Compiler PRIVATE src)

  # Link final project and libraries.
  target_link_libraries(Flex-Bison-Compiler)
else ()
  message(NOTICE "The C compiler is unknown.")
endif ()
