#ifdef FUNCTION_CNCPT_HPP
#	ifndef FM_FUNCTXNL_HPP
#	define FM_FUNCTXNL_HPP

template<typename derived, typename signature, function_prototype proto_type= signature>
//requires function_prototype<signature>
class function_crtp_base
{ static_assert(std::is_same_v<signature, proto_type>,"generic implementaion must fail."); };//declare defaults

template<typename derived, function_prototype signature, typename result_type, typename ... args_t>
class function_crtp_base <derived, result_type(args_t ...), signature> {
	bool constexpr static is_const	= std::is_const_v<std::remove_reference_t<derived>>;
	bool constexpr static is_rvalue = std::is_rvalue_reference_v<derived>;
	bool constexpr static is_lvalue = std::is_lvalue_reference_v<derived>;
	bool constexpr static is_value	= !(is_lvalue || is_rvalue);
public:
	typedef function_crtp_base function_crtp;
	typedef result_type return_type;
	typedef signature	proto_type;
	typedef std::tuple<args_t...>	argument_tuple;
	template<std::size_t idx>
	using argument_type = std::tuple_element_t<idx, argument_tuple>;
	std::size_t constexpr static argument_count = sizeof...(args_t);

	typedef return_type(*forward)(args_t&& ...);

	typedef return_type(*invoke_void_type)(void*, args_t&& ...);

	template<typename function_object>
	using invoke_object_type= return_type(*)(function_object &&, args_t&& ...);

	decltype(auto) operator()(args_t ... args)
		requires(is_value && !is_const)
	{	return static_cast<derived&>(*this).do_invoke(std::forward<decltype(args)>(args)...);	};// !operator() 

	decltype(auto) operator()(args_t ... args) const
		requires(is_value && is_const)
	{	return static_cast<derived const&>(*this).do_invoke(std::forward<decltype(args)>(args)...);	};// !operator() const

	decltype(auto) operator()(args_t ... args)&
		requires(is_lvalue && !is_const)
	{	return static_cast<derived>(*this).do_invoke(std::forward<decltype(args)>(args)...);	};// !operator() &

	decltype(auto) operator()(args_t ... args) const&
		requires(is_lvalue&& is_const)
	{	return static_cast<derived>(*this).do_invoke(std::forward<decltype(args)>(args)...);	};// !operator() const&

	decltype(auto) operator()(args_t ... args) &&
		requires(is_rvalue && !is_const)
	{	return static_cast<derived>(*this).do_invoke(std::forward<decltype(args)>(args)...);	};// !operator() &&

	decltype(auto) operator()(args_t ... args) const&&
		requires(is_rvalue&& is_const)
	{	return static_cast<derived>(*this).do_invoke(std::forward<decltype(args)>(args)...);	};// !operator() const&&

protected:
	static constexpr auto nullfn = [](args_t ...) -> return_type {
		if constexpr (std::is_void_v<return_type>)
			return;
		else if constexpr (std::is_default_constructible_v<return_type>)
			return return_type{};
	};

	struct proto {
		return_type operator()(args_t ...) const;
	};
}; // !function_crtp_base

namespace detail {
	template<typename, function_prototype>
	struct is_function_crtp_base: std::false_type {};

	template<typename crtp, function_prototype signature>
	struct is_function_crtp_base<function_crtp_base<crtp, signature>, signature>: std::true_type {};

	template<typename fn_type, function_prototype signature, typename crtp_base= typename fn_type::function_crtp>
	bool constexpr static derived_from_function_crtp_base =	is_function_crtp_base<crtp_base, signature>::value &&
															std::is_base_of_v<crtp_base,fn_type>;
}; // !detail

template<typename F, typename T>
decltype(auto) apply(F&& f, T&& t) {
	return [&f1=f, &t1=t] <std::size_t ... idx>
		//requires std::is_invocable_v<F, std::tuple_element_t<idx,T> ...> 
	(std::index_sequence<idx...>) -> decltype(auto) { /*-> std::invoke_result_t<F, std::tuple_element_t<idx, T> ...>*/
		typedef std::remove_cvref_t<F> func_t;
		constexpr static auto size = sizeof...(idx);
		typedef std::tuple_element_t<0, T> first_t;

		if constexpr (std::is_member_pointer_v<func_t>) {
			static_assert(size);

			return [&f1, &t1] <std::size_t ... idx_1>(std::index_sequence<idx_1...>/*, F &&f, T &&t*/)-> decltype(auto)
			/*-> std::invoke_result_t<F, std::tuple_element_t<idx, T> ...>*/ {
				constexpr static auto size = sizeof...(idx_1);
				auto arg_0 = [&a0 = get<0>(t1)]()-> auto&& {
					typedef std::remove_cvref_t<first_t> raw_first_t;
					if constexpr (std::is_same_v<decltype(std::ref(a0)), raw_first_t>)
						//reference wrapper -> return the infered refrence
						return a0.get();
					else if constexpr ([] ()constexpr {
						if constexpr (size)
							return requires(first_t ptr) {
								{*ptr};
								std::is_invocable_v<F, decltype(*ptr), typename std::tuple_element_t<idx_1 + 1, T> ...>;
						};
						else
							return requires(first_t ptr) {
								{*ptr};
								std::is_invocable_v<F, decltype(*ptr)>;
						};
					}())//if is [smart] pointer -> return the derefrenced object
						return *(std::forward<first_t>(a0));
					else // return the original object
						return std::forward<first_t>(a0);

				}; // !arg_0

				if constexpr (std::is_member_object_pointer_v<func_t>) {
					static_assert(!size);
					return (arg_0().*f1);
				}
				else if constexpr (size)
					return (arg_0().*f1)(std::forward<std::tuple_element_t<idx_1 + 1, T>>(get<idx_1 + 1>(std::forward<T>(t1))) ...);
				else
					return (arg_0().*f1)();
			}(std::make_index_sequence<std::tuple_size_v<T> - 1>{}/*, std::forward<F>(f), std::forward<T>(t)*/); // !return []<idx_1...>(rest)
		} else if constexpr (size)
			return (std::forward<F>(f1))(std::forward<std::tuple_element_t<idx, T>>(get<idx>(std::forward<T>(t1))) ...);
		else
			return (std::forward<F>(f1))();
		//else if constexpr (std::is_function < std::remove_pointer_t<func_t> >)
		//	return f(std::forward<std::tuple_element_t<idx, T>>(std::forward<T>(t)) ...);
	}(std::make_index_sequence<std::tuple_size_v<T>>{}); // !apply.return []<idx...>(args)
}; // !apply

template<typename F, typename ... ARGS>
	requires (std::is_invocable_v<F, ARGS ...>) // && std::is_const_v<std::invoke_result_t<F, ARGS ...>>)
decltype(auto) invoke(F&& f, ARGS&& ... args)
{
	typedef std::remove_cvref_t<F> func_t;
	constexpr static auto size = sizeof...(args);

	if constexpr (std::is_member_pointer_v<func_t>)
		return lib_fm::apply(std::forward<F>(f), std::forward_as_tuple(std::forward<ARGS>(args)...));
	else if constexpr (size)
		return (std::forward<F>(f))(std::forward<ARGS>(args)...);
	else
		return (std::forward<F>(f))();
};

#	endif // !FM_FUNCTXNL_HPP
#endif // !FUNCTION_CNCPT_HPP
