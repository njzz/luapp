#pragma once
#include "LuaExport.h"
//author:njzz
//github:https://github.com/njzz/luapp
//csdn:https://blog.csdn.net/m0_66399932

namespace app {
	namespace lua {

		//////////////////// ---------------------

		//调用并设置返回值
		template <typename R>
		struct InvokeMemberFunction {
			template <typename Class,typename F, typename ...Args>
			inline static int call(lua_State *states,Class *obj, F f, Args&&... args) {
				return LuaArg::set(states, (obj->*f)(std::forward<Args>(args)...));//包含返回值的情况
			}
		};

		//无参数版本，需要根据返回值区分，在RP里不能特化，因为set的void参数不能实例化
		template <>
		struct InvokeMemberFunction<void> {
			template <typename Class, typename F, typename ...Args>
			inline static int call(lua_State *states, Class *obj, F f, Args&&...args) {
				(obj->*f)(std::forward<Args>(args)...);
				return 0;//没有返回值
			}
		};

		//调用对象
		template <typename MFunction>
		class LuaBinderClass :public pcb_set {
		public:
			using CallParamTuple = typename base::mfunction_traits<MFunction>::param_tuple_d;
			using ClassType = typename base::mfunction_traits<MFunction>::class_type;
			using ReturnType = typename base::mfunction_traits<MFunction>::rt_type;

			LuaBinderClass(lua_State *ls, const char *pName, MFunction f) :m_f(f) {
				m_ls=ls;
				m_name = pName;
			}
			~LuaBinderClass() {
				//一般不需要，直接关闭状态机即可，所以不写在析构函数里
				//release()
			}
			
			template<size_t... indexs>
			int FillParamsAndCall(lua_State *states, const std::index_sequence<indexs... > &) {
				//constexpr auto param_size = std::tuple_size<CallParamTuple>::value;				
				auto obj =(ClassType **) lua_touserdata(states, 1);//使用的userdata元表方式，那么第1个参数是userdata的值
				LuaArg::getargs(states, 2, std::get<indexs>(m_p)...);//lua参数从2开始，用tuple存储参数
				return InvokeMemberFunction<ReturnType>::call(states,*obj, m_f, std::get<indexs>(m_p)...);//用tuple里的参数去调用c函数
			}

			//继承函数，开始调用
			int call(lua_State *args) override {
				constexpr auto psize = std::tuple_size<CallParamTuple>::value;
				using  typeindex = std::make_index_sequence<psize>;
				return FillParamsAndCall(args, typeindex{});
			}
			//注销
			//bool release() override {
				////不支持，通过userdata,元表注册的，同一个类的同一个方法，所有对象都共享一个LuaBinderClass
				//return false;//不清理内存，只能全部关闭的时候清理
			//}

		protected:
			MFunction m_f;//成员函数指针
			CallParamTuple m_p{};//参数
		};

		//每个状态机里，每个成员函数都会创建一个 LuaBinderClass
		template<typename T>
		pcb_set *CreateClassPCB(lua_State *l, T v, const char *n) {
			//当前 -1 为元表
			auto r = new LuaBinderClass<T>(l, n, v);
			LuaExportor::PCBCache(r);//缓存起来
			return r;
		}


		//导出对象创建模板函数
		//可以在LUA_CLASS_EXPORT_BEGIN前特例化namespace app{namespace lua{ template<> T* CreateExportLuaObject<T>();}}来创建自定义类对象
		//也可以在包含本文件前重新定义 LUA_EXPORT_NEWOBJ 宏来特殊处理
		template<typename T>
		inline T *CreateExportLuaObject() {
			return new T{};
		}

		//带参数版本
		//template<typename T>
		//inline T *CreateExportLuaObjectP(lua_State *l) {
		//	return CreateExportLuaObject<T>();
		//}

		template<typename T>
		inline void DestroyExportLuaObject(T *o) {
			delete o;
		}
		//////////////////// ---------------------

		

//可以通过重新定义 LUA_EXPORT_NEWOBJ 来重新定义对象从lua里构造方式的方式  t:class type l:lua_state
// 如:#define LUA_EXPORT_NEWOBJ(xxx,l) template<typename xxx> xxx *CreateByLua<xxx>(lua_tointeger(l,1));
#ifndef LUA_EXPORT_NEWOBJ
#define LUA_EXPORT_NEWOBJ(t,l) app::lua::CreateExportLuaObject<t>()
#endif

//可以通过重新定义 LUA_EXPORT_DESTROYOBJ 来重新定义对象的析构方式  t:class type   o:void *,userdata的开始地址，如果转换为对象指针则为 *(T**)o 
// 如: #define LUA_EXPORT_DESTROYOBJ(xxx) void DestroyLuaObj(xxx);
#ifndef LUA_EXPORT_DESTROYOBJ
#define LUA_EXPORT_DESTROYOBJ(t,o) app::lua::DestroyExportLuaObject<t>(*(t**)o)
#endif

//根据类型，获取metatable名
#ifndef LUA_EXPORT_METATABLE
#define LUA_EXPORT_METATABLE(xx) #xx
#endif


//修改class,可以在任意地方注册  ClassName::LuaClassRegister
//在class里申明，对象需要有默认构造函数或重新定义 LUA_EXPORT_NEWOBJ 或特例化 CreateExportLuaObject
#define LUA_CLASS_EXPORT_DECLARE  public:static void LuaClassRegister(lua_State *l);

//不修改class，只能在定义LUA_CLASS_EXPORT_EBEGIN的地方注册
#define LUA_CLASS_EXIST_REG(CLASSNAME,l)  LuaClassRegister_##CLASSNAME(l)

