#ifndef SHORT_FUNCTION_HPP
#define SHORT_FUNCTION_HPP
//! @file short_function.hpp
//! @brief a header only library to type-erase empty trivial function objects.
//! 
//! this file defines the 'short_function' template class as a function type capable of holding pointers to free functions,
//! as well as pointers to member methods, no-capture lambdas and any empty trivial function object type. It can provide a
//! bridge between functional progarmming and old C-API expecting free function pointers.
//! 
//! Example:
//! @snippet ./monostate_from.cpp classdef
//! Next we try some short functions:
//! @snippet ./monostate_from.cpp short_func

#include <string>
#include <optional>
#include "monostate_from.hpp"

namespace lib_fm {

	//! @brief determines the underlying category of a *short_function*
	enum class short_function_source { 
		//! @brief when a short function is default constructed
		none,
		//! @brief when a short function is evaluated to a 'monostate_from' on a static function
		free_function,
		//! @brief when a short function is evaluated to a 'monostate_from' on a member function
		instance_method,
		//! @brief when a short function is evaluated to a 'monostate_from' on a pointer to data member
		//! *data_field* is currently unusable as a *monostate_from** because of the way *boost::callable_traits* is defined
		data_field,
		
		/*lambda,*/

		//! @brief when a short function holds an instance of a trivial empty function object
		empty_object
	}; // !short_function_source

	constexpr auto to_string_view(short_function_source src) {
		std::string_view constexpr names[] = { "default function", "free function", "instance method", "data field", "empty object", "" };
		auto const i = static_cast<std::size_t>(src);
		return names[std::min(i,std::size(names)-1)];
	};

#	include "detail/d_short_function.hpp"

