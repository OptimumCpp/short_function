#ifndef ZBIND_HPP
#define ZBIND_HPP
//! @file monostate_from.hpp
//! @brief a header only library to optimize memory overhead of callback-bound code.
//! 
//! size of member pointers is generally larger than free function pointers and void pointers. I t is also affected by the class
//! hierarchy model of the parent, and varies on a per-class basis. This has an adverse effect on small-value optimization in
//! callback bound code (using functional, signals2, asio...).
//! The C++17 type-deduced none-type template parameter(<auto param>) helps make every member pointer or function pointer into a
//! zero-sized callable object, just like a no-capture lambda. The same effect can be achieved using a generic lambda, 
//! but this library provides a means to avoid generic call syntax as well as repeating the parameter list. this library relies on
//! boost/callable_traits as an enabler boiler plate and C++20 for correct compilation.
//! 
//! Example:
//! @snippet ./monostate_from.cpp classdef
//! Next we bind some functions:
//! @snippet ./monostate_from.cpp bindings


#include "functional.hpp"

namespace lib_fm {
#	include "detail/d_monostate_from.hpp"

	//! @brief concept zero_cost_binding
	//! @brief identifies any instance of 'make_monostate_from'
	//! succeeds iff the parameter type is a 'make_monostate_from', fails otherwise.
	//! @tparam fn_type the candidate type to be tested as a 'make_monostate_from'
	template<typename fn_type>
	concept zero_cost_binding = detail::zero_cost_binding_def<std::remove_cvref_t<fn_type>>;

	//! @defgroup monostate_from-class
	//! The ***zbind_t*** class provides the basis for template variables ***monostate_from<fn>*** and ***zbind_v<type,fn>*** to remove the memory
	//! footprint of their operand function/member pointer.
	//! @{
	//! @struct zbind_t 
	//! @brief an empty type(std::is_empty_t==true) who converts member pointers and function pointers to a function object.
	//! requires its input to be of pointer to member or pointer to function types. Converts the input to an empty callable -
	//! suitable for small value optimization in callback oriented libraries.  
	//! @tparam fn_type the function or member pointer type to be converted
	//! @tparam fn the function or member address to be converted
	template<typename fn_type, fn_type fn>
		requires is_function_pointer<fn_type> || std::is_member_function_pointer_v<fn_type> //std::is_member_pointer_v<fn_type>
	struct make_monostate_from_overload //cleans the mess up, concludes the final interface
		: detail::zero_bind<fn_type, fn>
	{
		static_assert(fn,"'nullptr' is not acceptable.\n You may want to default construct 'short_function'");
	public:
		using base_type = detail::zero_bind<fn_type, fn>;
		using typename base_type::function_ptr;

	//cheating on doxygen to document an inherited function:(if the condition is true, compilation fails) 
	#if(__cplusplus<199711L)
		constexpr result operator()(object &&, args&&...);
	#endif // INHERITED_FUNCTION
		using base_type::operator();

		constexpr auto operator==(make_monostate_from_overload const) const noexcept { return true; };
		constexpr auto operator!=(make_monostate_from_overload const) const noexcept { return false; };

		//! @fn operator function_ptr()
		//! @brief ***monostate_from*** is implicitly castable to function_ptr
		//! @return address of
		//! the input function for free/static/none-instance functions.
		//! an equivalent free-function with forwarded parameters, for instance-member methods
		using base_type::operator function_ptr;
	};
	
	template<auto fn>
	using make_monostate_from = make_monostate_from_overload<decltype(fn), fn>;

//! @fn result zbind_t::operator()(ref qualified Class&& object, types &&...args)
//! @brief unique none-template call operator for ***monostate_from***.
//! forwards arguments to original function
//! @param ...args proper forwarding references to original function parameters.
//! @param object First argument will be a reference to parent class object, iff the ***fn*** is a member pointer.
//! @return rvo+nrvo the result of original function

	//! @brief a template variable to disambiguate overloaded functions when using ***monostate_from*** with out explicitly casting.
	//! @tparam fn_type signature of the funtion pointer
	//! @tparam fn name of the overload set
	template<typename fn_type, fn_type fn>
	make_monostate_from_overload<fn_type, fn> static constexpr monostate_from_overload; //ultimate tool: used for overloaded functions

	//! @brief a zero-sized template variable equivalent to member/function pointer 
	//! @tparam fn member/function pointer to be optimized for callback libraries
	template<auto fn>
	make_monostate_from<fn> static constexpr monostate_from;
	//ultimate tool: simplified for single overload sets

	//! @brief same as ***zbind_front***, but with explicit overload selection parameter for overloaded functions.
	//! @tparam fn_type pointer to the intended overload  of the function
	//! @tparam fn the function name
	//! @tparam ...bound deduced types of parameters to bind in the front
	//! @param ...b parameters to bind in the front
	//! @return a callable bound to specified parameters
	template<typename fn_type, fn_type fn, typename ... bound>
		requires(std::is_member_pointer_v<fn_type> || is_function_pointer<fn_type>)
	auto constexpr zbind_front_v(bound&& ... b) { 
		return detail::zbind_front_v<fn_type, fn>(std::forward<bound>(b)...);
	};//ultimate tool: used for overloaded functions

	//! @brief same as ***zbind_back***, but with explicit overload selection parameter for overloaded functions.
	//! @tparam fn_type pointer to the intended overload  of the function
	//! @tparam fn the function name
	//! @tparam ...bound deduced types of parameters to bind in the back
	//! @param ...b parameters to bind in the back
	//! @return a callable bound to specified parameters
	template<typename fn_type, fn_type fn, typename ... bound>
		requires(std::is_member_pointer_v<fn_type> || is_function_pointer<fn_type>)
	auto constexpr static zbind_back_v(bound&& ... b) {
		return detail::zbind_back_v<fn_type, fn>(std::forward<bound>(b)...);
	};//ultimate tool: used for overloaded functions

	//! @brief a counterpart **for std::bind_front** accepting member/function pointers as none-type template argument
	//! @tparam fn the function name
	//! @tparam ...bound deduced types of parameters to bind in the front
	//! @param ...b parameters to bind in the front
	//! @return a callable bound to specified parameters
	template<auto fn, typename ... bound>
	auto constexpr static zbind_front(bound&& ... b) {
		return detail::zbind_front_v<decltype(fn), fn>(std::forward<bound>(b)...);
	};//ultimate tool: simplified for single overload sets

	//! @brief a counterpart **for std::bind_back** accepting member/function pointers as none-type template argument
	//! @tparam fn the function name
	//! @tparam ...bound deduced types of parameters to bind in the back
	//! @param ...b parameters to bind in the back
	//! @return a callable bound to specified parameters
	template<auto fn, typename ... bound>
	auto constexpr static zbind_back(bound&& ... b) {
		return detail::zbind_back_v<decltype(fn), fn>(std::forward<bound>(b)...);
	};//ultimate tool: simplified for single overload sets

	//! @}

}; // !lib_fm

#endif // !ZBIND_HPP
