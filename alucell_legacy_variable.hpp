#ifndef _ALUCELL_LEGACY_VARIABLE_H_
#define _ALUCELL_LEGACY_VARIABLE_H_

#include <map>
#include <cstring>

#include "string_utils.hpp"

namespace alucell {

  namespace variable {
    
    class basic_variable {
    public:
      virtual ~basic_variable() {}
      virtual unsigned int get_length() = 0;
      virtual void get_data(void* dst) = 0;
    };

    
    class string: public basic_variable {
    public:
      string(database_raw_access* db, unsigned int id) {
	std::vector<char> buffer(db->get_variable_size(id), 0);
	db->read_data_from_database(id, reinterpret_cast<double*>(&buffer[0]));
	std::size_t string_length(static_cast<std::size_t>(*reinterpret_cast<double*>(&buffer[0])));

	value = std::string(buffer.begin() + 2 * sizeof(double),
			    buffer.begin() + 2 * sizeof(double) + string_length);
      }
      
      const std::string& get_value() const { return value; }

      unsigned int get_length() { return value.size(); }
      void get_data(void* dst) { std::memcpy(dst, &value[0], get_length()); }
      
    private:
      std::string value;
    };

    
    class number: public basic_variable {
    public:
      number(database_raw_access* db, unsigned int id) {
	db->read_data_from_database(id, &value);
      }
      
      double get_value() const { return value; }

      unsigned int get_length() { return sizeof(value); }
      void get_data(void* dst) { std::memcpy(dst, &value, get_length()); }

    private:
      double value;
    };

    
    template<typename T>
    class array: public basic_variable {
    public:
      array(database_raw_access* db, unsigned int id) {
	buffer = new unsigned char[db->get_variable_size(id)];
	db->read_data_from_database(id, buffer);
	values = reinterpret_cast<T*>(buffer + 2 * sizeof(double));
	size = *reinterpret_cast<double*>(buffer);
	components = *reinterpret_cast<double*>(buffer + sizeof(double));
      }

      virtual ~array() { delete [] buffer; buffer = NULL; }
      
      unsigned int get_size() const { return size; }
      unsigned int get_components() const { return components; }
      T get_value(unsigned int i, unsigned int j) const {
	return values[i * components + j];
      }

      unsigned int get_length() { return size * components * sizeof(T); }
      void get_data(void* dst) { std::memcpy(dst, &values, get_length()); }

    private:
      unsigned char* buffer;
      unsigned int size;
      unsigned int components;
      T* values;
    };

    
    class matrix: public basic_variable {};
    class sky_matrix: public basic_variable {};

    
    class expression: public basic_variable {
    public:
      expression(database_raw_access* db, unsigned int id) {
	buffer.resize(db->get_variable_size(id));
	db->read_data_from_database(id, &buffer[0]);
      }

      unsigned int get_length() { return buffer.size(); }
      void get_data(void* dst) { std::memcpy(dst, &buffer[0], get_length()); }
    private:
      std::vector<unsigned char> buffer;
    };
    
  }

  
  namespace operators {

    struct rand: public std::unary_function<double, double> {
      double operator()(double) { return std::rand(); }
    };

    struct integer: public std::unary_function<double, double> {
      double operator()(double x) { return static_cast<long int>(x); }
    };

    struct integer2: public std::binary_function<double, double, double> {
      double operator()(double x, double y) { throw std::string("integer2 not implemented."); }
    };

    struct sqr: public std::unary_function<double, double> {
      double operator()(double x) { return x * x; }
    };

    struct cpu_time: public std::unary_function<double, double> {
      double operator()(double x) { throw std::string("cpu_time not implemented."); }
    };

    struct div_eucl: public std::binary_function<double, double, double> {
      double operator()(double x, double y) {
	long int a(x), b(y);
	return (a / b) + 1.e-15;
      }
    };

    struct nexteven: public std::unary_function<double, double> {
      double operator()(double x) {
	long int a(x);
	return a % 2 ? a : a + 1;
      }
    };

    struct inv: public std::unary_function<double, double> {
      double operator()(double x) {
	return x == 0.0 ? 0.0 : 1.0 / x;
      }
    };

    struct negate: public std::unary_function<double, double> {
      double operator()(double x) {
	return -x;
      }
    };

