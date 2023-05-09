#include <stdio.h>
#include "LuaWrap.h"

using namespace app;

 bool Lua_initfunc(lua::LuaWrap &lvm) {
	lvm.DoFile("hello.lua");
	return true;
}

 //测试调用lua函数
 void RunLuaCaller() {
	 lua::LuaWrap lw;
	 //设定初始化函数，也可以设定全局初始化函数 lua::SetGlobalInit
	 lw.Init(&Lua_initfunc);

	 //调用初始化函数 Lua_initfunc 里加载都hello.lua里的函数
	 printf("AddNum(20,30) = %d \n", lw.Call<int>("AddNum", 20, 30));
	 printf("utils.math.add(20,30,100) = %d \n", lw.Call<int>("utils.math.add", 20, 30, 100));
	 auto str = lw.Call<std::string>("AddString", "string is ", "added");
	 printf("AddString(string is ,added)=%s\n",str.c_str());

	 //lua 返回多个返回值
	 auto rtuple = lw.Call<std::tuple<std::string, int,std::string>>("Rt3Value", "v1", 2 , "v3");
	 printf("Rt3Value('v1',2,'v3')=%s,%d,%s\n", std::get<0>(rtuple).c_str(),std::get<1>(rtuple), std::get<2>(rtuple).c_str());

	 //lua 返回多个返回值
	 auto rtuple2 = lw.Call<std::tuple<int, float, std::string>>("Rt3Value", 2, 2.f, "second");
	 printf("Rt3Value(2,2.f,'second')=%d,%f,%s\n", std::get<0>(rtuple2), std::get<1>(rtuple2), std::get<2>(rtuple2).c_str());

	 //调用lua函数print
	 lw.Call<void>("print", "call lua self function:print\n");

	 //定义字符串函数，并调用该函数
	 lw.RunString<void>("function PrintParam(vv) print(\"param:\"..vv) end  ", "PrintParam", "pass by string");
	 //直接调用字符串函数
	 lw.RunString<void>("print(\"run sample string code\") ");
 }

 //导出一个成员函数
 class KKK {
 public:
	 void print(int a) {
		 printf("Class member function KKK::print v:%d\n",a);
	 }
 };

 //导出普通函数
 static std::string NormalFunc(std::string p) {
	 return p + " : cpp added.";
 }

 //测试向lua导出函数
 void RunLuaExportor(){
	lua::LuaWrap lw;
	lw.Init();

	lw.RegFunction("func_addstr", &NormalFunc);
	lw.RunString<void>("print(func_addstr('call normal function'))");

	//导出lambda，返回两个值
	auto exLam = [](int a) {
		return std::tuple<int,int>(a + 1,a+2);
	};

	//注册xxx类函数并调用
	lw.RegFunction("exLamabc", exLam);
	lw.RunString<void>("print('exLamabc(5)=',exLamabc(5))");

	//注册xxx.yyy类函数并调用
	lw.RegFunction("exLam.abcd", exLam);
	lw.RunString<void>("print('exLam.abcd(6)=',exLam.abcd(6))");

	//注册任意 aaa.bbb.ccc.ddd 函数并调用
	lw.RegFunction("exLam.kk.cc.bb", exLam);
	lw.RunString<void>("print('exLam.kk.cc.bb(7)=',exLam.kk.cc.bb(7))");

	KKK k;
	//注意使用 std::function 确定类型
	std::function<void(int)> ff = std::bind(&KKK::print, &k , std::placeholders::_1);
	lw.RegFunction("kkk.print", ff);//注意 k 的生命周期
	lw.RunString<void>("kkk.print(6)");
}

 //向Lua导出类，类似于MFC框架
 class TestClass {
 public:
	 TestClass() {
		 printf("TestClass Constructor!\n");
	 }

	 ~TestClass() {
		 printf("TestClass Destructor!\n");
	 }

	 int add(int a, int b) {
		 return  a + b;
	 }

	 void setv(int value) {
		 m_value = value;
	 }

	 int getv() {
		 return m_value;
	 }

	 int m_value;
	 //导出申明
	 LUA_CLASS_EXPORT_DECLARE;
 };

 LUA_CLASS_EXPORT_BEGIN(TestClass)  //导出实现开始
	 LUA_CLASS_EXPORT_FUNC("add", &TestClass::add) //设定导出的成员函数名和函数地址，支持导出基类函数，只需要指明类即可
	 LUA_CLASS_EXPORT_FUNC("getvalue", &TestClass::getv) //在lua里换个名字
	 LUA_CLASS_EXPORT_FUNC("setv", &TestClass::setv) 
LUA_CLASS_EXPORT_END() //结束

//测试向lua导出类
 void RunLuaExportClass() {
	 lua::LuaWrap lw;
	 lw.Init();

	 //调用导出类的  LuaClassRegister 函数导出该类到当前状态机
	 TestClass::LuaClassRegister(lw.get());

	 //测试导出的类
	 lw.RunString<void>(
		 R"( 
			 local t1 = TestClass() --new object
             local v = t1:add(1, 4)  -- 5
             t1:setv(6) 
             local t2 = TestClass() --new object
			 t2:setv(v)
             print('t1:getvalue()='..t1:getvalue())  --6
             print('t2:getvalue()='..t2:getvalue()) --5 
		)"
		 );
 }

int main(void)
{
	RunLuaCaller();
	RunLuaExportor();
	RunLuaExportClass();
	lua::CleanAll();//清理所有
	system("pause");
	return 0;
}
