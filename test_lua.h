#pragma once
#include <stdio.h>
#include "LuaWrap.h"

//基础测试，测试基本的加载执行lua文件，执行字符串lua代码，调用lua函数,lua调用c++函数
void LuaTest_Basic(app::lua::LuaWrap &lw);
//测试使用现有类，使用其它成员函数
void LuaTest_Class_Use(app::lua::LuaWrap &lw);
//测试导出新类/派生类，虚函数等情况
void LuaTest_Class_Export(app::lua::LuaWrap &lw);
//测试导出现有类，不能修改类代码的情况
void LuaTest_Class_Exist(app::lua::LuaWrap &lw);
//测试自定义类构造
void LuaTest_Class_Customer(app::lua::LuaWrap &lw);

/*
导出类的宏分为:
LUA_CLASS_USE_BEGIN:使用导出，不能在lua代码里使用构造方法构造(未注册/生成构造函数，gc函数)，仅在lua代码里使用，适用于二级对象(一级对象的方法里返回该对象(mtable))
LUA_CLASS_EXIST_BEGIN:现有导出，不对现有类做任何修改，可通过类名构造对象(一级对象)
LUA_CLASS_EXPORT_BEGIN:修改类，派生类导出，在类中做申明，可以在任何地方调用注册，可通过类名构造对象(一级对象)
LUA_CLASS_CUSTOMER_BEGIN:自定义导出，可以自定义构造函数名，构造函数(返回btable，一级对象)，tablename


可选方法:

EXIST,EXPORT 方式对象构造默认使用new，可以自定义，有如下方式
1.特例化 CreateExportLuaObjectP 函数，可以接受lua参数
2.特例化 CreateExportLuaObject 函数，不接受lua参数
3.重定义 LUA_EXPORT_NEWOBJ(CLASS,L) 宏

成员函数可以使用其它函数代替，使用 LUA_OTHER_FUNC 宏

对象析构的自定义方法
1.特例化 DestroyExportLuaObject 函数
2.重定义宏 LUA_EXPORT_DESTROYOBJ(CLASS,obj)

更多说明参考 LuaExportClass.h
*/