    struct min: public std::binary_function<double, double, double> {
      double operator()(double x, double y) {
	return x < y ? x : y;
      }
    };
  
    struct max: public std::binary_function<double, double, double> {
      double operator()(double x, double y) {
	return x < y ? y : x;
      }
    };


    template<double (*f_ptr)(double)>
    struct unary_function_wrapper: public std::unary_function<double, double> {
      double operator()(double x) { return f_ptr(x); }
    };

    template<double (*f_ptr)(double, double)>
    struct binary_function_wrapper: public std::binary_function<double, double, double> {
      double operator()(double x, double y) { return f_ptr(x, y); }
    };

  }


  class stack_machine {
  public:
    stack_machine();
  
    std::vector<double> run(const std::vector<double>& args,
			    const std::vector<double>& bytecode,
			    const std::map<std::string, std::vector<double> >& context) {
      reset();

      nj = args.size();
      computation_stack = args;
      call_stack.push_back(bytecode);
      instruction_pointers.push_back(&call_stack.back()[0]);

      print_stacks();
    
      while (not done) {
	std::cout << "current instruction: " << current_instruction() << std::endl;
	switch (current_instruction()) {
        case 0:
          increment_ptr(1);
          break;
          
        case 100:  // just after function call
          prepare_arguments();
          increment_ptr(1);
          break;
          
        case 200:  // end of list
          return_from_call();
          break;
          
        case 300:  // interpret symbol
          process_symbol(context);
          increment_ptr(7);
          break;

        case 400:  // push real value
          computation_stack.push_back(*(instruction_pointers.back() + 1));
          increment_ptr(2);
          break;

        case 1:  // add
          call_builtin(30);
          increment_ptr(1);
          break;

        case 2:  // sub
          call_builtin(31);
          increment_ptr(1);
          break;

        case 3:  // mult
          call_builtin(32);
          increment_ptr(1);
          break;

        case 4:  // div
          call_builtin(33);
          increment_ptr(1);
          break;

        case 5:  // pow
          call_builtin(34);
          increment_ptr(1);
          break;

        case 6:  // neg
          call_builtin(35);
          increment_ptr(1);
          break;

        default:
          throw std::string("Unknown instruction.");
          break;
	}
	print_stacks();
      }

      return computation_stack;
    }

  private:
    std::map<std::size_t, void (stack_machine::*)()> builtins;
  
    std::vector<std::vector<double> > call_stack;
    std::vector<double*> instruction_pointers;
    
    std::vector<double> argument_stack, computation_stack;
    
    std::string function_name_buffer;
    std::size_t nj, mj;
    double tmp1, tmp2;
    bool done;

    void print_stacks() {
      std::cout << "  argument stack:  ";
      for (auto x: argument_stack)
	std::cout << x << "  ";
      std::cout << std::endl << "  computation stack:  ";
      for (auto x: computation_stack)
	std::cout << x << "  ";
      std::cout << std::endl;
    }

    void reset() {
      call_stack.clear();
      instruction_pointers.clear();
    
      argument_stack.clear();
      computation_stack.clear();
    
      done = false;
    }

    std::size_t current_instruction() {
      return static_cast<std::size_t>(*instruction_pointers.back());
    }
  
    void increment_ptr(std::size_t n) {
      instruction_pointers.back() += n;
    }


    void call_builtin(std::size_t id) {
      (this->*builtins[id])();
    }
  
    void process_symbol(const std::map<std::string, std::vector<double> >& context) {
      mj = *(instruction_pointers.back() + 1);
      nj = *(instruction_pointers.back() + 2);
      std::copy(instruction_pointers.back() + 3,
		instruction_pointers.back() + 3 + 4,
		reinterpret_cast<double*>(&function_name_buffer[0]));

    
      if (mj == 0) {  // push an arg on the cstack
	computation_stack.push_back(argument_stack[argument_stack.size() - 1 - nj]);
      } else if (mj == -1) {
	call_builtin(nj);
      } else {  // call a user function
	auto user_f(context.find(trimmed(function_name_buffer)));
      
	if (user_f == context.end())
	  throw std::string("alucell_expression::eval : function not found.");

	call_stack.push_back(user_f->second);
	instruction_pointers.push_back(&call_stack.back()[0]);
      }
    }
  
    void prepare_arguments() {
      for (std::size_t i(0); i < nj; ++i) {
	double value(computation_stack.back());
	computation_stack.pop_back();
      
	argument_stack.push_back(value);
      }
      argument_stack.push_back(nj);
    }
  
