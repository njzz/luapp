#pragma once
#include <string>
#include <assert.h>
#include <tuple>
#include "LuaVM.h"

//author:njzz
//github:https://github.com/njzz/luapp
//csdn:https://blog.csdn.net/m0_66399932

namespace app {
	namespace lua {
		//偷下懒 ，空参数用结构代替
		struct emp {};

		//传递给lua的void *参数，作为lightuserdata
		//传递给lua的mtable参数，创建新的userdata,并设置元表table_name，一般用于把在c++里面创建的对象，传递给lua使用，并且已经在lua里导出该类
		//从lua获取的 void *(返回值/参数) 为 userdata 或 lightuserdata，需要根据具体函数转换 如果为导出对象T，则转换为 T**
		//PS:所有以mtable方式传递给lua的userdata对象，由lua管理内存，里面的p不参与析构
		// 1.如果p是c++导出类对象，在lua创建，然后多次传入穿出，则由原始关联的userdata析构时析构 
		// 2.如果p是c++代码创建，包裹p的userdata析构时，p不参与析构
		struct mtable {
			mtable(void *p, const char *name) :pointer(p),table_name(name){}
			void *pointer;
			const char *table_name;//一般为类名
		};

		
		//Lua参数操作
		struct LuaArg {
			//set 压栈给lua 1.调用lua函数时压栈参数给lua函数  2.lua调用c++后返回给lua结果
			inline static int set(lua_State *ls, bool arg) {
				// lua_checkstack确保堆栈上至少有"n"个额外空位。假设不能把堆栈扩展到相应的尺寸，函数返回"false"。
				// 失败的原因包括将把栈扩展到比固定最大尺寸还大（至少是几千个元素）或分配内存失败。
				// lua 栈已经足够大，不用检查
				//if (lua_checkstack(ls, 1))
				lua_pushboolean(ls, arg?1:0);
				return 1;
			}
			inline static int set(lua_State *ls, int arg) {
				lua_pushinteger(ls, arg);
				return 1;
			}
			inline static int set(lua_State *ls, long long arg) {
				lua_pushinteger(ls, arg);
				return 1;
			}
			inline static int set(lua_State *ls, size_t arg) {
				lua_pushinteger(ls, arg);
				return 1;
			}
			inline static int set(lua_State *ls, float arg) {
				lua_pushnumber(ls, arg);
				return 1;
			}
			inline static int set(lua_State *ls, double arg) {
				lua_pushnumber(ls, arg);
				return 1;
			}
			inline static int set(lua_State *ls, const char* arg) {
				lua_pushstring(ls, arg);
				return 1;
			}
			inline static int set(lua_State *ls, const std::string& arg) {
				lua_pushlstring(ls, arg.c_str(), arg.length());
				return 1;
			}

			static int set_metatable(lua_State *ls, const mtable &mt, char flag);
			inline static int set(lua_State *ls, const mtable &mt) {
				return set_metatable(ls, mt, 0);
			}

			inline static int set(lua_State *ls, void* arg) {
				lua_pushlightuserdata(ls, arg);
				return 1;
			}
			inline static int set(lua_State *ls, nullptr_t arg) {
				lua_pushnil(ls);
				return 1;
			}

			//注意 set 和 setargs
			inline static int setargs(lua_State *ls) { return 0; }//0个参数版本

			template <typename T>
			static int set(lua_State *, T) {
				static_assert(/*std::is_integral<T>::value*/false, "set:arg type not support: [bool,int,long long,size_t,float,double,const char * ,void *,std::string,std::tuple");
				return 0;
			}

			template<typename Ty, typename...Args>
			inline static int setargs(lua_State *ls, Ty &&arg1, Args&&...args) {
				auto rt=set(ls, arg1);//调用其它版本函数
				rt+= setargs(ls, args...);//如果args还剩下一个，调用其它函数，否则调用自己
				return rt;
			}

