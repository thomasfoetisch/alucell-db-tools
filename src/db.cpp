
#include <iostream>
#include <iomanip>
#include <set>
#include <map>
#include <cctype>
#include <algorithm>

#include <unistd.h>

#include "alucelldb.hpp"

const char* usage_message =
  "USAGE: db <action> <db_filename> [<action-options> ...]\n"
  "  Inspect and manipulate the content of alucell database files.\n"
  "\n"
  "The db command is a toolbox, where each tool is selected by giving\n"
  "the appropriate <action> keyword. <action> can be one of 'ls', 'dump',\n"
  "'mesh', 'info' and 'show'. Each action needs a dbfile to work with, and\n"
  "possibly some additional parameters.\n"
  "See 'dbfile <action> <db_filename> -h for more information about the\n"
  "action <action>.\n"
  "\n"
  "Notes on the syntax used in the documentation.\n"
  "All the parameters on the command lines are mandatory, unless specified\n"
  "differently. Optional options that may appear at most once are written\n"
  "between square brackets (e.g. [-n]). Options that can appear zero or more\n"
  "times are written with a trailing Kleen star '*' (e.g. -<n>*), and options\n"
  "that can appear at least once or more times use the trailing '+' (e.g. \n"
  "<var_name>+). Keywords between angle brackets (e.g. <action>) are\n"
  "metasynctactical variables to denote a set of possible values. For example\n"
  "the syntax <db_filename> is used to denote any possible textual\n"
  "representation of a valid path to a dbfile.";

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

const char* meshes_help_message =
  "USAGE: db mesh <db_filename> [-h] [-a] [-n] [-e] [-s] -<n>* <mesh_name>*\n"
  "  List meshes that are defined in the dbfile, and associated variables.\n"
  "\n"
  "Without any options, only the names of the detected meshes are listed. If\n"
  "some <mesh_name> are provided, only thoses meshes will be displayed, if they\n"
  "exist. No error will occur if they don't.\n"
  "The 'mesh' action accept the following options:\n"
  "  -n           List the variables defined on the nodes of the mesh.\n"
  "  -e           List the variables defined on the elements of the mesh.\n"
  "  -s           List the scalar variable that are associated with the mesh.\n"
  "  -a           Imply the options -n -e -s.\n"
  "  -<n>         List only the nodal or elemental variables defined on the mesh\n"
  "               which have <n> components. This option can occur multiple times.\n"
  "               <n> is expected to be any valid integer number in base 10,\n"
  "               e.g. 1, 3, 16 or 42."
  "  -h           Show this message.\n"
  "  <mesh_name>  Restrict the displayed meshes to the <mesh_name> specified.\n"
  "               Multiples <mesh_name> can be specified.\n"
  "\n"
  "Examples\n"
  "  $ db mesh dbfile_stat\n"
  "     List all the meshes detected in the file 'dbfile_stat'.\n\n"
  "  $ db mesh dbfile_stat cuveb -a\n"
  "     List all the variables (scalar, nodal, elemental) defined on the mesh 'cuveb'.\n\n"
  "  $ db mesh dbfile_stat cuveb -a -3 -16\n"
  "     List all the scalar variables, and all the nodal and elemental variables which have\n"
  "     exactly 3 components or 16 components (typically vectors in R^3).\n\n"
  "  $ db mesh dbfile_stat POINT0$(seq 1 9) -e\n"
  "     Assuming a Bash shell environment, list the elemental variables of the meshes 'POINT01'\n"
  "     'POINT02', ..., 'POINT09' detected in the file 'dbfile_stat'.\n";

const char* dump_help_message =
  "USAGE: db dump <db_filename> [-h] <var_name>+\n"
  "  Show the content of the variable names given on the \n"
  "  command line. Minimal formatting is performed to make\n"
  "  the content readable.\n"
  "\n"
  "The output obviously depends on the type of the variable \n"
  "dumped. For the real numbers, the value is printed in ascii,\n"
  "up to 12 digits. For real, integer and element arrays, all the\n"
  "components are printed, one row per line, separated by spaces. \n"
  "Strings are dumped without further interpretation or conversion.\n"
  "For expressions, a disassembly of the bytecode is displayed, which\n"
  "can allow, in principle, evaluation of the function by hand.\n"
  "\n"
  "The matrix and sky_matrix are not yet supported.";

const char* list_help_message =
  "USAGE: db ls <db_filename> [-h] [-t <datatype>]*\n"
  "  List the variables in the 'db_filename' file.\n"
  "\n"
  "Without options, all the variables are listed, alongside with its\n"
  "datatype and size in bytes. Possible datatypes are 'real_array',\n"
  "'element_array', 'matrix', 'sky_matrix', 'int_array', 'real_number',\n"
  "'expression' and 'string'.\n"
  "\n"
  "The 'ls' action accepts the following options:\n"
  "  -t <datatype>  Restrict the listed variable to this datatype only.\n"
  "                 Multiple occurences of this option can be used. In this\n"
  "                 case, only variable whose datatype is one of the datatypes\n"
  "                 specified with the -t option are listed.\n"
  "  -h             Print this message.\n";

