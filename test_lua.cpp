#include "test_lua.h"

using namespace app;

int main(void)
{
	{
		lua::LuaWrap lw;	
		lw.Init();//设定初始化函数 : 可以设定全局初始化函数 lua::SetGlobalInit

		LuaTest_Basic(lw);
		LuaTest_Class_Use(lw);
		LuaTest_Class_Export(lw);
		LuaTest_Class_Exist(lw);
		LuaTest_Class_Customer(lw);
		printf("\n-----begin lua gc-----\n");
		lw.RunString<void>("collectgarbage('collect')");
		printf("\n-----lua gc end,any key gc all-----\n");
	}
	system("pause");
	lua::CleanAll();//清理所有未使用的
	system("pause");
	return 0;
}