			template<typename ...Args, size_t... indexs>
			static void set_tuple(lua_State *ls, const std::tuple<Args...> &v, const std::index_sequence<indexs... > &) {
				setargs(ls, std::get<indexs>(v)...);
			}

			template<typename ...Args>
			static int set(lua_State *ls, const std::tuple<Args...> &v) {
				constexpr auto rt_size = sizeof...(Args);//(int)std::tuple_size<std::tuple<Args...>>::value;
				using  typeindex = std::make_index_sequence<rt_size>;
				set_tuple(ls, v, typeindex{});
				return rt_size;
			}

			//get 系列 1.c++调用lua后获取lua的返回值    2.lua调用c++时，获取lua传过来的参数
			static void get(lua_State *states, int index, bool &arg) {
				//lua_toboolean
				arg = luaL_checkinteger(states, index) != 0;
			}
			static void get(lua_State *states, int index, int &arg) {
				arg = (int)luaL_checkinteger(states, index);
			}
			static void get(lua_State *states, int index, long long &arg) {
				arg = (long long)luaL_checkinteger(states, index);
			}
			static void get(lua_State *states, int index, size_t &arg) {
				arg = (size_t)luaL_checkinteger(states, index);
			}
			static void get(lua_State *states, int index, float &arg) {
				arg = (float)luaL_checknumber(states, index);
			}
			static void get(lua_State *states, int index, double &arg) {
				arg = (double)luaL_checknumber(states, index);
			}
			static void get(lua_State *states, int index, const char* &arg) {
				arg = (const char *)luaL_checkstring(states, index);
			}
			static void get(lua_State *states, int index, void* &arg) {
				arg = (void *)lua_touserdata(states, index);
			}
			static void get(lua_State *states, int index, std::string &arg) {
				size_t t = 0;
				auto sr = luaL_checklstring(states, index, &t);
				if (sr && t > 0) arg.assign(sr, t);
			}
			static void get(lua_State *ls, int idx, emp &) {//空结果
			}
			static void getargs(lua_State *states, int index) {}//0个参数版本

			template <typename T>
			static void get(lua_State *,int, T) {
				static_assert(/*std::is_integral<T>::value*/false, "get:arg type not support: [bool,int,long long,size_t,float,double,const char * ,void *,std::string,std::tuple");
			}

			template<typename Ty, typename...Args> inline
				static void getargs(lua_State *states, int index, Ty& arg1, Args&...args)
			{
				get(states, index, arg1);//调用其它函数
				getargs(states, index + 1, args...);//如果args还剩下一个，调用其它函数，否则调用自己
			}

			template<typename ...Args, size_t... indexs>
			static void get_tuple(lua_State *ls, std::tuple<Args...> &v, const std::index_sequence<indexs... > &) {
				auto p_size = (int)sizeof...(Args);//个数，1个就是从 -1获取，2个就是 -2，以此类推
				getargs(ls, -p_size, std::get<indexs>(v)...);
			}

			//函数不能部分特例化，如果需要可以使用类/结构
			template<typename ...Args>
			static void get(lua_State *ls, int idx , std::tuple<Args...> &r) {
				constexpr auto rt_size = (int)sizeof...(Args);// std::tuple_size<std::tuple<T...>>::value;
				using  typeindex = std::make_index_sequence<rt_size>;
				get_tuple(ls, r, typeindex{});
			}
		};

		
		//该类封装，用来调用LUA脚本里的函数
		//virtual public lua::VM 
		class LuaCaller :public lua::VM {			
		protected:
			//确认函数，并压栈
			bool luax_assume_func(const char* func);

			//根据Return类型推导返回值个数
			template<typename T>
			struct ReturnParamCounts {
				static constexpr int value = 1;//std::is_same<T, emp>::value ? 0 : 1;
			};

			//返回为emp，无返回值
			template<>
			struct ReturnParamCounts<emp> {
				static constexpr int value = 0;
			};

