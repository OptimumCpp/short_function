#ifdef ZBIND_HPP

namespace detail { //dirty book: contains the (nasty) cheats
	namespace bct = boost::callable_traits;

	template< typename fn_type, fn_type fn>
	struct zero_bind
	:	lib_fm::function_crtp_base<zero_bind<fn_type, fn> const,bct::function_type_t<fn_type>> {
		using base_type = lib_fm::function_crtp_base<zero_bind<fn_type, fn> const, bct::function_type_t<fn_type>>;
		using typename base_type::proto_type;
		using typename base_type::return_type;
		using typename base_type::argument_tuple;
		using base_type::argument_count;
		template<std::size_t N>
		using argument_type = typename base_type::template argument_type<N>;

		typedef proto_type *function_ptr;
		typedef fn_type	target_type;
		target_type static constexpr target_function = fn;

	private:
		friend class base_type::function_crtp_base;
		decltype(auto) static do_invoke(auto&& ... args) { return lib_fm::invoke(std::forward<target_type>(fn), std::forward<decltype(args)>(args)...); };

	public:

		constexpr operator function_ptr()const noexcept {
			if constexpr (std::is_same_v<target_type, function_ptr>)
				return target_function;
			else 
				return [] (auto...args) { return do_invoke(std::forward<decltype(args)>(args)...);	};
		};
	}; // !zero_bind<fn_type, fn>

	template<typename fn_type, fn_type fn, std::size_t shift, typename ... bound, std::size_t ... idx>
	auto middle_bound(std::index_sequence<idx...>, bound&& ... b) {
		return[... b = b]
		(std::tuple_element_t<idx + shift, bct::args_t<fn_type>>&&... c) mutable->bct::return_type_t<fn_type> {
			if constexpr ( shift )
				return lib_fm::invoke(std::forward<fn_type>(fn), std::forward<bound>(b)..., std::forward<decltype(c)>(c)...);
			else
				return lib_fm::invoke(std::forward<fn_type>(fn), std::forward<decltype(c)>(c)..., std::forward<bound>(b)...);
		};// lambda[b...]
	}; // !middle_bound

	template<typename fn_type, fn_type fn, typename ... bound>
	//requires(std::is_member_pointer_v<fn_type> || is_function_pointer_v<fn_type>) 
	auto constexpr zbind_front_v(bound&& ... b)
		//requires(std::is_member_pointer_v<fn_type> || is_function_pointer_v<fn_type>) 
	{
		auto constexpr shift = sizeof...(b);
		auto constexpr size = std::tuple_size_v< bct::args_t< fn_type > > -shift;
		return middle_bound<fn_type, fn, shift>
			(std::make_index_sequence<size>{}, std::forward<bound>(b)...);
	};//ultimate tool: used for overloaded functions

	template<typename fn_type, fn_type fn, typename ... bound>
	//requires(std::is_member_pointer_v<fn_type> || is_function_pointer_v<fn_type>) 
	auto constexpr zbind_back_v(bound&& ... b)
		//requires(std::is_member_pointer_v<fn_type> || is_function_pointer_v<fn_type>) 
	{
		auto constexpr size = std::tuple_size_v< bct::args_t< fn_type > > -sizeof...(b);
		return middle_bound<fn_type, fn, 0>
			(std::make_index_sequence<size>{}, std::forward<bound>(b)...);
	};//ultimate tool: used for overloaded functions

	template<typename>
	bool constexpr is_zbind = false;

	template<typename fn_type, fn_type fn>
	bool constexpr is_zbind<zero_bind<fn_type, fn>> = true;

	template<typename fn_type>
	concept zero_cost_binding_def =
		is_zbind<fn_type> ||
		(trivial_callable<fn_type, typename fn_type::proto_type> &&
		 is_zbind<typename fn_type::base_type> &&
		 std::is_base_of_v<typename fn_type::base_type, fn_type>);
}; // !details

#endif // ZBIND_HPP
