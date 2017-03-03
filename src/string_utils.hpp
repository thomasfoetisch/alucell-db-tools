#ifndef _STRING_UTILS_H_
#define _STRING_UTILS_H_

#include <string>
#include <algorithm>

inline void trim_right(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
		       std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
	  s.end());
}

inline std::string right_trimmed(std::string s) {
  trim_right(s);
  return s;
}

inline void trim_left(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(),
				  std::not1(std::ptr_fun<int, int>(std::isspace))));
}

inline std::string left_trimmed(std::string s) {
  trim_left(s);
  return s;
}

inline void trim(std::string& s) {
  trim_right(s);
  trim_left(s);
}

inline std::string trimmed(std::string s) {
  trim(s);
  return s;
}

inline bool suffixed(const std::string& str, const std::string& suffix) {
  return str.rfind(suffix) == str.size() - suffix.size();
}

inline bool prefixed(const std::string& str, const std::string& prefix) {
  return str.find(prefix) == 0;
}


#endif /* _STRING_UTILS_H_ */
