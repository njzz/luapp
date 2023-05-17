#include "test_lua.h"

using namespace app;

 
 //----------------------向Lua导出类，类似于MFC消息映射框架--------------------
 //基类，含有一个虚函数
 class Class_ExportBase {
 public:
	 virtual void print(const char *) const {
		 printf("base class print\n");
	 }
 };

 //派生类:导出
 class Class_Export:public Class_ExportBase {
 public:
	 Class_Export() {//从Lua创建
		 m_createByLua = true;
		 printf("Class_Export Constructor:Lua\n");
	 }

	 Class_Export(int ) {//从c++创建
		 m_createByLua = false;
		 printf("Class_Export Constructor:C++\n");
	 }

	 ~Class_Export() {
		 if(m_createByLua)
			 printf("Class_Export Destructor:Lua\n");
		 else
			 printf("Class_Export Destructor:C++\n");
	 }

	 virtual void print(const char *v) const override{
		 if(m_createByLua)
			printf("create by lua:%s\n",v);
		 else
			printf("create by c++:%s\n",v);
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

 LUA_CLASS_EXPORT_BEGIN(Class_Export)  //导出开始 EXPORT_BEGIN lua代码里可以通过 Class_Export()创建对象
	 LUA_MEM_FUNC("add", &Class_Export::add) //设定导出的成员函数名和函数地址
	 LUA_MEM_FUNC("getvalue", &Class_Export::getv) //在lua里换个名字
	 LUA_MEM_FUNC("setv", &Class_Export::setv)
	 LUA_MEM_FUNC("print", &Class_ExportBase::print) //虚函数，调用派生类的函数
LUA_CLASS_END() //结束

//导出类测试函数
 void LuaTest_Class_Export(lua::LuaWrap &lw) {

	 printf("\n----begin test class-export ----\n");

	 //调用导出类的  LuaClassRegister 函数导出该类到当前状态机
	 //也可以使用宏LUA_CLASS_EXPORT_REG(Class_Export, lw.get());
	 Class_Export::LuaClassRegister(lw.get());

	 //对象在lua生成，并调用成员函数
	 lw.RunString<void>(
		 R"(
			 local t1 = Class_Export() --new object:local
             local v = t1:add(1, 4)  -- 5
             t1:setv(6) 
             t2 = Class_Export() --new object:global
			 t2:setv(v)
             print('t1:getvalue()='..t1:getvalue())  --6
             print('t2:getvalue()='..t2:getvalue()) --5 
             t1:print('call in lua')  --print
		)" );

	 //定义一个 lambda ，导出给lua，接收lua传递的参数
	 auto UseLuaObj = [](void *o) {
		 //如果参数为lightuserdata(先由c++通过void*传过去，再被传出来)，则直接转换为对象指针		 
		 //这里接收lua传递过来的 Class_Export 对象，所以void *根据具体情况使用
		 auto pobj = *(Class_Export **)o;
		 pobj->printcpp(); //打印，该函数未导出，仅能在c++里调用
	 };
	 lw.RegFunction("UseLuaObj", UseLuaObj);//注册函数 UseLuaObj 给 lua
	 lw.RunString<void>("UseLuaObj(t2)");//在lua里调用导出函数，将全局对象t2传出来

	 //对象已经在c++中生成，传递给lua使用
	 //编写一个lua函数，将c++创建的对象，传递给lua使用(该类已被导出)
	 Class_Export tc(1);
	 lw.RunString<void>(
		 R"( function PassByParam(p)
				 p:print('call in lua') --print
		     end
		)",//定义lua函数 PassByParam
		 "PassByParam",//调用lua函数 PassByParam
		 lua::mtable{&tc,LUA_EXPORT_METATABLE(Class_Export) });//将tc指针作为导出的Class_Export传递给函数 PassByParam
 }