	//! @brief a type erasure on trivial empty function objects
	//! can hold a refrence to the type of a 'lambda', 'monostate_from' or a UD empty function object
	//! @tparam signature the commont function prototype of all the functions that it can hold 
	//! @requires the 'signature' to satisfy 'std::is_function'
	template<function_prototype signature/*, bool denied= true*/>
	struct short_function:
		detail::short_function_def<short_function<signature>,signature>
	{
		typedef detail::short_function_def<short_function, signature>	base_type;
		using typename base_type::proto_type;
		using typename base_type::return_type;
		using typename base_type::argument_tuple;
		using base_type::argument_count;

		typedef signature	signature_type;
		typedef std::add_pointer_t<signature> free_pointer;
		using typename base_type::short_function_command;
		using base_type::is_nullable;

		//! @brief default constructor. only available if return type is default constructible
		constexpr short_function() noexcept
			requires(is_nullable) :
			short_function { base_type::nullfn } {};

		//! @brief calls the underlying type erased function
		//! @param args... same parameters as the signature
		//! @return a default constructed object of return type, if the object is null and return type is not void;
		//! result of the underlying object otherwise.
		using base_type::operator();

		//! @brief initializing constructor. Accepts trivial functions of correct signature
		//! iff the signature of **fn_type** is the same as *short_function*, this constructor is implicit; explicit, if **fn_type**
		//! is of a covariant signature(with convertible return type and constructible inputs).
		//! @tparam fn_type the trivial empty function type to erase
		//! @param fn the instance of empty function type as input  
		template<trivial_callable<signature> fn_type>
		constexpr explicit(/*denied &&*/ !matched_callable<fn_type, signature>)
			short_function(fn_type fn) noexcept:
			manager { make_manager(fn) } {};
		
		//! @brief convert between **restrictive/permissive** twin types. explicit, if result is **restrictive**
		//constexpr explicit(denied)
		//	short_function(short_function<signature, !denied> oth) noexcept:
		//	manager { oth.manager } {};

		//! @defgroup null-check
		//! @brief check if the object is empty.
		//! if return type is not void nor default constructible, the object can not become null. Otherwise, iff the object was default
		//! constructed or assigned from a default constructed instance of *short_function*, it is considered empty. 
		//! @{
		//! @brief checks if the object is empty
		//! @return **true**, if empty; **false**, otherwise.
		bool constexpr is_empty() const noexcept {
			if constexpr ( is_nullable )
				return source()==short_function_source::none;
			else
				return false;
		}; // !is_empty
		//! @brief  check against null
		//! @return **true**, iff empty; **false**, otherwise. 
		bool constexpr operator!() const noexcept { return is_empty(); };
		//! @brief covertion to bool
		//! @return **true**, iff not empty; **false**, otherwise. 
		constexpr explicit operator bool() const { return !is_empty(); };
		//! @}


		//! @defgroup get-function-pointer
		//! @brief retrive a free function pointer to the underlying callable 
		//! provides a mechanism to use **short_function** as a valid input to old C call back API. regardless of the underlying
		//! type-erased function, a free function pointer representation exists for all trivial empty function objects
		//! @{
		auto constexpr to_ptr() const noexcept { command_query<short_function_command::callable>(); };
		constexpr explicit operator free_pointer() const { return to_ptr(); };
		//! @}

		//! @brief meta data about the undelying  type-erased function.
		//! retrives the category of the literal value used to create this object. 
		//! @return an insatance of *short_function_source*
		auto constexpr source() const noexcept { return command_query<short_function_command::source>(); };

		//! @brief checks if this object is calling a covariant function type or one with an exactly matched signature.
		//! @return **true** iff the official parameter set and return type match; **false** otherwise.
		bool constexpr is_converted() const noexcept { return !(command_query<short_function_command::exact_match>()); };

		friend bool operator==(short_function const &, short_function const &) noexcept = default;
		friend bool operator!=(short_function const &, short_function const &) noexcept = default;

		//! @brief type un-erasure. tries to retrive to the original empty function object or function/member pointer.
		//! this function can retrive an empty function object; or a function/member pointer in case the stored function was a **monostate_from**
		//! @tparam fn_type a function type with a covariant signatue or an exact set of parameters and return type as this object.
		//! @return an **std::optionl<fn_type>** to the *fn_type* instance, iff types match; a null **std::optional**, otherwise.
		template<callable<signature> fn_type>
		std::optional<fn_type> target() const noexcept {
			if ( *this ) {
				if constexpr ( trivial_callable <fn_type, signature> ) {	// return function object:
					fn_type fn {};
					if ( manager == make_manager(std::forward<fn_type>(fn)) )
						return fn;
				} else if constexpr ( primitive_callable<fn_type> )	// going for monostate_from test
					if ( matched_callable<fn_type, signature> != is_converted() ) {
						using val_fn_type = std::remove_cvref_t<fn_type>;
						auto constexpr pointer_type = std::is_member_object_pointer_v<val_fn_type> ?
								short_function_source::data_field :
							std::is_member_function_pointer_v<val_fn_type> ?
								short_function_source::instance_method :
								short_function_source::free_function;
						if (source() == pointer_type) {					// return function/member pointer:
							auto const [class_id, target_ptr] = command_query<short_function_command::z_target>();
							if ((!class_id) || (class_id==detail::reflect_id<val_fn_type>))
								return { *static_cast<fn_type const *>(target_ptr) };
						};
					}; // !primitive_callable<fn_type>
			}; // !*this
			// default:
			// return null:
			return { std::nullopt };
		}; // !target<fn_type>

	private:
		using typename base_type::manager_type;
		using base_type::make_manager;
		using base_type::command_query;

		friend class base_type::function_crtp_base; // uses:

		return_type do_invoke(auto&&...args) const
		{	return this -> template command_query<short_function_command::callable>()(std::forward<decltype(args)>(args)...);	};

		template <short_function_command command>
		auto constexpr command_query() const noexcept { return this->template command_query<command>(manager); };

		constexpr auto make_manager(trivial_callable<signature> auto fn) const noexcept { return make_manager(fn, manager); };

		manager_type manager;
	}; // !short_function 
}; // !lib_fm

#endif // !SHORT_FUNCTION_HPP

