#include "test_lua.h"

using namespace app;

//向Lua导出已有类，不修改/不继承类，且只能在lua里使用(不创建)
//仅使用的情况
class Class_OnlyUse {
public:
	void print(const char *v) {
		printf("Class_OnlyUse print:%s\n",v);
	}
};

//该函数在成员函数里以 LUA_OTHER_FUNC 导出，第一个参数必须为 void *,且转换为导出类的二级指针
static void TestFunc(void *o, int i) {
	auto p = *(Class_OnlyUse**)o;
	std::string ss{ "lua param " };
	ss += std::to_string(i);
	p->print(ss.c_str());
}

LUA_CLASS_USE_BEGIN(Class_OnlyUse)  //USE_BEGIN 仅做参数使用，不能在lua里通过 Class_OnlyUse() 创建，Class_OnlyUse为类型名
LUA_MEM_FUNC("print", &Class_OnlyUse::print)
LUA_OTHER_FUNC("other", &TestFunc)
LUA_CLASS_END() //结束

//导出类测试/不修改类
void LuaTest_Class_Use(lua::LuaWrap &lw) {

	printf("\n----begin test class-use ----\n");

	//注册
	LUA_CLASS_USE_REG(Class_OnlyUse, lw.get());

	//对象在lua生成，产生错误
	lw.RunString<void>(R"( local er = Class_OnlyUse())"); //GetLastError.
	printf("Lua Error:%s\n",lw.GetLastError().c_str());

	//在c++创建对象，只能在lua使用
	//定义一个函数，接收 Class_OnlyUse 参数，调用成员函数
	Class_OnlyUse co;
	lw.RunString<void>(R"( function ClassOnlyUse(x)
								x:print('call in lua') --call mem func print
								x:other(66) --call other fun
							end)","ClassOnlyUse",app::lua::mtable(&co, LUA_EXPORT_METATABLE(Class_OnlyUse)));
}


