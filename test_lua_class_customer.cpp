#include "test_lua.h"

using namespace app;

//自定义类的构造
class Class_Customer {
public:
	Class_Customer(const char *v) {
		printf("Class_Customer Create,Param:%s\n", v);
	}
	~Class_Customer() {
		printf("Class_Customer Destructor\n");
	}
	void print(const char *v) {
		printf("Class_Customer print:%s\n", v);
	}
	std::string cparam;
};

//返回值必须是 btable，可以使用任意参数
static app::lua::btable CustomerCreate(const char *sets) {
	return {new Class_Customer(sets),LUA_EXPORT_METATABLE(Class_Customer)};
}

//类名:用于定义注册函数名和GC函数名，元表名,构造函数名,构造函数地址
LUA_CLASS_CUSTOMER_BEGIN(Class_Customer,"Class_Customer","cpp.Class_Customer",&CustomerCreate) 
	LUA_MEM_FUNC("print", &Class_Customer::print)
LUA_CLASS_END()

//导出类测试/不修改类
void LuaTest_Class_Customer(lua::LuaWrap &lw) {

	printf("\n----begin test class-customer ----\n");

	//类导出注册给lua
	LUA_CLASS_CUSTOMER_REG(Class_Customer, lw.get());

	//对象在lua生成，并调用成员函数
	lw.RunString<void>(R"(			 
			 local ecc = cpp.Class_Customer('create param passed') --create
			 ecc:print(type(ecc))
		)");
}
