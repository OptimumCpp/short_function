#ifdef SHORT_FUNCTION_HPP

//namespace lib_fm {
	namespace detail {

		template<typename test>
			requires std::is_class_v<test>
		static constexpr void reflect(){};

		template<typename test>
		static constexpr void(*reflect_id)() = [] ()constexpr {
			if constexpr (std::is_member_function_pointer_v<test>)
				return &reflect<bct::class_of_t<test>>;
			else
				return nullptr;
		}(); 

		template<typename A, typename...choice>
		struct smallest {
			auto static test() {
				if constexpr ( !!sizeof...(choice) )
					if constexpr ( (false || ... || (sizeof(A) > sizeof(choice))) )
						return smallest<choice...>::test();
				return std::type_identity<A>{};
			};
			typedef typename decltype(test())::type type;
		};

		template< typename derived, function_prototype signature 
			, typename base_type = function_crtp_base<derived const, signature>
		>
		struct short_function_def:
			base_type {
		public:
			using typename base_type::return_type;
			using typename base_type::argument_tuple;
			using  base_type::template argument_type;
			using base_type::argument_count;
			using base_type::operator();

		protected:
			using base_type::nullfn;
			using proto = typename base_type::proto;
			enum class short_function_command { callable, source, exact_match, z_target, long_manager };

			bool static constexpr is_nullable {
				std::is_void_v<return_type> ||
				std::is_default_constructible_v<return_type>// ||
				//std::is_constructible_v<return_type, std::nullptr_t>
			};

			static auto tuple_manager(trivial_callable<signature> auto fn) noexcept {
				typedef decltype(fn) fn_type;

				auto static lambda = [](auto...args)->return_type {
					std::remove_reference_t<fn_type> fn {};
					return lib_fm::invoke(std::forward<fn_type>(fn), std::forward<decltype(args)>(args)...);
				};

				return std::make_tuple(
					[](fn_type fn) constexpr -> std::add_pointer_t<signature> {
						if constexpr ( std::is_constructible_v< std::add_pointer_t<signature>, fn_type> )
							return static_cast<std::add_pointer_t<signature>> (std::forward<fn_type>(fn));
						else {
							return lambda;
						};
					}(fn)
					,
					[]() constexpr -> short_function_source {
						if constexpr ( zero_cost_binding<fn_type> ) {
							return	std::is_member_function_pointer_v<typename fn_type::target_type> ?
										short_function_source::instance_method :
									std::is_member_object_pointer_v<typename fn_type::target_type> ?
										short_function_source::data_field :
										short_function_source::free_function;
							//} else if constexpr ( std::is_lambda_v<fn_type> ) {
							//	result.source = short_function_source::lambda;
						} else if constexpr ( is_nullable )
							if constexpr (
								std::is_same_v<fn_type const &, decltype(nullfn) const &> )
								return short_function_source::none;

						return short_function_source::empty_object;
					}()
					,
					matched_callable<fn_type, signature>
					,
					[]() constexpr -> std::tuple<void (*)(), void const *> {
						if constexpr ( zero_cost_binding<fn_type> )
							return { reflect_id<typename fn_type::target_type>, &fn_type::target_function };
						return { 0, nullptr };
					}()
					,
					static_cast<void *(*)(int)>(nullptr)
				);
			};

			typedef decltype(tuple_manager<proto>({})) tuple_manager_value;
			typedef tuple_manager_value const *tuple_manager_type;

			template <short_function_command command>
			static auto constexpr command_query(tuple_manager_type const mgr) noexcept
			{ return get<static_cast<std::size_t>(command)>(*mgr); };

			static auto make_manager(trivial_callable<signature> auto fn, tuple_manager_type const) noexcept {
				auto static table { tuple_manager(fn) };
				return &table;
			};

			union func_manager_result {
				std::add_pointer_t<signature> callable;
				short_function_source source;
				bool exact_match;
				void const *z_target;
				void *(*long_manager)(int);
			};

			typedef func_manager_result(*func_manager_type)(short_function_command const) noexcept;

			static
				func_manager_type make_manager(trivial_callable<signature> auto fn, func_manager_type const) noexcept {
				typedef decltype(fn) fn_type;

				auto constexpr lambda = [](short_function_command const query) noexcept {
					fn_type fn {};
					auto table = tuple_manager(std::forward<fn_type>(fn));

					func_manager_result result{};

					switch ( query ) {
					case short_function_command::callable:

						result.callable = command_query<short_function_command::callable>(&table);
						return result;

					case short_function_command::source:

						result.source = command_query<short_function_command::source>(&table);
						return result;

					case short_function_command::exact_match:

						result.exact_match = command_query<short_function_command::exact_match>(&table);
						return result;

					case short_function_command::z_target:

						result.z_target = command_query<short_function_command::z_target>(&table);
						return result;

					default:
					case short_function_command::long_manager:

						result.long_manager = command_query<short_function_command::long_manager>(&table);
						return result;

					}; // !make_manager(fn, func_manager_type)
				}; // !lambda
				return lambda;
			};

			template <short_function_command command>
			static auto constexpr command_query(func_manager_type const mgr) noexcept {
				auto result = mgr(command);

				if constexpr ( command == short_function_command::callable )
					return result.callable;

				if constexpr ( command == short_function_command::source )
					return result.source;

				if constexpr ( command == short_function_command::exact_match )
					return result.exact_match;

				if constexpr ( command == short_function_command::z_target )
					return result.z_target;

				if constexpr ( command == short_function_command::long_manager )
					return result.long_manager;
			}; // !command_query(func_manager_type)

			typedef void (*void_func_manager_type)(void);

			static constexpr
				auto make_manager(trivial_callable<signature> auto fn, void const *const mgr) noexcept {
				return	static_cast<void const *>
					(make_manager(std::forward<decltype(fn)>(fn), static_cast<tuple_manager_type>(mgr)));
			};

			template <short_function_command command>
			static auto constexpr command_query(void const *const mgr) noexcept {
				return command_query<command>(static_cast<tuple_manager_type>(mgr));
			};

			static constexpr
				auto make_manager(trivial_callable<signature> auto fn, void_func_manager_type const mgr) noexcept {
				return	reinterpret_cast<void_func_manager_type>
					(make_manager(std::forward<decltype(fn)>(fn), reinterpret_cast<func_manager_type>(mgr)));
			};

			template <short_function_command command>
			static auto constexpr command_query(void_func_manager_type const mgr) noexcept {
				return command_query<command>(reinterpret_cast<func_manager_type>(mgr));
			};

			//could be a struct, but function is more abstract:
			using manager_type = typename smallest<tuple_manager_type, func_manager_type, void const *, void_func_manager_type>::type;
			//using manager_type = tuple_manager_type;
			//using manager_type = func_manager_type;
			//using manager_type = void const *;
			//using manager_type = void_func_manager_type;

		}; // !short_function_crtp

	}; // !detail
//}; // !lib_fm

#endif // SHORT_FUNCTION_HPP
