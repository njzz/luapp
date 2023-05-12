#include <stdio.h>
#include "LuaWrap.h"

using namespace app;

 //--------------------测试调用lua函数---------------------------
 void RunLuaCaller() {
	 lua::LuaWrap lw;
	 //设定初始化函数 : 可以设定全局初始化函数 lua::SetGlobalInit
	 lw.Init();
	 if (lw.DoFile("hello.lua")) {//确认加载hello.lua成功

		 //调用初始化函数 Lua_initfunc 里加载都hello.lua里的函数
		 printf("AddNum(20,30) = %d \n", lw.Call<int>("AddNum", 20, 30));
		 printf("utils.math.add(20,30,100) = %d \n", lw.Call<int>("utils.math.add", 20, 30, 100));
		 auto str = lw.Call<std::string>("AddString", "string is ", "added");
		 printf("AddString(string is ,added)=%s\n", str.c_str());

		 //lua 返回多个返回值
		 auto rtuple = lw.Call<std::tuple<std::string, int, std::string>>("Rt3Value", "v1", 2, "v3");
		 printf("Rt3Value('v1',2,'v3')=%s,%d,%s\n", std::get<0>(rtuple).c_str(), std::get<1>(rtuple), std::get<2>(rtuple).c_str());

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
 }

 //-----------------------向lua导出函数--------------------------
 //导出类成员函数
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

 //----------------------向Lua导出类，类似于MFC消息映射框架--------------------
 //基类，含有一个虚函数
 class TestClassBase {
 public:
	 virtual void print() const {
		 printf("base class print\n");
	 }
 };

 //派生类:导出
 class TestClass:public TestClassBase {
 public:
	 TestClass() {//从Lua创建  也可以在包含luaExportClass.h前重新定义宏 LUA_EXPORT_NEWOBJ，来自定义创建类对象，默认直接 new
		 m_createByLua = true;
		 printf("TestClass Constructor:Lua\n");
	 }

	 TestClass(int ) {//从c++创建
		 m_createByLua = false;
		 printf("TestClass Constructor:C++\n");
	 }

	 ~TestClass() {
		 if(m_createByLua)
			 printf("TestClass Destructor:Lua\n");
		 else
			 printf("TestClass Destructor:C++\n");
	 }

	 virtual void print() const override{
		 if(m_createByLua)
			printf("subclass:I am create by lua\n");
		 else
			printf("subclass:I am create by c++\n");
	 }

	 //该函数未导出
	 void printcpp() {
		 printf("not export function,only use in c++\n");
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
	 bool m_createByLua;
	 //导出申明
	 LUA_CLASS_EXPORT_DECLARE;
 };

 LUA_CLASS_EXPORT_BEGIN(TestClass)  //导出实现开始
	 LUA_CLASS_EXPORT_FUNC("add", &TestClass::add) //设定导出的成员函数名和函数地址
	 LUA_CLASS_EXPORT_FUNC("getvalue", &TestClass::getv) //在lua里换个名字
	 LUA_CLASS_EXPORT_FUNC("setv", &TestClass::setv) 
	 LUA_CLASS_EXPORT_FUNC("ppt", &TestClassBase::print) //虚函数，调用派生类的函数
LUA_CLASS_EXPORT_END() //结束

//导出类测试
 void RunLuaExportClass() {
	 lua::LuaWrap lw;
	 lw.Init();

	 //调用导出类的  LuaClassRegister 函数导出该类到当前状态机
	 TestClass::LuaClassRegister(lw.get());

	 //对象在lua生成，并调用成员函数
	 lw.RunString<void>(
		 R"(
			 local t1 = TestClass() --new object:local
             local v = t1:add(1, 4)  -- 5
             t1:setv(6) 
             t2 = TestClass() --new object:global
			 t2:setv(v)
             print('t1:getvalue()='..t1:getvalue())  --6
             print('t2:getvalue()='..t2:getvalue()) --5 
             t1:ppt()  --print
		)"
		 );

	 //定义一个 lambda ，导出给lua，接收lua传递的参数
	 auto UseLuaObj = [](void *o) {
		 //如果参数为lightuserdata(先由c++通过void*传过去，再被传出来)，则直接转换为对象指针
		 //所以void *根据具体情况使用
		 //这里接收lua传递过来的 TestClass 对象
		 auto pobj = *(TestClass **)o;
		 pobj->printcpp(); //打印，该函数未导出，仅能在c++里调用
	 };
	 lw.RegFunction("UseLuaObj", UseLuaObj);//注册函数 UseLuaObj 给 lua
	 lw.RunString<void>("UseLuaObj(t2)");//在lua里调用导出函数，将全局对象t2传出来

	 //对象已经在c++中生成，传递给lua使用
	 //编写一个lua函数，将c++创建的对象，传递给lua使用(该类已被导出)
	 TestClass tc(1);
	 lw.RunString<void>(
		 R"(
			 function PassByParam(p)
				 p:ppt() --print
		     end
		)",//定义lua函数 PassByParam
		 "PassByParam",//调用lua函数 PassByParam
		 lua::mtable{&tc,"TestClass"});//将tc指针作为导出的TestClass传递给函数 PassByParam
	 //PS:也可以使用 LUA_EXPORT_METATABLE(TestClass)
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