const char* show_help_message =
  "USAGE: db show <db_filename> [-h] <var_name>+\n"
  "  Show a human readable summary of the content of the variable names given \n"
  "  on the command line.\n";

const char* info_help_message =
  "USAGE: db info <db_filename> [-h]\n"
  "  Show the content of the infoblock stored in the dbfile.\n";


inline
void check_file_read_accessibility(const std::string& filename, const std::string& error_msg) {
  if (access(filename.c_str(), R_OK) != 0)
    switch (errno) {
    case EACCES:
      throw error_msg + " (you don't have read access rights on this file)";
    case ELOOP:
      throw error_msg + " (too many symbolic links)";
    case ENAMETOOLONG:
      throw error_msg + " (filename is too long)";
    case ENOTDIR:
      throw error_msg + " (regular file used as a file path component)";
    case ENOENT:
      throw error_msg + " (one of the components of the path does not exist)";
    default:
      throw error_msg;
    }
}

void dump_variable_value(int argc, char* argv[]) {
  if (argc == 0)
    throw std::string("Expecting database filename.");
  
  if (argc == 1) {
    throw std::string("Expecting variable name(s).");
  } else {
    const std::string db_filename(argv[0]);
    check_file_read_accessibility(db_filename, db_filename + " is not accessible");
    
    --argc;
    ++argv;

    std::vector<std::string> variables_to_dump;
    while (argc > 0) {
      if (argv[0] == std::string("-h")) {
	std::cout << dump_help_message << std::endl;
	return;
      } else {
	variables_to_dump.push_back(argv[0]);
      }

      --argc;
      ++argv;
    }
  
    alucell::database_read_access db(db_filename);
    alucell::database_index index(&db);

    for (const auto& name: variables_to_dump) {
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

  const std::string db_filename(argv[0]);
  check_file_read_accessibility(db_filename, db_filename + " is not accessible");

  --argc;
  ++argv;

  bool list_scalar(false), list_nodal(false), list_elemental(false);
  std::set<std::string> meshes_to_list;
  std::set<unsigned int> vector_ranks_to_list;
  while (argc) {
    if (argv[0] == std::string("-h")) {
      std::cout << meshes_help_message << std::endl;
      return;
    } else if (argv[0] == std::string("-e")) {
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

  alucell::database_read_access db(db_filename);
  alucell::database_index index(&db);

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
              else if (a.get_size() == elements.get_size() and list_elemental)
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


void database_info(int argc, char* argv[]) {
  if (argc < 1)
    throw std::string("info: wrong number of arguments.");

  const std::string db_filename(argv[0]);
  check_file_read_accessibility(db_filename, db_filename + " is not accessible");

  --argc;
  ++argv;

  while (argc) {
    if (argv[0] == std::string("-h")) {
      std::cout << info_help_message << std::endl;
      return;
    } else {
      throw std::string("Wrong argument.");
    }
    --argc;
    ++argv;
  }

  alucell::database_read_access db(db_filename);
  db.dump_database_infos(std::cout);
}

void list_dbfile_content(int argc, char* argv[]) {
  if (argc < 1)
    throw std::string("ls: wrong number of arguments.");

  const std::string db_filename(argv[0]);
  check_file_read_accessibility(db_filename, db_filename + " is not accessible");

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
    } else if (argv[0] == std::string("-h")) {
      std::cout << list_help_message << std::endl;
      return;
    } else {
      throw std::string("Wrong argument.");
    }
  }

  alucell::database_read_access db(db_filename);
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
void show_array(alucell::database_read_access* db, unsigned int id) {
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
    const std::string db_filename(argv[0]);
    check_file_read_accessibility(db_filename, db_filename + " is not accessible");

    --argc;
    ++argv;

    std::vector<std::string> variables_to_show;
    while (argc > 0) {
      if (argv[0] == std::string("-h")) {
	std::cout << show_help_message << std::endl;
	return;
      } else {
	variables_to_show.push_back(argv[0]);
      }

      --argc;
      ++argv;
    }


    alucell::database_read_access db(db_filename);
    alucell::database_index index(&db);

    for (const auto& name: variables_to_show) {
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
    }
  }
}

void print_usage() {
  std::cout << usage_message << std::endl;
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
  } else if (std::string("info") == argv[0]) {
    database_info(argc - 1, argv + 1);
  } else if (std::string("-h") == argv[0]){
    print_usage();
  } else {
    throw std::string("Unknown action ") + argv[0];
  }
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
