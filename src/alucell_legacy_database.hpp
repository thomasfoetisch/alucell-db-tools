#ifndef _ALUCELLLEGACYDATABASE_
#define _ALUCELLLEGACYDATABASE_

#include <iostream>

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

  
  class database_write_access {
  public:
    database_write_access():
      lengths_buffer_offset(0),
      offsets_buffer_offset(26500 / 2 * sizeof(double)),
      names_buffer_offset(132500 * sizeof(double)),
      last_block_offset(names_buffer_offset + 8 * sizeof(int)),
      used_slots_number(0),
      item_number(0) {};

    
    database_write_access(const std::string& _filename):
      lengths_buffer_offset(0),
      offsets_buffer_offset(26500 / 2 * sizeof(double)),
      names_buffer_offset(132500 * sizeof(double)),
      last_block_offset(names_buffer_offset + 8 * sizeof(int)),
      used_slots_number(0),
      item_number(0) {
      open(_filename);
    };

    ~database_write_access() {
      close();
    }
    

    void open(const std::string& _filename) {
      close();

      dbfile.open(_filename.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
      if (!dbfile)
	throw std::string("[error] database_write_access::open(filename): Unable to open dbfile.");

      filename = _filename;

      lengths_buffer_offset = 0;
      offsets_buffer_offset = 26500 / 2 * sizeof(double);
      names_buffer_offset = 26500 * sizeof(double);
      last_block_offset = info_block_file_offset * sizeof(double) + 8 * sizeof(int);
      used_slots_number = 0;
      item_number = 0;
    }

    void close() {
      dbfile.close();
      filename = "";
      
      item_number = 0;
      lengths_buffer_offset = 0;
      offsets_buffer_offset = 0;
      names_buffer_offset = 0;
    }

    static std::string prepend_type_char(const std::string& name, alucell::data_type t) {
      std::string item_name(1, alucell::type_id_to_type_char(alucell::data_type_to_type_id(t)));
      item_name += '_';
      item_name += name;
      return  item_name;
    }
    
    void insert(std::string name, const alucell::data_type t, void* const data, const int size) {
      /*
       * Prepend the type character code at the front of the name
       */
      name = prepend_type_char(name, t);

      
      /*
       *  Resize the name to a multiple of the name slot's size
       */
      const unsigned int required_slots_number(name.size()/32 + 1);
      name.resize(required_slots_number * 4 * sizeof(double), ' ');

      std::cout << "Writing variable " << name
		<< ", will require " << required_slots_number
		<< " slots." << std::endl;

      
      /*
       *  Write the length of the variable in the length table,
       *  in units of sizeof(double)
       */
      int size_in_block(size / sizeof(double));
      dbfile.seekp(lengths_buffer_offset
		   + (required_slots_number - 1) * sizeof(size),
		   std::ios::beg);
      dbfile.write(const_cast<char*>(reinterpret_cast<const char*>(&size_in_block)), sizeof(size_in_block));
      lengths_buffer_offset += required_slots_number * sizeof(size);

      std::cout << "  writing the length (" << size_in_block << ") of the var at "
		<< dbfile.tellp() - static_cast<std::ios::pos_type>(sizeof(size)) << std::endl;


      /*
       *  Write the file offset of the variable's data in the offset table,
       *  in units of sizeof(double)
       */
      int last_block_offset_in_block(last_block_offset / sizeof(double) + 1);
      dbfile.seekp(offsets_buffer_offset
		   + (required_slots_number - 1) * sizeof(last_block_offset),
		   std::ios::beg);
      dbfile.write(reinterpret_cast<char*>(&last_block_offset_in_block), sizeof(last_block_offset_in_block));
      offsets_buffer_offset += required_slots_number * sizeof(last_block_offset);

      std::cout << "  writing the data offset (" << last_block_offset_in_block << ") of the var at "
		<< dbfile.tellp() - static_cast<std::ios::pos_type>(sizeof(last_block_offset)) << std::endl;


      /*
       *  Write the variable's name in the name slots
       */
      dbfile.seekp(names_buffer_offset,
		   std::ios::beg);
      dbfile.write(&name[0], name.size());
      names_buffer_offset += name.size();
      used_slots_number += required_slots_number;

      std::cout << "  writing the name of the var at "
		<< dbfile.tellp() - static_cast<std::ios::pos_type>(name.size()) << std::endl;

      
      /*
       *  Write the variable's data
       */
      dbfile.seekp(last_block_offset, std::ios::beg);
      dbfile.write(reinterpret_cast<char*>(data), size);
      last_block_offset += size;

      std::cout << "  writing the data of the var at "
		<< dbfile.tellp() - static_cast<std::ios::pos_type>(size)
		<< std::endl;

      
      
      item_number += 1;
      update_infos();
    }

    void update_infos() {
      std::vector<int> info_block = {
	last_block_offset / static_cast<int>(sizeof(double)),
	fortran_io_unit,
	used_slots_number,
	length_buffer_file_offset + 1,
	name_buffer_file_offset + 1,
	info_block_file_offset + 1,
	block_size,
	max_item_number
      };

      dbfile.seekp(info_block_file_offset * sizeof(double));
      dbfile.write(reinterpret_cast<char*>(&info_block[0]), 8 * sizeof(int));

      std::cout << "  writing the info block of the dbfile at "
		<< dbfile.tellp() - static_cast<std::ios::pos_type>(8 * sizeof(int))
		<< std::endl
		<< std::endl;
    }

  private:
    std::string filename;
    std::ofstream dbfile;

    static const int offset_buffer_file_offset = 26500 / 2;
    
    static const int fortran_io_unit = 3;
    static const int length_buffer_file_offset = 0;
    static const int name_buffer_file_offset = 26500;
    static const int info_block_file_offset = 132500;
    static const int block_size = 1;
    static const int max_item_number = 26500;
    
    int lengths_buffer_offset;
    int offsets_buffer_offset;
    int names_buffer_offset;
    int last_block_offset;

    int used_slots_number;
    int item_number;
  };
  
}

#endif
