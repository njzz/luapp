#include "test_lua.h"

using namespace app;

 //--------------------测试调用lua函数---------------------------
 void RunLuaCaller(lua::LuaWrap &lw) {
	 printf("\n----begin test call lua function----\n");

	 //直接调用lua函数
	 lw.RunString<void>("print(\"run sample string code\") ");
	 //定义字符串函数，并调用该函数
	 lw.RunString<void>("function PrintParam(vv) print(\"param:\"..vv) end ", "PrintParam", "pass by string");
	 //调用lua函数print，传递字符串参数
	 lw.Call<void>("print", "call lua self function:print\n");

	 if (lw.DoFile("hello.lua")) {//确认加载hello.lua成功	

		 //调用加载的hello.lua里的函数
		 printf("AddNum(20,30) = %d \n", lw.Call<int>("AddNum", 20, 30));
		 //调用xx.bb.cc类函数
		 printf("utils.math.add(20,30,100) = %d \n", lw.Call<int>("utils.math.add", 20, 30, 100));
		 //调用返回字符串类函数 也可以是 const char *
		 auto str = lw.Call<std::string>("AddString", "string is ", "added");
		 printf("AddString(string is ,added)=%s\n", str.c_str());

		 //lua 返回多个返回值
		 auto rtuple = lw.Call<std::tuple<std::string, int, std::string>>("Rt3Value", "v1", 2, "v3");
		 printf("Rt3Value('v1',2,'v3')=%s,%d,%s\n", std::get<0>(rtuple).c_str(), std::get<1>(rtuple), std::get<2>(rtuple).c_str());

		 //lua 返回多个返回值
		 auto rtuple2 = lw.Call<std::tuple<int, float, std::string>>("Rt3Value", 2, 2.f, "second");
		 printf("Rt3Value(2,2.f,'second')=%d,%f,%s\n", std::get<0>(rtuple2), std::get<1>(rtuple2), std::get<2>(rtuple2).c_str());		 
	 }
	 else {//加载不成功，打印错误信息
		 printf(lw.GetLastError().c_str());
	 }
 }

 //-----------------------向lua导出c++函数--------------------------
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
 void RunLuaExportor(lua::LuaWrap &lw){
	printf("\n----begin test function export----\n");
	//注册普通函数并调用
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

 void LuaTest_Basic(app::lua::LuaWrap &lw) {
	 RunLuaCaller(lw);
	 RunLuaExportor(lw);
 }