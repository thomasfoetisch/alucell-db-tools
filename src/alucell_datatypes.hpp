#ifndef _ALUCELL_DATATYPES_H_
#define _ALUCELL_DATATYPES_H_

namespace alucell {

  enum class data_type { unknown = 0,
      real_array, element_array,
      matrix, sky_matrix,
      int_array,
      real_number,
      expression,
      string};

  inline std::string pretty_data_type(data_type t) {
    static const char* pretty_names[] = {"unknown",
					 "real_array", "element_array",
					 "matrix", "sky_matrix",
					 "int_array",
					 "real_number",
					 "expression",
					 "string"};

    return pretty_names[static_cast<unsigned int>(t)];
  }

  inline data_type pretty_name_to_data_type(const std::string& t) {
    if (t == "real_array") {
      return data_type::real_array;
    } else if (t == "element_array") {
      return data_type::element_array;
    } else if (t == "matrix") {
      return data_type::matrix;
    } else if (t == "sky_matrix") {
      return data_type::sky_matrix;
    } else if (t == "int_array") {
      return data_type::int_array;
    } else if (t == "real_number") {
      return data_type::real_number;
    } else if (t == "expression") {
      return data_type::expression;
    } else if (t == "string") {
      return data_type::string;
    } else {
      return data_type::unknown;
    }
  }
  
  inline data_type type_id_to_data_type(unsigned int i) {
    if (i > 10)
      throw std::string("Invalid type id");
      
    static const data_type types[] = {data_type::unknown,
				      data_type::real_array,
				      data_type::element_array,
				      data_type::matrix,
				      data_type::sky_matrix,
				      data_type::int_array,
				      data_type::real_number,
				      data_type::unknown,
				      data_type::unknown,
				      data_type::expression,
				      data_type::string};
    return types[i];
  }

  inline unsigned int data_type_to_type_id(data_type t) {
    static const unsigned int ids[] = {0, 1, 2, 3, 4, 5, 6, 9, 10};
    return ids[static_cast<unsigned int>(t)];
  }

  inline char type_id_to_type_char(unsigned int i) {
    if (i > 10)
      throw std::string("Invalid type id");
    
    return static_cast<char>(i + 'A' - 1);
  }
  
  inline unsigned int type_char_to_type_id(char t) {
    if (not (t >= 'A' and t <= 'J'))
      throw std::string("Invalid type char");
    
    return static_cast<unsigned int>(t - 'A' + 1);
  }

  

}

#endif /* _ALUCELL_DATATYPES_H_ */
