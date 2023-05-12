#pragma once

#include <memory>
#include <functional>
#include <tuple>
#include <vector>

namespace base {

	//函数类型追踪
	//获取函数返回值类型，参数tuple类型，指针类型
	//typedef typename function_traits<Function>::param_tuple  CallParamTuple;
	//typedef typename function_traits<Function>::rt_type  FuncRtType;
	//typedef typename function_traits<Function>::p_type  FuncPointerType;
	template <typename Function>
	struct function_traits : public function_traits < decltype(&Function::operator()) >
	{
	};


	//function 类型特化
	template <typename ClassType, typename ReturnType, typename ...Args>
	struct function_traits < ReturnType(ClassType::*)(Args...) const >
	{
		typedef std::function<ReturnType(Args...)> f_type;//function 类型
		typedef ReturnType(*p_type)(Args...);//pointer 类型
		typedef ReturnType rt_type;//return 类型
		typedef std::tuple<Args...> param_tuple;//param 类型
	};


	//指针函数特化
	template <typename ReturnType, typename ...Args>
	struct function_traits< ReturnType(*)(Args...) >
	{
		typedef std::function<ReturnType(Args...)> f_type;//function 类型
		typedef ReturnType(*p_type)(Args...);//pointer 类型
		typedef ReturnType rt_type;//return 类型
		typedef std::tuple<Args...> param_tuple;//param 类型
	};

	//指针函数特化 __stdcall
	//在windows/linux X64下，只有一种调用方式(__fastcall，类似__stdcall/) ，使用4/6个寄存器传递参数，多的从右往左压栈，由调用者清栈
	//x64 下调用方式申明会被编译器忽略，只需要定义不申明调用方式的模板即可
	//在x86 下，默认为 __cdecl 方式，这里申明一个 __stdcall 调用方式的，linux只使用 64位的
#if  !defined(WIN64) && defined(WIN32) 
	template <typename ReturnType, typename ...Args>
	struct function_traits< ReturnType(__stdcall *)(Args...) >
	{
		typedef std::function<ReturnType(Args...)> f_type;//function 类型
		typedef ReturnType(*p_type)(Args...);//pointer 类型
		typedef ReturnType rt_type;//return 类型
		typedef std::tuple<Args...> param_tuple;//param 类型
	};
#endif

	//成员函数类型，根据成员指针
	template <typename T>
	struct mfunction_traits : public mfunction_traits < decltype(T) > {};

	//普通版本
	template <typename ClassType, typename ReturnType, typename ...Args>
	struct mfunction_traits < ReturnType(ClassType::*)(Args...)>
	{
		using class_type = ClassType;
		using p_type = ReturnType(ClassType::*)(Args...);//pointer 类型
		using param_tuple = std::tuple< Args...>;//param 类型
		using rt_type = ReturnType;//return 类型
	};

	//const 版本
	template <typename ClassType, typename ReturnType, typename ...Args>
	struct mfunction_traits < ReturnType(ClassType::*)(Args...) const>
	{
		using class_type = ClassType;
		using p_type = ReturnType(ClassType::*)(Args...) const;//pointer 类型
		using param_tuple = std::tuple< Args...>;//param 类型
		using rt_type = ReturnType;//return 类型
	};
	
}