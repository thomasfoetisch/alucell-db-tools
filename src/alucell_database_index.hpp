#ifndef _ALUCELL_DATABASE_INDEX_H_
#define _ALUCELL_DATABASE_INDEX_H_

#include <map>
#include <string>

#include "alucell_legacy_database.hpp"


namespace alucell {

class database_index {
public:
  database_index (alucell::database_read_access* db) {
    for (unsigned int id(0); id < db->get_variables_number(); ++id)
      index[db->get_variable_name(id)] = id;
  }

  unsigned int get_variable_id(const std::string& name) {
    auto it(index.find(name));

    if (it == index.end())
      throw std::string("Variable not found.");

    return it->second;
  }

  bool exists(const std::string& name) {
    auto it(index.find(name));
    return it != index.end();
  }

private:
  std::map<std::string, unsigned int> index;
};

}

#endif /* _ALUCELL_DATABASE_INDEX_H_ */
