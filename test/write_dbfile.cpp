
#include "../src/alucell_legacy_database.hpp"

#include <algorithm>
#include <numeric>

/*
 *  Create a dbfile with a unique variable named 'array',
 *  which is a real array of 20 element and 1 components.
 *  The array is initialized with increasing integer values 
 *  from 1 to 20.
 */

int main(int argc, char *argv[]) {
  alucell::database_write_access db("dbfile");

  const std::string name("array");

  /*
   *  Initialize the data to be inserted
   */
  std::vector<double> values(22);
  std::iota(values.begin() + 2, values.end(), 1);
  values[0] = 20.;
  values[1] = 1.;


  /*
   *  Insert and close
   */
  db.insert(name, alucell::data_type::real_array, &values[0], values.size() * sizeof(double));
  db.close();
  
  return 0;
}