    void return_from_call() {
      std::size_t arg_number(argument_stack.back());
      argument_stack.pop_back();

      for (std::size_t i(0); i < arg_number; ++i)
	argument_stack.pop_back();

      call_stack.pop_back();
      instruction_pointers.pop_back();

      if (call_stack.size() == 0)
	done = true;
      else
	increment_ptr(1);
    }

    template<typename operation>
    void binary_op() {
      const double tmp1(computation_stack.back());
      computation_stack.pop_back();
      const double tmp2(computation_stack.back());
      computation_stack.pop_back();

      computation_stack.push_back(operation()(tmp1, tmp2));
    }

    template<typename operation>
    void unary_op() {
      computation_stack.back() = operation()(computation_stack.back());
    }
  };

  stack_machine::stack_machine()
    : builtins(),
      call_stack(), instruction_pointers(),
      computation_stack(), argument_stack(),
      function_name_buffer(32, ' '),
      nj(0), mj(0), tmp1(0.0), tmp2(0.0),
      done(false) {
    builtins[1] = &stack_machine::unary_op<operators::unary_function_wrapper<std::sin> >;
    builtins[2] = &stack_machine::unary_op<operators::unary_function_wrapper<std::cos> >;
    builtins[3] = &stack_machine::unary_op<operators::unary_function_wrapper<std::tan> >;
  
    builtins[4] = &stack_machine::unary_op<operators::unary_function_wrapper<std::asin> >;
    builtins[5] = &stack_machine::unary_op<operators::unary_function_wrapper<std::acos> >;
    builtins[6] = &stack_machine::unary_op<operators::unary_function_wrapper<std::atan> >;

    builtins[7] = &stack_machine::unary_op<operators::unary_function_wrapper<std::sqrt> >;
    builtins[8] = &stack_machine::unary_op<operators::unary_function_wrapper<std::exp> >;
    builtins[9] = &stack_machine::unary_op<operators::unary_function_wrapper<std::log> >;

    builtins[10] = &stack_machine::binary_op<operators::min>;
    builtins[11] = &stack_machine::binary_op<operators::max>;

    builtins[12] = &stack_machine::binary_op<std::equal_to<double> >;
    builtins[13] = &stack_machine::binary_op<std::greater<double> >;
    builtins[14] = &stack_machine::binary_op<std::greater_equal<double> >;
    builtins[15] = &stack_machine::binary_op<std::less<double> >;
    builtins[16] = &stack_machine::binary_op<std::less_equal<double> >;

    builtins[17] = &stack_machine::unary_op<operators::rand>;

    builtins[18] = &stack_machine::unary_op<operators::unary_function_wrapper<std::sinh> >;
    builtins[19] = &stack_machine::unary_op<operators::unary_function_wrapper<std::cosh> >;
    builtins[20] = &stack_machine::unary_op<operators::unary_function_wrapper<std::tanh> >;

    builtins[21] = &stack_machine::unary_op<operators::integer>;
    builtins[22] = &stack_machine::binary_op<operators::integer2>;
    builtins[23] = &stack_machine::unary_op<operators::sqr>;
    builtins[24] = &stack_machine::unary_op<operators::cpu_time>;
    builtins[25] = &stack_machine::binary_op<operators::div_eucl>;
    builtins[26] = &stack_machine::binary_op<std::modulus<long int> >;
    builtins[27] = &stack_machine::unary_op<operators::nexteven>;
    builtins[28] = &stack_machine::binary_op<operators::binary_function_wrapper<std::pow> >;
    builtins[29] = &stack_machine::unary_op<operators::inv>;
  
    builtins[30] = &stack_machine::binary_op<std::plus<double> >;
    builtins[31] = &stack_machine::binary_op<std::minus<double> >;
    builtins[32] = &stack_machine::binary_op<std::multiplies<double> >;
    builtins[33] = &stack_machine::binary_op<std::divides<double> >;
    builtins[34] = &stack_machine::binary_op<operators::binary_function_wrapper<std::pow> >;
    builtins[35] = &stack_machine::unary_op<operators::negate>;
  }


