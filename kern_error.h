#ifndef kern_error
#define kern_error


#include <system_error>
#include <mach/kern_return.h>

std::error_category &kern_category();

/* kern_return_t is typedef'd to an int so it's not a unique type */
#if 0
namespace std {

	template<>
	struct is_error_code_enum<kern_return_t> : public true_type {};

	template<>
	struct is_error_condition_enum<kern_return_t> : public true_type {};

}

inline std::error_condition make_error_condition(kern_return_t e) {
	return std::error_condition(e, kern_category());
}


inline std::error_code make_error_code(kern_return_t e) {
	return std::error_condition(e, kern_category());
}
#endif

inline std::error_code make_kern_error_code(kern_return_t e) {
  return std::error_code(e, kern_category());
}

#endif
