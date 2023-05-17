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

			LuaBinderClass( const char *pName, MFunction f) :m_f(f) {
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
				CallParamTuple params{};//参数
				LuaArg::getargs(states, 2, std::get<indexs>(params)...);//lua参数从2开始，用tuple存储参数
				return InvokeMemberFunction<ReturnType>::call(states,*obj, m_f, std::get<indexs>(params)...);//用tuple里的参数去调用c函数
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
		};

		//每个状态机里，每个成员函数都会创建一个 LuaBinderClass
		template<typename T>
		pcb_set *CreateClassPCB(lua_State *l, T v, const char *n) {
			//当前 -1 为元表
			auto r = new LuaBinderClass<T>(n, v);
			LuaExportor::PCBCache(l,r);//缓存起来
			return r;
		}


		//导出对象创建模板函数
		//可以在LUA_CLASS_EXPORT_BEGIN前特例化namespace app{namespace lua{ template<> T* CreateExportLuaObject<T>();}}来创建自定义类对象
		//也可以在包含本文件前重新定义 LUA_EXPORT_NEWOBJ 宏来特殊处理
		template<typename T>
		inline T *CreateExportLuaObject() {
			return new T{};
		}

		//带参数版本，参数可以通过 lua::LuaArg::get(l,1/2/3/4,...)/getargs(l,1,...) 获取
		template<typename T>
		inline T *CreateExportLuaObjectP(lua_State *l) {
			return CreateExportLuaObject<T>();
		}

		template<typename T>
		inline void DestroyExportLuaObject(T *o) {
			delete o;
		}
		//////////////////// ---------------------
		struct ClassRegSet {
			const char *cons_name=nullptr;//构造器，注册名
			lua_CFunction cons_fnc=nullptr;//构造器，注册函数
			pcb_set *cons_pcb=nullptr;//构造器注册 pcb 和上面两个互斥，且优先使用

			const char *metatable=nullptr;//metatable 名
			lua_CFunction gc_fnc=nullptr;//gc函数
			std::list<pcb_set*> funcs;//可调用成员

		};
		bool ExportClassToLua(lua_State *L,const ClassRegSet &rs);

//可以通过重新定义 LUA_EXPORT_NEWOBJ 来重新定义对象从lua里构造方式的方式  t:class type l:lua_state
// 如:#define LUA_EXPORT_NEWOBJ(xxx,l) template<typename xxx> xxx *CreateByLua<xxx>(lua_tointeger(l,1));
#ifndef LUA_EXPORT_NEWOBJ
#define LUA_EXPORT_NEWOBJ(t,l) app::lua::CreateExportLuaObjectP<t>(l)
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

//注册宏，4种
#define LUA_CLASS_USE_REG(CLASSNAME,l)  LuaClassRegister_##CLASSNAME(l)
#define LUA_CLASS_EXIST_REG(CLASSNAME,l)  LuaClassRegister_##CLASSNAME(l)
#define LUA_CLASS_CUSTOMER_REG(CLASSNAME,l)  LuaClassRegister_##CLASSNAME(l)
#define LUA_CLASS_EXPORT_REG(CLASSNAME,l)  CLASSNAME::LuaClassRegister(l)

 /*
 Constructor
 1.创建一个对象， 
 2.关联一个到元表
 3.一个返回值，返回set_metatable压栈的元表

GC对象回收:只回收new出来的对象，绑定的函数对象是类公用的，状态机关闭时统一回收
1.检查-1位置参数是否是一个类型为classname的用户数据（参见 luaL_newmetatable ),返回数据的地址（lua_touserdata）

 */

#define LUA_FNC_NAME_CONSTRUCTOR(CLASSNAME) LuaClassConstructor_##CLASSNAME
#define LUA_FNC_NAME_GC(CLASSNAME) LuaClassGC_##CLASSNAME

//可以特例化 app::lua::CreateExportLuaObjectP，然后调用 LuaArg::get/getargs,获取参数
//需要在lua里通过函数构造的，参数为 ClassName(类型名),TableName(元表名)
//构造一个对象，并设置对象的metatable为TABLENAME，1:标记随userdata销毁c++对象
#define LUA_CLASS_FNC_CONSTRUCTOR_DEFINE(CLASSNAME,TABLENAME)\
		static int LUA_FNC_NAME_CONSTRUCTOR(CLASSNAME)(lua_State* L) {\
			auto obj = LUA_EXPORT_NEWOBJ(CLASSNAME,L);\
			auto r = app::lua::LuaArg::set(L,app::lua::btable{obj, TABLENAME});\
			if(r==0) {LUA_EXPORT_DESTROYOBJ(CLASSNAME,&obj);}\
			return r;\
		}\

