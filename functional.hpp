#ifndef FUNCTION_CNCPT_HPP
#define FUNCTION_CNCPT_HPP
#include <tuple>
#include <type_traits>
#include <boost/callable_traits.hpp>

namespace lib_fm {
	namespace bct = boost::callable_traits;

	template<typename type>
	concept trivial_empty = std::is_empty_v<type> && std::is_trivial_v<type>;

	template<typename signature>
	concept function_prototype = std::is_function_v<signature>;

	template<typename type>
	concept is_function_pointer = std::conjunction_v < std::is_pointer<type>, std::is_function<std::remove_pointer_t<type>>>;

	template<typename fn_type>
	concept primitive_callable = (std::is_member_pointer_v<fn_type> || is_function_pointer<fn_type>);

#	include "function.utility.hpp"

	template<typename fn_type, typename signature>
	concept callable = function_prototype<signature> &&
		std::is_invocable_r_v<bct::return_type_t<signature>, decltype(lib_fm::apply<fn_type, bct::args_t<signature>>), fn_type&&, bct::args_t<signature>&&>;

	template<typename fn_type, typename signature>
	concept matched_callable = function_prototype<signature> && 
		(std::is_same_v<bct::function_type_t<fn_type>, signature>	||
		 detail::derived_from_function_crtp_base<fn_type, signature>);

	//template<typename arg, typename fn_type, std::size_t idx>
	//concept argument = std::is_same_v<arg, std::tuple_element_t<idx, bct::args_t<fn_type>>>;

	template<typename fn_type, typename signature>
	concept trivial_callable = function_prototype<signature> && trivial_empty<fn_type> && callable<fn_type, signature>;

	template<typename fn_type, typename signature>
	concept matched_trivial_callable = matched_callable<fn_type, signature> && trivial_empty<fn_type>;

}; // !lib_fm


#endif // !FUNCTION_CNCPT_HPP
