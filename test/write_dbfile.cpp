
#include "../src/alucell_legacy_database.hpp"

#include <algorithm>
#include <numeric>

int main(int argc, char *argv[]) {
  alucell::database_write_access db("dbfile");

  const std::string name("array");
  std::vector<double> values(22);
  std::iota(values.begin() + 2, values.end(), 1);
  values[0] = 20.;
  values[1] = 1.;

  std::string item_name;
  item_name += alucell::type_id_to_type_char(alucell::data_type_to_type_id(alucell::data_type::real_array));
  item_name += '_';
  item_name += name;
  
  db.insert(item_name, alucell::data_type::real_array, &values[0], values.size() * sizeof(double));

  db.close();
  
  return 0;
}
