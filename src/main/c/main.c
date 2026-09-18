#include <support/types.h>

#include <frontend/lexer.h>

i32 main(i32 argc, const char **argv) {
  LexicalAnalyzer *la = la_create();

  la_free(la);
  return 0;
}
