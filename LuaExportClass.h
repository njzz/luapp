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
		struct LuaInvokeMemF {
			template <typename Class,typename F, typename ...Args>
			inline static int call(lua_State *states,Class *obj, F f, Args&&... args) {
				return RLUA<R>::set(states, (obj->*f)(std::forward<Args>(args)...));//包含返回值的情况
			}
		};

		//无参数版本，需要根据返回值区分，在RP里不能特化，因为set的void参数不能实例化
		template <>
		struct LuaInvokeMemF<void> {
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
			using CallParamTuple = typename base::mfunction_traits<MFunction>::param_tuple;
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
			//获取lua传过来的参数
			inline void getargs(lua_State *states, int index, bool &arg) {
				arg = luaL_checkinteger(states, index) != 0;
			}
			inline void getargs(lua_State *states, int index, int &arg) {
				arg = (int)luaL_checkinteger(states, index);
			}
			inline void getargs(lua_State *states, int index, long long &arg) {
				arg = (long long)luaL_checkinteger(states, index);
			}
			inline void getargs(lua_State *states, int index, size_t &arg) {
				arg = (size_t)luaL_checkinteger(states, index);
			}
			inline void getargs(lua_State *states, int index, float &arg) {
				arg = (float)luaL_checknumber(states, index);
			}
			inline void getargs(lua_State *states, int index, double &arg) {
				arg = (double)luaL_checknumber(states, index);
			}
			inline void getargs(lua_State *states, int index, const char* &arg) {
				arg = (const char *)luaL_checkstring(states, index);
			}
			inline void getargs(lua_State *states, int index, std::string &arg) {
				size_t t = 0;
				auto sr = luaL_checklstring(states, index, &t);
				if (sr && t > 0) arg.assign(sr, t);
			}
			inline void getargs(lua_State *states, int index) {}//0个参数版本


			template<typename _Ty, typename..._Args> inline
				void getargs(lua_State *states, int index, _Ty& arg1, _Args&...args)
			{
				getargs(states, index, arg1);//调用其它函数
				getargs(states, index + 1, args...);//如果args还剩下一个，调用其它函数，否则调用自己
			}

			template<size_t... indexs>
			int FillParamsAndCall(lua_State *states, const std::index_sequence<indexs... > &) {
				//constexpr auto param_size = std::tuple_size<CallParamTuple>::value;				
				auto obj =(ClassType **) lua_touserdata(states, 1);//使用的userdata元表方式，那么第1个参数是userdata的值
				getargs(states, 2, std::get<indexs>(m_p)...);//lua参数从2开始，用tuple存储参数
				return LuaInvokeMemF<ReturnType>::call(states,*obj, m_f, std::get<indexs>(m_p)...);//用tuple里的参数去调用c函数
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
		//////////////////// ---------------------

//在class里申明，对象需要有默认构造函数或者重新定义 LUA_EXPORT_NEWOBJ
#define LUA_CLASS_EXPORT_DECLARE \
		static void LuaClassRegister(lua_State *l);\
		static int LuaClassConstructor(lua_State* L);\
		static int LuaClassGC(lua_State* L)


//在cpp里LUA_CLASS_EXPORT_BEGIN之前重新定义此宏，可以重新定义对象xxx从lua里构造方式的方式
// 如:
// #undef LUA_EXPORT_NEWOBJ
// #define LUA_EXPORT_NEWOBJ(xxx,l) xxx *CreateByLua(lua_tointeger(l,1));
#define LUA_EXPORT_NEWOBJ(xxx,l) new xxx{};

//在cpp里LUA_CLASS_EXPORT_BEGIN之前重新定义此宏，可以重新定义对象xxx的析构的方式
// 如:
// #undef LUA_EXPORT_CLRAROBJ
// #define LUA_EXPORT_CLRAROBJ(xxx) void DestroyLuaObj(xxx *);
#define LUA_EXPORT_DESTROYOBJ(xxx) delete xxx;

 /*
 Constructor
 1.创建一个对象， 
 2.申请一个userdata，入栈  
 3.获取元表到栈顶  
 4.将栈顶元表设置为入栈userdata的元表，弹出栈顶元表 
 5.一个返回值，返回当前栈顶userdata，通过元方法调用时，该userdata为第一个参数 (index:1)

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

 */
#define LUA_CLASS_EXPORT_BEGIN(CLASSNAME)\
		int CLASSNAME::LuaClassConstructor(lua_State* L) {\
			auto obj = LUA_EXPORT_NEWOBJ(CLASSNAME,L);\
			auto a = (CLASSNAME*)lua_newuserdata(L, sizeof(void*));\
			memcpy(a, &obj, sizeof(a));\
			luaL_getmetatable(L, #CLASSNAME);\
			lua_setmetatable(L, -2);\
			return 1;\
		}\
		int CLASSNAME::LuaClassGC(lua_State* L){\
			auto obj = (CLASSNAME**)luaL_checkudata(L, -1, #CLASSNAME);\
			LUA_EXPORT_DESTROYOBJ(*obj);\
			return 0;\
		}\
		void CLASSNAME::LuaClassRegister(lua_State *L) {\
			auto top = lua_gettop(L);\
			lua_pushcfunction(L, &CLASSNAME::LuaClassConstructor);\
			lua_setglobal(L,#CLASSNAME);\
			luaL_newmetatable(L, #CLASSNAME);\
			lua_pushstring(L, "__gc");\
			lua_pushcfunction(L, &CLASSNAME::LuaClassGC);\
			lua_settable(L, -3);\

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
