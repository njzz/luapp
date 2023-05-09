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
		//该类封装，用来调用LUA脚本里的函数
		//virtual public lua::VM 
		class LuaCaller :public lua::VM {			
		protected:
			//调用lua，压入参数
			void pusharg(int arg);
			void pusharg(long long arg);
			void pusharg(size_t arg);
			void pusharg(float arg);
			void pusharg(double arg);
			void pusharg(const char* arg);
			void pusharg(const std::string& arg);
			void pusharg(void* arg);
			void pusharg(nullptr_t arg);
			void pusharg() {}//0个参数版本


			template<typename _Ty, typename..._Args> inline
				void pusharg(_Ty arg1, const _Args&...args)
			{
				pusharg(arg1);//调用其它函数
				pusharg(args...);//如果args还剩下一个，调用其它函数，否则调用自己
			}

			//确认函数，并压栈
			bool luax_assume_func(const char* func);

			//调用lua，获取返回值(做个返回tuple的，函数不能部分特例化，使用结构)
			template<typename _Ty>
			struct LuaCallRt {
				static _Ty get(lua_State *ls) {
					//return luax_getretval<_Ty>();
					static_assert(false, "return type not int support list: [bool,int,long long,size_t,float,double,std::string,std::tuple");
				}
			};

			template<> struct LuaCallRt<bool> {
				static bool get(lua_State *ls,int idx = -1) {
					bool r = false;
					if (lua_isboolean(ls, idx)) {						
						r = lua_toboolean(ls, idx)!=0;
					}
					return false;
				}
			};

			template<> struct LuaCallRt<int> {
				static int get(lua_State *ls, int idx = -1) {
					int r = 0;
					if (lua_isinteger(ls, idx)) {
						r = (int)lua_tointeger(ls, idx);
					}
					return r;
				}
			};

			template<> struct LuaCallRt<long long> {
				static long long get(lua_State *ls, int idx = -1) {
					long long r = 0;
					if (lua_isinteger(ls, idx)) {
						r = (long long)lua_tointeger(ls, idx);
					}
					return r;
				}
			};

			template<> struct LuaCallRt<size_t> {
				static size_t get(lua_State *ls, int idx = -1) {
					size_t r = 0;
					if (lua_isinteger(ls, idx)) {
						r = (size_t)lua_tointeger(ls, idx);
					}
					return r;
				}
			};

			template<> struct LuaCallRt<float> {
				static float get(lua_State *ls, int idx = -1) {
					float r = 0;
					if (lua_isnumber(ls, idx)) {
						r = (float)lua_tonumber(ls, idx);
					}
					return r;
				}
			};

			template<> struct LuaCallRt<double> {
				static double get(lua_State *ls, int idx = -1) {
					double r = 0;
					if (lua_isnumber(ls, idx)) {
						r = lua_tonumber(ls, idx);
					}
					return r;
				}
			};

			template<> struct LuaCallRt<std::string> {
				static std::string get(lua_State *ls, int idx = -1) {
					std::string r;
					if (lua_isstring(ls, idx)) {
						size_t l=0;
						auto sr = lua_tolstring(ls, idx,&l);
						if (sr && l > 0) r.assign(sr, l);
					}
					return r;
				}
			};

			//空参数版本
			template<> struct LuaCallRt<emp> {
				static emp get(lua_State *ls, int idx = -1) {
					return emp{};
				}
			};

			//返回多个结果，用tuple，函数不能部分特例化，只能使用类/结构
			template<typename ...T>
			struct LuaCallRt<std::tuple<T...>>{
				
				//返回tuple，无参数版本
				static inline void r_tuple(lua_State *ls,int idx) {}

				//解tuple，1个至多个参数版本
				template<typename _Ty, typename..._Args>
				static inline void r_tuple(lua_State *ls,int idx,_Ty& arg1, _Args&...args)
				{
					arg1 = LuaCallRt<_Ty>::get(ls,idx);
					r_tuple(ls,idx+1,args...);//如果args还剩下一个，调用其它函数，否则调用自己
				}

				template<typename ...T, size_t... indexs>
				static void r_tuple_helper(lua_State *ls,std::tuple<T...> &v, const std::index_sequence<indexs... > &) {
					auto p_size = (int)sizeof...(T);//返回值个数，1个就是从 -1获取，2个就是 -2，以此类推
					r_tuple(ls,-p_size,std::get<indexs>(v)...);
				}

				static std::tuple<T...> get(lua_State *ls, int idx = -1) {
					constexpr auto rt_size = (int)sizeof...(T);// std::tuple_size<std::tuple<T...>>::value;
					using  typeindex = std::make_index_sequence<rt_size>;
					std::tuple<T...> rt;
					r_tuple_helper(ls,rt, typeindex{});
					return rt;
				}
			};

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
					pusharg(args...);
					auto pushedVar = lua_gettop(m_ls) - topCheck;//参数个数
					if (pushedVar == (int)sizeVar) {//参数检查
						const int rtValueCount = ReturnParamCounts<_Result>::value;
						if (lua_pcall(m_ls, pushedVar, rtValueCount, 0) == 0) {
							if (rtValueCount > 0)
							{
								result = LuaCallRt<_Result>::get(m_ls);
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
							pusharg(args...);

							auto pushedVar = lua_gettop(m_ls) - topCheck;//参数个数

							if (pushedVar == (int)sizeVar) {//参数检查

								if (lua_pcall(m_ls, pushedVar, rtValueCount, 0) == 0) {
									if (rtValueCount > 0)
									{
										result = LuaCallRt<_Result>::get(m_ls);
									}
								}
								//else {//call error,info:lua_tostring(m_ls, -1)
								//}
							}
						}
					}
					else if (rtValueCount > 0) {
						result = LuaCallRt<_Result>::get(m_ls);
					}
				}
				lua_settop(m_ls, top); // resume stack		
				return result;
			}

		public:
			//任意返回值版本
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