//析构，所有的一致 ，如果只是使用mtable，则isgc = 0 不析构
#define LUA_CLASS_FNC_GC_DEFINE(CLASSNAME,TABLENAME)\
		static int LUA_FNC_NAME_GC(CLASSNAME)(lua_State* L){\
			auto obj = (void*)luaL_checkudata(L, -1,TABLENAME);\
			auto isgc = *((char *)obj+sizeof(void*));\
			if(isgc){ LUA_EXPORT_DESTROYOBJ(CLASSNAME,obj);}\
			return 0;\
		}\


//-----------------------------------------------------

//不修改类，不用申明，对象只能在lua使用(使用mtable)，不能调用CLASSNAME()创建
//使用 LUA_CLASS_USE_REG(classname,l) 注册
//不定义Constructor函数，不定义GC函数,定义注册函数，构造参数调用注册函数
#define LUA_CLASS_USE_BEGIN(CLASSNAME)\
		static void LuaClassRegister_##CLASSNAME(lua_State *L) {\
				app::lua::ClassRegSet rs;/*rs.gc_fnc=&LUA_FNC_NAME_GC(CLASSNAME);*/rs.metatable = LUA_EXPORT_METATABLE(CLASSNAME);\

//不修改类，不用申明，可在lua里通过CLASSNAME()构造
//使用 LUA_CLASS_EXIST_REG(classname,l) 注册
//定义Constructor函数,定义GC函数，定义注册函数，构造参数调用注册函数
#define LUA_CLASS_EXIST_BEGIN(CLASSNAME)\
		LUA_CLASS_FNC_CONSTRUCTOR_DEFINE(CLASSNAME,LUA_EXPORT_METATABLE(CLASSNAME))\
		LUA_CLASS_FNC_GC_DEFINE(CLASSNAME,LUA_EXPORT_METATABLE(CLASSNAME))\
		static void LuaClassRegister_##CLASSNAME(lua_State *L) {\
			app::lua::ClassRegSet rs;rs.gc_fnc=&LUA_FNC_NAME_GC(CLASSNAME);rs.metatable = LUA_EXPORT_METATABLE(CLASSNAME);\
			rs.cons_name= LUA_EXPORT_METATABLE(CLASSNAME);rs.cons_fnc=&LUA_FNC_NAME_CONSTRUCTOR(CLASSNAME);\

//修改/派生类，类中使用 LUA_CLASS_EXPORT_DECLARE 申明
//使用 LUA_CLASS_EXPORT_REG(classname,l) 注册，或在任意地方使用 CLASSNAME::LuaClassRegister(l) 注册
//定义Constructor函数,定义GC函数，定义注册函数，构造参数调用注册函数
#define LUA_CLASS_EXPORT_BEGIN(CLASSNAME)\
		LUA_CLASS_FNC_CONSTRUCTOR_DEFINE(CLASSNAME,LUA_EXPORT_METATABLE(CLASSNAME))\
		LUA_CLASS_FNC_GC_DEFINE(CLASSNAME,LUA_EXPORT_METATABLE(CLASSNAME))\
		void CLASSNAME::LuaClassRegister(lua_State *L) {\
			app::lua::ClassRegSet rs;rs.gc_fnc=&LUA_FNC_NAME_GC(CLASSNAME);rs.metatable = LUA_EXPORT_METATABLE(CLASSNAME);\
			rs.cons_name= LUA_EXPORT_METATABLE(CLASSNAME);rs.cons_fnc=&LUA_FNC_NAME_CONSTRUCTOR(CLASSNAME);\

//自定义注册，可以自定义tablename,构造函数名(支持.分割)，构造函数(需要已经定义，且函数必须返回btable  )
//使用 LUA_CLASS_CUSTOMER_REG(classname,l) 注册，可在lua里通过CONSFUNC_NAME()构造
//定义GC函数，构造参数调用注册函数
#define LUA_CLASS_CUSTOMER_BEGIN(CLASSNAME,TABLENAME,CONSFUNC_NAME,CONSFNC)\
		LUA_CLASS_FNC_GC_DEFINE(CLASSNAME,TABLENAME)\
		void LuaClassRegister_##CLASSNAME(lua_State *L) {\
			using RTType =typename base::function_traits<decltype(CONSFNC)>::rt_type;\
			static_assert(std::is_same<RTType,app::lua::btable>::value,"construct function must return btable");\
			app::lua::ClassRegSet rs;rs.gc_fnc=&LUA_FNC_NAME_GC(CLASSNAME);rs.metatable = TABLENAME;\
			rs.cons_pcb = app::lua::CreatePCB(L, CONSFNC, CONSFUNC_NAME);\

//成员函数为类成员函数
#define LUA_MEM_FUNC(NAME,MEMFUNC) 	rs.funcs.push_back(app::lua::CreateClassPCB(L, MEMFUNC, NAME));

//成员函数未其它普通函数，注:第一个参数必须为 void *,然后在函数内转换为 CLASS **
#define LUA_OTHER_FUNC(NAME,FUNC) rs.funcs.push_back(app::lua::CreatePCB(L, FUNC, NAME));

#define LUA_CLASS_END()  app::lua::ExportClassToLua(L,rs);}		
	}
}
