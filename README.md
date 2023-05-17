# luapp
lua cpp 封装，方便使用

测试和使用，参考test_lua.cpp

所用lua版本 5.3.5，需要使用luajit的同学自行修改

代码在windows 7上使用vs2017编译测试通过x86/x64,未使用windows专用代码

新建工程，将所有文件包含进去即可，注意设置lua库的include 和 lib 目录。


导出类的宏分为:

LUA_CLASS_USE_BEGIN:使用导出，不能在lua代码里使用构造方法构造(未注册/生成构造函数，gc函数)，适用于二级对象(一级对象的方法里返回该对象(mtable))

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