
#include "alucell_legacy_database.hpp"

namespace alucell {

  void database_read_access::read_header() {
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


  database_read_access::database_read_access(): block_infos(8, 0) {}
  
  /*
   * Constuctor
   */
  database_read_access::database_read_access(const std::string& _filename)
    : filename(), dbfile(), index(), block_infos(8, 0) {
    open(_filename);
  }

  std::pair<unsigned int, unsigned int> database_read_access::read_array_size_infos(unsigned int offset) {
    double meta[2] = {0.};

    dbfile.seekg(offset, std::ios::beg);
    dbfile.read(reinterpret_cast<char*>(meta), 2 * sizeof(double));

    return std::make_pair(static_cast<unsigned int>(meta[0]),
			  static_cast<unsigned int>(meta[1]));
  }

  void database_read_access::open(const std::string& _filename) {
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

  void database_read_access::close() {
    /*
     * Clear state:
     */
    dbfile.close();
    filename = "";
    index.clear();
    std::fill(block_infos.begin(), block_infos.end(), 0);
  }

  void database_read_access::dump_database_infos(std::ostream& stream) {
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
  
}
