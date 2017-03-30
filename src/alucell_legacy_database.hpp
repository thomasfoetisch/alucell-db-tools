#ifndef _ALUCELLLEGACYDATABASE_
#define _ALUCELLLEGACYDATABASE_

#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

#include "string_utils.hpp"
#include "alucell_datatypes.hpp"

namespace alucell {

  class database_read_access {
  private:
    struct database_index_item {
      std::string name;  // Variable name
      unsigned int length;  // variable data length
      unsigned int offset;  // variable data offset in file
      data_type type;

      database_index_item(const std::string& variable_id, unsigned int l, unsigned int o)
	: name(), length(l), offset(o), type(data_type::unknown) {
	if (variable_id.size() < 3)
	  throw std::string("Invalid variable identifier");
	
	type = type_id_to_data_type(type_char_to_type_id(variable_id[0]));
	name = std::string(variable_id.begin() + 2, variable_id.end());
      }
    };

    /*
     * Database access state:
     */
    std::string filename;
    std::ifstream dbfile;
    std::vector<database_index_item> index;
    std::vector<unsigned int> block_infos;
  
    void read_header();

    std::pair<unsigned int, unsigned int> read_array_size_infos(unsigned int offset);

  public:
    /*
     * Constuctors
     */
    database_read_access();
    
    database_read_access(const std::string& _filename);

    void open(const std::string& _filename);

    void close();

    void dump_database_infos(std::ostream& stream);

    /*
     * Accessors for the variables properties.
     */
    unsigned int get_variable_size(unsigned int id) const { return index[id].length; }
    data_type get_variable_type(unsigned int id) const { return index[id].type; }
    const std::string& get_variable_name(unsigned int id) const { return index[id].name; }
    void read_data_from_database(unsigned int id, void* dst) {
      dbfile.seekg(index[id].offset, std::ios::beg);
      dbfile.read(reinterpret_cast<char*>(dst), index[id].length);
    }
    unsigned int get_variables_number() const { return index.size(); }
  };

}

#endif