			//返回tuple
			template<typename ...T>
			struct ReturnParamCounts<std::tuple<T...>> {
				static constexpr int value = sizeof...(T);
			};
			
			//调用函数
			template<typename _Result, typename..._Args>
			_Result Call_x(const char *func, const _Args&...args) {
				_Result result{};
				auto top = lua_gettop(m_ls); // store stack
				if (luax_assume_func(func)) {
					auto sizeVar = sizeof...(args);
					auto topCheck = lua_gettop(m_ls);//返回栈顶原始索引，因为栈底是1，所以就是当前栈大小
					LuaArg::setargs(m_ls,args...);
					auto pushedVar = lua_gettop(m_ls) - topCheck;//参数个数
					if (pushedVar == (int)sizeVar) {//参数检查
						const int rtValueCount = ReturnParamCounts<_Result>::value;
						if (lua_pcall(m_ls, pushedVar, rtValueCount, 0) == 0) {
							if (rtValueCount > 0){
								LuaArg::get(m_ls,-1,result);
							}
						}
						//else {//call error,info:lua_tostring(m_ls, -1)
						//}
					}
				}
				lua_settop(m_ls, top); // resume stack
				return result;
			}

			template<typename _Result, typename..._Args>
			_Result RunString_x(const char *str, const char *strFunc = nullptr, const _Args&...args) {
				_Result result;

				auto top = lua_gettop(m_ls); // store stack
				int rtValueCount = ReturnParamCounts<_Result>::value;
				//如果 strFunc 不为空，则返回值是调用 strFunc的返回值
				bool paramForInner = (strFunc && *strFunc);
				if (luaL_loadstring(m_ls, str) == 0 && lua_pcall(m_ls, 0, (paramForInner?0:rtValueCount), 0) == 0)//解析执行字符串
				{
					if (strFunc!=nullptr && *strFunc) {//如果里面有函数需要调用
						if (luax_assume_func(strFunc))
						{
							auto sizeVar = sizeof...(args);
							auto topCheck = lua_gettop(m_ls);//当前栈顶
							LuaArg::setargs(m_ls,args...);

							auto pushedVar = lua_gettop(m_ls) - topCheck;//参数个数

							if (pushedVar == (int)sizeVar) {//参数检查

								if (lua_pcall(m_ls, pushedVar, rtValueCount, 0) == 0) {
									if (rtValueCount > 0)
									{
										LuaArg::get(m_ls, -1, result);
									}
								}
								//else {//call error,info:lua_tostring(m_ls, -1)
								//}
							}
						}
					}
					else if (rtValueCount > 0) {
						LuaArg::get(m_ls, -1, result);
					}
				}
				lua_settop(m_ls, top); // resume stack		
				return result;
			}

		public:
			//任意返回值版本
			//如果调用参数为void *,则压栈为 lightuserdata
			//如果参数为 const mtable &,压栈为userdata
			//如果返回值为 void *,则lua可返回 lightuserdata 或 userdata
			template<typename _Result, typename..._Args>
			typename std::conditional<std::is_same<_Result, void>::value, lua::emp, _Result>::type Call(const char*  func, const _Args&...args) {
				//可以return void;但不能 void = void;
				using RtType = typename std::conditional<std::is_same<_Result, void>::value, lua::emp, _Result>::type;
				return Call_x<RtType>(func, args...);
			}

			//任意返回值版本，注意：如果strFunc不为空，则返回值是strFunc的返回值，否则为str本身的返回值
			template<typename _Result, typename..._Args> 
			typename std::conditional<std::is_same<_Result, void>::value, lua::emp, _Result>::type  RunString(const char  *str, const char *strFunc = nullptr, const _Args&...args) {
				//可以return void;
				using RtType = typename std::conditional<std::is_same<_Result, void>::value, lua::emp, _Result>::type;
				return RunString_x<emp>(str, strFunc, args...);
			}

			//加载文件
			bool LoadFile(const char *file);
			//加载并运行文件
			bool DoFile(const char *file);
		};
	}
}