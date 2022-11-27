// monostate_from.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <format>
#include <functional>
#include <memory>
#include "monostate_from.hpp"
#include "short_function.hpp"
using namespace lib_fm;

class foo;
class abs_foodata {};
class virt_foodata1 {};
class virt_foodata2 {};
class foodata {};


//! [classdef]

struct abs_foobase {
	abs_foodata data_field;
public:
	virtual void bar(foo&)=0;
	virtual void bar()=0;
};

class virt_foobase1
	: virtual public abs_foobase
{
	virt_foodata1 data_field;
};

class virt_foobase2
	: virtual public abs_foobase
{
	virt_foodata2 data_field;
};

class foo
	: virt_foobase1
	, virt_foobase2
{
public:
	foo() = default;
	static void go(foo &, foo &);// a free function
	static void come(foo &);// a free function

	//an overload set:
	void bar(foo &) override;
	void bar();
	
	void bazz(foo &);//a single overload
//private:
	foodata data_field;
};

void foo::go(foo&, foo&) { std::cout << "foo::go\n"; };
void foo::come(foo&) { std::cout << "foo::come\n"; };
void foo::bar(foo &) { std::cout << "foo::bar(foo&)\n"; };
void foo::bar() { std::cout << "foo::bar\n"; };
void foo::bazz(foo &) { std::cout << "foo::bazz\n"; };

//! [classdef]

namespace bc = boost::callable_traits;
namespace bcd = boost::callable_traits::detail;


int main() {

	//! [bindings]
	foo object;

	//native syntax. simple & readable. Downside-> might disable small value optimization:
	auto bazz1 = std::bind_front(&foo::bazz, std::ref(object));
	static_assert(sizeof(bazz1) >= sizeof(std::make_pair(& foo::bazz, std::ref(object))));
	std::function<void(foo &)> bazz1f{ bazz1 };
	std::cout << std::format(	"reference:{}, member pointer:{}, function pointer:{}, bind member:{}, function:{}\n",
								sizeof(std::ref(object)), sizeof(&foo::bazz), sizeof(&foo::go), sizeof(bazz1), sizeof(bazz1f));

	//lambda syntax. small value opimized. Downside-> forwarding might be forgotten in complex cases:
	auto bazz2 = [&object](auto &&x) {return object.bazz(std::forward<decltype(x)>(x)); };
	static_assert(sizeof(bazz2) == sizeof(std::ref(object)));
	std::function<void(foo &)> bazz2f { bazz2 };

	//monostate_from syntax. compact & small value opimized:
	auto bazz3 = std::bind_front(monostate_from<&foo::bazz>, std::ref(object));
	static_assert(sizeof(bazz3) == sizeof(std::ref(object)));
	static_assert(std::is_empty_v<decltype(monostate_from<&foo::bazz>)>, "monostate_from must be empty");
	std::function<void(foo&)> bazz3f{ bazz3 };

	//monostate_from_overload syntax. disambiguate the overload without cast operator:
	std::function<void(foo &)> bar = monostate_from_overload<void(foo:: *)(), &foo::bar>;

	//monostate_from syntax. bind none-instance function:
	std::function<void(foo &)> baxx = std::bind_front(monostate_from<&foo::go>, std::ref(object));
	static_assert(std::is_empty_v<decltype(monostate_from<&foo::go>)>, "monostate_from must be empty");
	//! [bindings]

	//! [short_func]
	short_function<void(foo&)> sh_func = monostate_from<&foo::come>;
	static_assert(sizeof(sh_func) <= sizeof(void*), "short function must be no larger than a void pointer");

	auto const test_short_function = [&sh_func] {
		foo object;
		sh_func(object);
		std::cout << "function source is\t " << to_string_view(sh_func.source()) << "\n";
		std::cout << "empty state is\t " << std::boolalpha << sh_func.is_empty() << "\n\n";
	};

	test_short_function();

	sh_func = monostate_from_overload<void (foo::*)(), &foo::bar>;
	test_short_function();

	sh_func = [](foo&) {std::cout << "lambda as short function\n"; };
	test_short_function();

	struct function_object {
		void operator()(foo&)const { std::cout << "empty object as short function\n"; };
	};

	sh_func = function_object{};
	test_short_function();

	sh_func = {};
	test_short_function();
	//! [short_func]
	
	return EXIT_SUCCESS;
};

