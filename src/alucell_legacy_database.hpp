#ifndef _ALUCELLLEGACYDATABASE_
#define _ALUCELLLEGACYDATABASE_

#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

#include "string_utils.hpp"
#include "alucell_datatypes.hpp"

namespace alucell {

  class database_raw_access {
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
  
    void read_header() {
      // Note: From here we assume that the file is open and readable,
      //       and that the state is clean (variablesInfos and blockInfos
      //       reinitialized).

      // Random constant used in Alucell legacy code:
      const unsigned int max_saved_vectors(26500);

      std::vector<unsigned int>
	lengths_buffer(max_saved_vectors, 0),
	offsets_buffer(max_saved_vectors, 0);
      std::vector<char> 
	names_buffer(max_saved_vectors * 4 * sizeof(double), 0);
  
      /* 
       * Read the lengths, offsets, names and info block
       * from the file into the buffer:
       */
      dbfile.read(reinterpret_cast<char*>(&lengths_buffer[0]),
		  sizeof(unsigned int)*lengths_buffer.size());

      dbfile.read(reinterpret_cast<char*>(&offsets_buffer[0]),
		  sizeof(unsigned int)*offsets_buffer.size());

      dbfile.read(&names_buffer[0], names_buffer.size());

      dbfile.read(reinterpret_cast<char*>(&block_infos[0]),
		  sizeof(unsigned int)*block_infos.size());
  
      /*
       * Read the vector names list:
       */
      const unsigned int& slots_number(block_infos[2]);
      for(unsigned vector_idx(0), offset(0); offset < slots_number; ++offset, ++vector_idx)
	{
	  /*
	   * For each variable, find the size of the name in multiple of 32char:
	   */
	  unsigned int vector_name_size(1);
	  while(offsets_buffer[offset] == 0) {
            ++vector_name_size;
            ++offset;
          }

	  /*
	   * Extract the name of the stored vector:
	   */
	  const std::string vector_name(names_buffer.begin()
					+ (offset - (vector_name_size - 1)) * 4 * sizeof(double), 
					names_buffer.begin()
					+ (offset + 1) * 4 * sizeof(double));

	  const database_index_item
	    item(trimmed(vector_name),
		 lengths_buffer[offset] * static_cast<unsigned int>(sizeof(double)),
		 (offsets_buffer[offset] - 1) * static_cast<unsigned int>(sizeof(double)));
	
	  index.push_back(item);
	}
    }

    std::pair<unsigned int, unsigned int> read_array_size_infos(unsigned int offset) {
      double meta[2] = {0.};

      dbfile.seekg(offset, std::ios::beg);
      dbfile.read(reinterpret_cast<char*>(meta), 2 * sizeof(double));

      return std::make_pair(static_cast<unsigned int>(meta[0]),
			    static_cast<unsigned int>(meta[1]));
    }

  public:
    database_raw_access(): block_infos(8, 0) {}
  
    /*
     * Constuctor
     */
    database_raw_access(const std::string& _filename)
      : filename(), dbfile(), index(), block_infos(8, 0) {
      open(_filename);
    }

    void open(const std::string& _filename) {
      close();
    
      /*
       * Load new database:
       */
      dbfile.open(_filename.c_str(), std::ios::in | std::ios::binary);
    
      if(!dbfile)
	throw std::string("[error] read_header(filename): Unable to read dbfile.");

      filename = _filename;
      read_header();
    }

    void close() {
      /*
       * Clear state:
       */
      dbfile.close();
      filename = "";
      index.clear();
      std::fill(block_infos.begin(), block_infos.end(), 0);
    }

    void dump_database_infos(std::ostream& stream) {
      /*
       * Dump the info block content:
       */
      stream << "Alucell database \"" << filename << "\":" << std::endl << std::endl;

      stream << "Last used block offset: " << block_infos[0] << std::endl;
      stream << "Fortran i/o unit: " << block_infos[1] << std::endl;
      stream << "Total name slots used: " << block_infos[2] << std::endl;
      stream << "Offset of length table: " << block_infos[3] << std::endl;
      stream << "Offset of storage offset table: " << block_infos[4] << std::endl;
      stream << "Offset of block info: " << block_infos[5] << std::endl;
      stream << "Block size in sizeof(double): " << block_infos[6] << std::endl;
      stream << "Max number of stored vectors: " << block_infos[7] << std::endl;
    }

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
  

  /*
  database_raw_access::read_array_from_database(unsigned int id) {
  *
   * Find the file entry in the index vector:
   *
  std::vector<database_raw_access::database_index_item>::iterator infos(findElementByName(variableName));

  std::pair<unsigned int, unsigned int> dim(readArraySizeInfos(getVarOffset(*infos)));

  typename Variable<T>::Ptr var(new Variable<T>(dim.first, dim.second));
  dbfile.seekg(getVarOffset(*infos) + 2*sizeof(double), std::ios::beg);
  dbfile.read(reinterpret_cast<char*>(&var->at(0,0)), var->getSize()*sizeof(T));

  return var;
}
*/

}

#endif
