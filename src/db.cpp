
#include <iostream>
#include <iomanip>
#include <set>
#include <map>
#include <cctype>
#include <algorithm>

#include "alucell_legacy_database.hpp"

#include "alucell_legacy_variable.hpp"


class database_index {
public:
  database_index (alucell::database_raw_access* db) {
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

const char* matrix_message =
  "Full matrix data type is long gone, dude. If you really wanna \n"
  "see what's in this variable, you'll have to implement that. It's \n"
  "pretty easy, the row and column number are stored in int*4 \n"
  "format in the first 8 byte of the data. The rest are the \n"
  "components of the matrix, in fortran array layout. Good luck!";

const char* skymatrix_message =
  "Sky matrix data type is long gone, dude. If you really wanna \n"
  "see what's in this variable, you'll have to understand how it's \n"
  "stored in memory, and implement that. You can start by reading ds.f \n"
  "and vpiter.f. Good luck!";

void dump_variable_value(int argc, char* argv[]) {
  if (argc == 0)
    throw std::string("Expecting database filename.");
  
  if (argc == 1) {
    throw std::string("Expecting variable name(s).");
  } else {
    const std::string filename(argv[0]);
    
    --argc;
    ++argv;
  
    alucell::database_raw_access db(filename);
    database_index index(&db);
  
    while (argc > 0) {
      const std::string name(argv[0]);
      const unsigned int id(index.get_variable_id(name));
	
      switch (db.get_variable_type(id)) {
      case alucell::data_type::real_array:
	{
	  std::cout.precision(12);
	    
	  alucell::variable::array<double> v(&db, id);
	  for (unsigned int i(0); i < v.get_size(); ++i) {
	    for (unsigned int j(0); j < v.get_components(); ++j)
	      std::cout << std::setw(16) << std::right << v.get_value(i, j);
	    std::cout << std::endl;
	  }
	}
	break;
	  
      case alucell::data_type::matrix:
	std::cout << matrix_message << std::endl;
	break;
	  
      case alucell::data_type::sky_matrix:
	std::cout << skymatrix_message << std::endl;
	break;
	  
      case alucell::data_type::element_array:	  
      case alucell::data_type::int_array:
	{
	  std::cout.precision(12);
	    
	  alucell::variable::array<int> v(&db, id);
	  for (unsigned int i(0); i < v.get_size(); ++i) {
	    for (unsigned int j(0); j < v.get_components(); ++j)
	      std::cout << std::setw(16) << std::right << v.get_value(i, j);
	    std::cout << std::endl;
	  }
	}
	break;
	  
      case alucell::data_type::real_number:
	std::cout << alucell::variable::number(&db, id).get_value() << std::endl;
	break;
	  
      case alucell::data_type::expression:
	{
	  alucell::variable::expression v(&db, id);
	  alucell::expression_decoder d(&v);
	  d.dump_bytecode_assembly(std::cout);
	}
	break;
	  
      case alucell::data_type::string:
	std::cout << alucell::variable::string(&db, id).get_value() << std::endl;
	break;

      case alucell::data_type::unknown:
      default:
	throw std::string("unknown datatype.");
      }

      --argc;
      ++argv;
    }
  }
}


void list_dbfile_meshes(int argc, char* argv[]) {
  if (argc < 1)
    throw std::string("Wrong number of arguments");

  const std::string dbfile_name(argv[0]);
  --argc;
  ++argv;

  bool list_scalar(false), list_nodal(false), list_elemental(false);
  std::set<std::string> meshes_to_list;
  std::set<unsigned int> vector_ranks_to_list;
  while (argc) {
    if (argv[0] == std::string("-e")) {
      list_elemental = true;
    } else if (argv[0] == std::string("-s")) {
      list_scalar = true;
    } else if (argv[0] == std::string("-n")) {
      list_nodal = true;
    } else if (argv[0] == std::string("-a")) {
      list_scalar = true;
      list_nodal = true;
      list_elemental = true;
    } else if (argv[0][0] == '-'
               and std::all_of<const char*, int (*)(int)>(argv[0] + 1,
                                                          argv[0] + std::strlen(argv[0]),
                                                          std::isdigit)) {
      vector_ranks_to_list.insert(std::strtoul(argv[0] + 1,
                                                 NULL,
                                                 10));
    } else {
      meshes_to_list.insert(argv[0]);
    }
    --argc;
    ++argv;
  }

  alucell::database_raw_access db(dbfile_name);
  database_index index(&db);

  std::set<std::string> potential_mesh_names;
  for (unsigned int i(0); i < db.get_variables_number(); ++i) {
    const std::string var_name(db.get_variable_name(i));
    if (suffixed(var_name, "_nodes")
	and db.get_variable_type(i) == alucell::data_type::real_array)
      potential_mesh_names.insert(var_name.substr(0, var_name.size() - 6));
  }
  
  for (auto it(potential_mesh_names.begin());
       it != potential_mesh_names.end();) {
    if (meshes_to_list.size() and not meshes_to_list.count(*it)) {
      potential_mesh_names.erase(it++);
      continue;
    }

    const std::string elems_array_name(*it + "_elems");
    const std::string refs_array_name(*it + "_refs");

    if (index.exists(elems_array_name)) {
      const unsigned int id(index.get_variable_id(elems_array_name));
      if (db.get_variable_type(id) != alucell::data_type::int_array and
	  db.get_variable_type(id) != alucell::data_type::element_array) {
	potential_mesh_names.erase(it++);
	continue;
      }
    } else {
      potential_mesh_names.erase(it++);
      continue;
    }

    if (index.exists(refs_array_name)) {
      const unsigned int id(index.get_variable_id(refs_array_name));
      if (db.get_variable_type(id) != alucell::data_type::int_array) {
	potential_mesh_names.erase(it++);
	continue;
      }
    } else {
      potential_mesh_names.erase(it++);
      continue;
    }
    ++it;
  }

  for (const auto& mesh_name: potential_mesh_names) {
    std::cout << mesh_name << ": ";

    alucell::variable::array<double> nodes(&db, index.get_variable_id(mesh_name + "_nodes"));
    std::cout << nodes.get_size() << " nodes, ";

    alucell::variable::array<int> elements(&db, index.get_variable_id(mesh_name + "_elems"));
    std::cout << elements.get_size() << " elements." << std::endl;
    
    for (unsigned int i(0); i < db.get_variables_number(); ++i) {
      const std::string var_name(db.get_variable_name(i));
      if (prefixed(var_name, mesh_name + "_")
	  and not suffixed(var_name, "_nodes")
	  and not suffixed(var_name, "_refs")) {
	switch (db.get_variable_type(i)) {
	case alucell::data_type::real_array:
	  if (list_nodal or list_elemental) {
	    alucell::variable::array<double> a(&db, i);
            if (vector_ranks_to_list.size() == 0 or
                vector_ranks_to_list.count(a.get_components())) {
              if (a.get_size() == nodes.get_size() and list_nodal)
                std::cout << "  nodal variable: " << var_name.substr(mesh_name.size() + 1) << std::endl;
              if (a.get_size() == elements.get_size() and list_elemental)
                std::cout << "  elemental variable: " << var_name.substr(mesh_name.size() + 1) << std::endl;
            }
	  }
	  break;
	  
	case alucell::data_type::real_number:
	  if (list_scalar)
	    std::cout << "  scalar variable: " << var_name.substr(mesh_name.size() + 1) << std::endl;
	  break;
	  
	default:
	  break;
	}
      }
    }
    std::cout << std::endl;
  }
}


void extract_variables_to_dbfile(int argc, char* argv[]) {
  std::cout << "TODO" << std::endl;
}


void list_dbfile_content(int argc, char* argv[]) {
  if (argc < 1)
    throw std::string("ls: wrong number of arguments.");

  const std::string db_filename(argv[0]);
  --argc;
  ++argv;
  
  std::set<alucell::data_type> included_types;
  while (argc) {
    if (argv[0] == std::string("-t") and argc >= 2) {
      alucell::data_type t(alucell::pretty_name_to_data_type(argv[1]));
      if (t == alucell::data_type::unknown)
	throw std::string("Invalid datatype name.");
      
      included_types.insert(t);
      argc -= 2;
      argv += 2;
    } else {
      throw std::string("Wrong argument.");
    }
  }

  alucell::database_raw_access db(db_filename);
  for (unsigned int i(0); i < db.get_variables_number(); ++i) {
    if (included_types.size() == 0
	or (included_types.count(db.get_variable_type(i)) > 0))
      std::cout << std::setw(14) << std::left << alucell::pretty_data_type(db.get_variable_type(i))
		<< std::setw(10) << std::right << db.get_variable_size(i)
		<< "  " << db.get_variable_name(i) << std::endl;
  }
}

std::string print_memory_size(unsigned long s) {
  const char* prefixes[] = {" byte", " Kilobyte", " Megabyte", " Gigabyte", " Terabyte", " Exabyte", " Petabyte"};

  unsigned int prefix_id(0);
  while (s > 1000 and prefix_id < 7) {
    ++prefix_id;
    s /= 1000;
  }

  return std::to_string(s) + prefixes[prefix_id];
}

template<typename T>
void show_array(alucell::database_raw_access* db, unsigned int id) {
  alucell::variable::array<T> v(db, id);
  std::cout << "  rows: " << v.get_size() << std::endl;
  std::cout << "  components: " << v.get_components() << std::endl;
  std::cout << "  elements: " << v.get_size() * v.get_components() << std::endl;
  std::cout << "  memory: " << print_memory_size(v.get_size() * v.get_components() * sizeof(T) + 2 * sizeof(double)) << std::endl;
  for (unsigned int c(0); c < v.get_components(); ++c) {
    T
      min(std::numeric_limits<T>::max()),
      max(std::numeric_limits<T>::lowest());
    for (unsigned int i(0); i < v.get_size(); ++i) {
      min = min > v.get_value(i, c) ? v.get_value(i, c) : min;
      max = max < v.get_value(i, c) ? v.get_value(i, c) : max;
    }
    std::cout << "  component " << c << " range: [" << min << ", " << max << "]" << std::endl;
  }
}
  
void show_variable(int argc, char* argv[]) {
  if (argc == 0)
    throw std::string("show: wrong number of arguments.");

  if (argc == 1) {
    throw std::string("Expecting variable name(s).");
  } else {
    const std::string dbfile_name(argv[0]);
    --argc;
    ++argv;

    alucell::database_raw_access db(dbfile_name);
    database_index index(&db);

    while (argc > 0) {
      const std::string name(argv[0]);
      const unsigned int id(index.get_variable_id(name));

      switch (db.get_variable_type(id)) {
      case alucell::data_type::real_array:
	std::cout << name << ": real number array" << std::endl;;
	show_array<double>(&db, id);
	break;
	
      case alucell::data_type::int_array:
      case alucell::data_type::element_array:
	std::cout << name << ": integer/element array" << std::endl;;
	show_array<int>(&db, id);
	break;
	
      case alucell::data_type::matrix:
	std::cout << matrix_message << std::endl;
	break;
	
      case alucell::data_type::sky_matrix:
	std::cout << skymatrix_message << std::endl;
	break;

      case alucell::data_type::real_number:
	{
	  std::cout << name << ": real number" << std::endl;
	  alucell::variable::number v(&db, id);
	  std::cout << "  value: " << v.get_value() << std::endl;
	}
	break;
	
      case alucell::data_type::expression:
	{
	  std::cout << name << ": expression" << std::endl;
	  alucell::variable::expression v(&db, id);
	  alucell::expression_decoder d(&v);
	  std::cout << "  domain dimension: " << d.get_input_rank() << std::endl;
	  std::cout << "  codomain dimension: " << d.get_output_rank() << std::endl;
	}
	break;
	
      case alucell::data_type::string:
	{
	  std::cout << name << ": character string" << std::endl;
	  alucell::variable::string v(&db, id);
	  std::cout << "  value: '" << v.get_value() << "'" << std::endl;
	  std::cout << "  length: " << v.get_value().size() << std::endl;
	}
	break;
	
      default:
	throw std::string("Unknown type.");
      }

      --argc;
      ++argv;
    }
  }
}

void parse_action(int argc, char* argv[]) {
  if (std::string("ls") == argv[0]) {
    list_dbfile_content(argc - 1, argv + 1);
  } else if (std::string("show") == argv[0]) {
    show_variable(argc - 1, argv + 1);
  } else if (std::string("dump") == argv[0]) {
    dump_variable_value(argc - 1, argv + 1);
  } else if (std::string("mesh") == argv[0]) {
    list_dbfile_meshes(argc - 1, argv + 1);
  } else if (std::string("extract") == argv[0]) {
    extract_variables_to_dbfile(argc - 1, argv + 1);
  } else {
    throw std::string("Unknown action ") + argv[0];
  }
}

void print_usage() {
  static const char* message =
    "USAGE: db <action> [<args>] <dbfile> [<dbfile> ...]\n"
    "  Inspect and manipulate the content of alucell database files.\n"
    "\n"
    "The db command is a toolbox, where each tool is selected by giving\n"
    "the appropriate <action> keyword. <action> can be one of 'ls', 'dump', 'mesh', 'show', 'extract'.";
  
  std::cout << message << std::endl;
}


int main(int argc, char *argv[]) {

  try {
    if (argc >= 2)
      parse_action(argc - 1, argv + 1);
    else
      print_usage();
    return 0;
  }
  catch (const std::string& e) {
    std::cout << e << std::endl;
    return 1;
  }
}