 /*
 Constructor
 1.创建一个对象， 
 2.关联一个到元表
 3.一个返回值，返回set_metatable压栈的元表

GC对象回收:只回收new出来的对象，绑定的函数对象是类公用的，状态机关闭时统一回收
1.检查-1位置参数是否是一个类型为classname的用户数据（参见 luaL_newmetatable ),返回数据的地址（lua_touserdata）


LuaClassRegister  注册开始
1.将构造方法入栈
2.将构造方法名(类名)设置为全局符号，然后弹出栈顶
3.创建一个新表(类名，作为 userdata 的元表)，将它添加到注册表registry中，入栈
4.压入gc名，以重新定义回收
5.gc方法入栈
6.设置元表gc方法 lua_settable(L, -3) //t[k] = v，t:index(-3)在栈中的值(元表) v:栈顶(gc方法) k:是栈顶下的内容(-2,gc名字)  然后弹出k和v

LUA_CLASS_EXPORT_FUNC  当前元表在栈顶  将函数加入到元表方法
7.成员函数名入栈
8.创建lightuserdata PCB 回调函数通过upvalue索引取值，入栈
9.闭包入栈，带上创建的 PCB，并弹出关联的PCB栈
10.将元表name方法设置为闭包函数 lua_settable(L,-3) t[k] = v，t:index在栈中的值(元表) v:栈顶(闭包方法) k:是栈顶下的内容(函数名字)  然后弹出k和v，元表依旧在栈顶

LUA_CLASS_EXPORT_END  当前元表在栈顶   注册__index方法，恢复栈顶
11.__index 名入栈
12.-2元表复制入栈   lua_pushvalue(L, -2)  完成后栈 -1: 元表 -2:__index -3:元表
13.将元表index设置为自己 lua_settable(L, -3) t[k] = v  t:index在栈中的值(元表) v:栈顶(元表) k:是栈顶下的内容(__index)  然后弹出k和v

PS:也可以将__index,__newindex 设置为C函数，然后在C代码里操作

 */

//Constructor and GC
#define LUA_CLASS_EXPORT_CG(CLASSNAME)\
		static int LuaClassConstructor_##CLASSNAME(lua_State* L) {\
			auto obj = LUA_EXPORT_NEWOBJ(CLASSNAME,L);\
			auto r = app::lua::LuaArg::set_metatable(L,app::lua::mtable{obj, LUA_EXPORT_METATABLE(CLASSNAME)},1);\
			if(r==0) {LUA_EXPORT_DESTROYOBJ(CLASSNAME,&obj);}\
			return r;\
		}\
		static int LuaClassGC_##CLASSNAME(lua_State* L){\
			auto obj = (void*)luaL_checkudata(L, -1, LUA_EXPORT_METATABLE(CLASSNAME));\
			auto isgc = *((char *)obj+sizeof(void*));\
			if(isgc){ LUA_EXPORT_DESTROYOBJ(CLASSNAME,obj);}\
			return 0;\
		}\

//Register Code
#define LUA_CLASS_EXPORT_REGISTERCODE(CLASSNAME)\
		auto top = lua_gettop(L);\
		lua_pushcfunction(L, &LuaClassConstructor_##CLASSNAME);\
		lua_setglobal(L,#CLASSNAME);\
		luaL_newmetatable(L, LUA_EXPORT_METATABLE(CLASSNAME));\
		lua_pushstring(L, "__gc");\
		lua_pushcfunction(L, &LuaClassGC_##CLASSNAME);\
		lua_settable(L, -3);\

//not modify class,for exist
#define LUA_CLASS_EXPORT_BEGIN_EXIST(CLASSNAME)\
		LUA_CLASS_EXPORT_CG(CLASSNAME)\
		static void LuaClassRegister_##CLASSNAME(lua_State *L) {\
			LUA_CLASS_EXPORT_REGISTERCODE(CLASSNAME);\

//modify class
#define LUA_CLASS_EXPORT_BEGIN(CLASSNAME)\
		LUA_CLASS_EXPORT_CG(CLASSNAME)\
		void CLASSNAME::LuaClassRegister(lua_State *L) {\
			LUA_CLASS_EXPORT_REGISTERCODE(CLASSNAME)\

#define LUA_CLASS_EXPORT_FUNC(NAME,MEMFUNC) \
		lua_pushstring(L,NAME);\
		lua_pushlightuserdata(L, app::lua::CreateClassPCB(L, MEMFUNC, NAME));\
		lua_pushcclosure(L, &app::lua::pcb_set::LuaCallBack, 1);\
		lua_settable(L, -3);\

#define LUA_CLASS_EXPORT_END() \
		lua_pushstring(L, "__index");\
		lua_pushvalue(L, -2);\
		lua_settable(L, -3);\
		lua_settop(L, top);}		
	}
}