  class expression_decoder {
  public:
    explicit expression_decoder(variable::expression* expr)
      : input_rank(0),
        output_rank(0),
        bytecode(),
        pretty_format(32 * sizeof(double), ' '),
        builtin_names() {
      std::vector<double> buffer(expr->get_length() / sizeof(double), 0);
      expr->get_data(&buffer[0]);
    
      output_rank = buffer[0];
      input_rank = buffer[1];

      std::size_t bytecode_length(buffer[2]);
      bytecode.resize(bytecode_length, 0);
      std::copy(&buffer[0] + 3, &buffer[0] + 3 + bytecode_length, &bytecode[0]);

      std::copy(&buffer[0] + 3 + bytecode_length,
		&buffer[0] + 3 + bytecode_length + 32,
		reinterpret_cast<double*>(&pretty_format[0]));

      builtin_names[1] = "sin";
      builtin_names[2] = "cos";
      builtin_names[3] = "tan";
      builtin_names[4] = "asin";
      builtin_names[5] = "acos";
      builtin_names[6] = "atan";
      builtin_names[7] = "sqrt";
      builtin_names[8] = "exp";
      builtin_names[9] = "log";
      builtin_names[10] = "min";
      builtin_names[11] = "max";
      builtin_names[12] = "eq";
      builtin_names[13] = "gt";
      builtin_names[14] = "ge";
      builtin_names[15] = "lt";
      builtin_names[16] = "le";
      builtin_names[17] = "rand";
      builtin_names[18] = "sinh";
      builtin_names[19] = "cosh";
      builtin_names[20] = "tanh";
      builtin_names[21] = "int";
      builtin_names[22] = "?";
      builtin_names[23] = "sqr";
      builtin_names[24] = "TIMER";
      builtin_names[25] = "idiv";
      builtin_names[26] = "imod";
      builtin_names[27] = "nexteven";
      builtin_names[28] = "pow";
      builtin_names[29] = "inv";
    }

  
    //void print_bytecode(std::ostream&);
    std::string get_human_readable() const { return pretty_format; }
  
    std::size_t get_input_rank() const { return input_rank; }
    std::size_t get_output_rank() const { return output_rank; }

    void dump_bytecode_assembly(std::ostream& stream) {

      std::string function_name_buffer(32, ' ');
      std::size_t nj(0), mj(0), icode(0);
      double value(0.0);
    
      double* instruction(&bytecode[0]);
      while (instruction < &bytecode[0] + bytecode.size()) {
	switch (static_cast<std::size_t>(*instruction)) {
        case 0:
          //stream << "nop" << std::endl;
          break;
        case 100:
          stream << "prep_arg" << std::endl;
          break;
        case 200:
          stream << "eol" << std::endl;
          break;
        case 300:
          mj = *(instruction + 1);
          nj = *(instruction + 2);
	  icode = *(instruction + 3);
          std::copy(instruction + 4,
                    instruction + 4 + 4,
                    reinterpret_cast<double*>(&function_name_buffer[0]));
          if (mj == 0) {
            stream << "push_arg <" << nj << ">" << std::endl;
          } else if (mj == -1) {
            stream << "call_builtin <" << nj << "(" << builtin_names[nj] << ")>" << std::endl;
          } else {
            stream << "call_function <" << trimmed(function_name_buffer) << ">"
		   << " with " << nj << " arguments" << std::endl;
          }
          instruction += 6;
          break;
        case 400:
          stream << "push_real" << " <" << *(instruction + 1) << ">" << std::endl;
          instruction += 1;
          break;
        case 1:
          stream << "add" << std::endl;
          break;
        case 2:
          stream << "sub" << std::endl;
          break;
        case 3:
          stream << "mult" << std::endl;
          break;
        case 4:
          stream << "div" << std::endl;
          break;
        case 5:
          stream << "pow" << std::endl;
          break;
        case 6:
          stream << "neg" << std::endl;
          break;
        default:
          stream << "unknown" << std::endl;
	}
	++instruction;
      }
    }
  
    std::vector<double> eval(const std::vector<double>& arguments,
			     const std::map<std::string, std::vector<double> >& context) {
      stack_machine m;
      return m.run(arguments, bytecode, context);
    }

  private:
    std::size_t input_rank, output_rank;
    std::vector<double> bytecode;
    std::string pretty_format;
    std::map<std::size_t, std::string> builtin_names;
  };

}

#endif /* _ALUCELL_LEGACY_VARIABLE_H_ */
