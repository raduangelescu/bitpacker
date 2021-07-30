#define CATCH_CONFIG_RUNNER

#include "bitpacker.h"
#include <catch2/catch.hpp>

int main(int argc, char *argv[]) {
  const size_t c_Seed = 0xFEFE;
  srand(c_Seed);
  int result = Catch::Session().run(argc, argv);

  // global clean-up...

  return result;
}
