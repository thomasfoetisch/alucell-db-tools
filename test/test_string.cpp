#include <iostream>

#include "string_utils.hpp"

int main(int argc, char *argv[]) {
  std::cout << suffixed("cuveb_nodes", "_nodes") << std::endl;
  
  std::cout << suffixed("cuveb_nodes_max", "_nodes") << std::endl;
  
  std::cout << suffixed("cuveb_refs", "_nodes") << std::endl;
  
  return 0;
}
