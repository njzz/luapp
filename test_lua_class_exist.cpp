#include "test_lua.h"

using namespace app;

 //向Lua导出已有类，不修改/不继承类，可以在lua里创建
 class Class_Exist {
 protected:
	 Class_Exist(const char *from) { printf("Class_Exist Create,from:%s!\n",from); }//不能在外部构造
	 ~Class_Exist(){ printf("Class_Exist Destroy!\n"); }//不能在外部析构
 public:
	 void print(const char *p) {
		 printf("Class_Exist print:%s\n",p);
	 }

	 //模拟未导出到lua的构造函数/类工厂
	 static Class_Exist* Create(const char *from="C++") {
		 return new Class_Exist(from);
	 }

	 static void DestroyObj(Class_Exist* o) {
		 delete o;
	 }
 };

 //使用函数特例化方式重新构造/析构对象
 namespace app {
	 namespace lua {
		 template<> Class_Exist* CreateExportLuaObject<Class_Exist>() {
			 return Class_Exist::Create("Lua");
		 }
		 template<> void DestroyExportLuaObject<Class_Exist>(Class_Exist *o) {
			 Class_Exist::DestroyObj(o);
		 }
	 }
 }

 //使用宏方式重新构造对象
//#undef LUA_EXPORT_NEWOBJ
//#define LUA_EXPORT_NEWOBJ(t,l) Class_Exist::Create("Lua")

//使用宏方式重新定义析构
//#undef LUA_EXPORT_DESTROYOBJ
//#define LUA_EXPORT_DESTROYOBJ(n,o) Class_Exist::DestroyObj(*(n**)o)

LUA_CLASS_EXIST_BEGIN(Class_Exist)  //EXIST_BEGIN 现有导出，不对现有类做任何修改
	LUA_MEM_FUNC("print", &Class_Exist::print)
LUA_CLASS_END() //结束

//导出类测试/不修改类
void LuaTest_Class_Exist(lua::LuaWrap &lw) {

	printf("\n----begin test class-exist ----\n");

	 //类导出注册给lua
	 LUA_CLASS_EXIST_REG(Class_Exist, lw.get());

	 //对象在lua生成，并调用成员函数
	 lw.RunString<void>(R"(			 
			 local e1 = Class_Exist()
			 e1:print(type(e1))
		)" );
 }

