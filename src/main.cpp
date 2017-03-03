#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <functional>
#include <locale>

class ensight6_reader {
public:
  enum class element_type {
    point1, bar2, bar3, triangle3, triangle6, quadrangle4, quadrangle8,
      tetrahedron4, tetrahedron10, pyramid5, pyramid13,
      hexahedron8, hexahedron20, pentahedron6, pentahedron15
  };
  
  ensight6_reader(const std::string& filename) {
    
  }

  static bool read_line(std::istream& stream, std::string& line) {
    std::getline(stream, line);
    std::string::iterator pos(std::find(line.begin(), line.end(), '#'));

    if (pos != line.end())
      line.erase(pos, line.end());
    
    return stream.good();
  }

  static void trim_left(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
				    std::not1(std::ptr_fun<int, int>(std::isspace))));
  }

  static void trim_right(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
			 std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
	    s.end());
  }

  static void trim(std::string& s) {
    trim_left(s);
    trim_right(s);
  }

  static void normalize_white_space(std::string& s) {
    TODO
  }

  static bool read_non_empty_line(std::istream& stream, std::string& line) {
    do {
      read_line(stream, line);
      trim(line);
    } while (line.size() == 0 and stream.good());
    
    return stream.good();
  }

  /*
   * section_header = FORMAT | GEOMETRY | VARIABLE | TIME | FILE
   * section = section_header field*;
   * case_file = section*;
   */
  void read_case_file() {
    


  }
  void read_format_section() {}
  void read_geometry_section() {}
  void read_variable_section() {}
  void read_time_section() {}
  void read_file_section() {}
};


bool read_line_test(const std::string& test_line) {
  std::istringstream iss(test_line);
  std::string line;
  bool result(true);

  result = ensight6_reader::read_non_empty_line(iss, line);
  std::cout << ">" << test_line << "<" << " -> >" << line << "<" << std::endl;
  return result;
}

void read_non_empty_line_test(const std::string& filename) {
  std::ifstream file(filename.c_str(), std::ios::in);
  std::string line;

  ensight6_reader::read_non_empty_line(file, line);
  while (line.size()) {
    std::cout << ">" << line << "<" << std::endl;
    ensight6_reader::read_non_empty_line(file, line);
  } 
}

int main(int argc, char *argv[]) {
  /*
  read_line_test("hello les gens");
  read_line_test("\nhello les gens");
  read_line_test("#");
  read_line_test("\n#");
  read_line_test("oetu#");
  read_line_test("#h");
  read_line_test("h#");
  read_line_test("te#t");
  read_line_test("toehun   # oetuhe\n");
  read_line_test("#\n\noetu#oetu\n");
  */
  read_non_empty_line_test("input.txt");
  
  return 0;
}
