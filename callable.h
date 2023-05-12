#pragma once

#include <memory>
#include <functional>
#include <tuple>
#include <vector>

namespace base {

	//��������׷��
	//��ȡ��������ֵ���ͣ�����tuple���ͣ�ָ������
	//typedef typename function_traits<Function>::param_tuple  CallParamTuple;
	//typedef typename function_traits<Function>::rt_type  FuncRtType;
	//typedef typename function_traits<Function>::p_type  FuncPointerType;
	template <typename Function>
	struct function_traits : public function_traits < decltype(&Function::operator()) >
	{
	};


	//function �����ػ�
	template <typename ClassType, typename ReturnType, typename ...Args>
	struct function_traits < ReturnType(ClassType::*)(Args...) const >
	{
		typedef std::function<ReturnType(Args...)> f_type;//function ����
		typedef ReturnType(*p_type)(Args...);//pointer ����
		typedef ReturnType rt_type;//return ����
		typedef std::tuple<Args...> param_tuple;//param ����
	};


	//ָ�뺯���ػ�
	template <typename ReturnType, typename ...Args>
	struct function_traits< ReturnType(*)(Args...) >
	{
		typedef std::function<ReturnType(Args...)> f_type;//function ����
		typedef ReturnType(*p_type)(Args...);//pointer ����
		typedef ReturnType rt_type;//return ����
		typedef std::tuple<Args...> param_tuple;//param ����
	};

	//ָ�뺯���ػ� __stdcall
	//��windows/linux X64�£�ֻ��һ�ֵ��÷�ʽ(__fastcall������__stdcall/) ��ʹ��4/6���Ĵ������ݲ�������Ĵ�������ѹջ���ɵ�������ջ
	//x64 �µ��÷�ʽ�����ᱻ���������ԣ�ֻ��Ҫ���岻�������÷�ʽ��ģ�弴��
	//��x86 �£�Ĭ��Ϊ __cdecl ��ʽ����������һ�� __stdcall ���÷�ʽ�ģ�linuxֻʹ�� 64λ��
#if  !defined(WIN64) && defined(WIN32) 
	template <typename ReturnType, typename ...Args>
	struct function_traits< ReturnType(__stdcall *)(Args...) >
	{
		typedef std::function<ReturnType(Args...)> f_type;//function ����
		typedef ReturnType(*p_type)(Args...);//pointer ����
		typedef ReturnType rt_type;//return ����
		typedef std::tuple<Args...> param_tuple;//param ����
	};
#endif

	//��Ա�������ͣ����ݳ�Աָ��
	template <typename T>
	struct mfunction_traits : public mfunction_traits < decltype(T) > {};

	//��ͨ�汾
	template <typename ClassType, typename ReturnType, typename ...Args>
	struct mfunction_traits < ReturnType(ClassType::*)(Args...)>
	{
		using class_type = ClassType;
		using p_type = ReturnType(ClassType::*)(Args...);//pointer ����
		using param_tuple = std::tuple< Args...>;//param ����
		using rt_type = ReturnType;//return ����
	};

	//const �汾
	template <typename ClassType, typename ReturnType, typename ...Args>
	struct mfunction_traits < ReturnType(ClassType::*)(Args...) const>
	{
		using class_type = ClassType;
		using p_type = ReturnType(ClassType::*)(Args...) const;//pointer ����
		using param_tuple = std::tuple< Args...>;//param ����
		using rt_type = ReturnType;//return ����
	};
	
}