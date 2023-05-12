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
		//͵���� ���ղ����ýṹ����
		struct emp {};

		//���ݸ�lua��void *��������Ϊlightuserdata
		//���ݸ�lua��mtable�����������µ�userdata,������Ԫ��table_name��һ�����ڰ���c++���洴���Ķ��󣬴��ݸ�luaʹ�ã������Ѿ���lua�ﵼ������
		//��lua��ȡ�� void *(����ֵ/����) Ϊ userdata �� lightuserdata����Ҫ���ݾ��庯��ת�� ���Ϊ��������T����ת��Ϊ T**
		//PS:������mtable��ʽ���ݸ�lua��userdata������lua�����ڴ棬�����p����������
		// 1.���p��c++�����������lua������Ȼ���δ��봩��������ԭʼ������userdata����ʱ���� 
		// 2.���p��c++���봴��������p��userdata����ʱ��p����������
		struct mtable {
			mtable(void *p, const char *name) :pointer(p),table_name(name){}
			void *pointer;
			const char *table_name;//һ��Ϊ����
		};

		
		//Lua��������
		struct LuaArg {
			//set ѹջ��lua 1.����lua����ʱѹջ������lua����  2.lua����c++�󷵻ظ�lua���
			inline static int set(lua_State *ls, bool arg) {
				// lua_checkstackȷ����ջ��������"n"�������λ�����費�ܰѶ�ջ��չ����Ӧ�ĳߴ磬��������"false"��
				// ʧ�ܵ�ԭ���������ջ��չ���ȹ̶����ߴ绹�������Ǽ�ǧ��Ԫ�أ�������ڴ�ʧ�ܡ�
				// lua ջ�Ѿ��㹻�󣬲��ü��
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

			//ע�� set �� setargs
			inline static int setargs(lua_State *ls) { return 0; }//0�������汾

			template <typename T>
			static int set(lua_State *, T) {
				static_assert(/*std::is_integral<T>::value*/false, "set:arg type not support: [bool,int,long long,size_t,float,double,const char * ,void *,std::string,std::tuple");
				return 0;
			}

			template<typename Ty, typename...Args>
			inline static int setargs(lua_State *ls, Ty &&arg1, Args&&...args) {
				auto rt=set(ls, arg1);//���������汾����
				rt+= setargs(ls, args...);//���args��ʣ��һ��������������������������Լ�
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

			//get ϵ�� 1.c++����lua���ȡlua�ķ���ֵ    2.lua����c++ʱ����ȡlua�������Ĳ���
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
			static void get(lua_State *ls, int idx, emp &) {//�ս��
			}
			static void getargs(lua_State *states, int index) {}//0�������汾

			template <typename T>
			static void get(lua_State *,int, T) {
				static_assert(/*std::is_integral<T>::value*/false, "get:arg type not support: [bool,int,long long,size_t,float,double,const char * ,void *,std::string,std::tuple");
			}

			template<typename Ty, typename...Args> inline
				static void getargs(lua_State *states, int index, Ty& arg1, Args&...args)
			{
				get(states, index, arg1);//������������
				getargs(states, index + 1, args...);//���args��ʣ��һ��������������������������Լ�
			}

			template<typename ...Args, size_t... indexs>
			static void get_tuple(lua_State *ls, std::tuple<Args...> &v, const std::index_sequence<indexs... > &) {
				auto p_size = (int)sizeof...(Args);//������1�����Ǵ� -1��ȡ��2������ -2���Դ�����
				getargs(ls, -p_size, std::get<indexs>(v)...);
			}

			//�������ܲ����������������Ҫ����ʹ����/�ṹ
			template<typename ...Args>
			static void get(lua_State *ls, int idx , std::tuple<Args...> &r) {
				constexpr auto rt_size = (int)sizeof...(Args);// std::tuple_size<std::tuple<T...>>::value;
				using  typeindex = std::make_index_sequence<rt_size>;
				get_tuple(ls, r, typeindex{});
			}
		};

		
		//�����װ����������LUA�ű���ĺ���
		//virtual public lua::VM 
		class LuaCaller :public lua::VM {			
		protected:
			//ȷ�Ϻ�������ѹջ
			bool luax_assume_func(const char* func);

			//����Return�����Ƶ�����ֵ����
			template<typename T>
			struct ReturnParamCounts {
				static constexpr int value = 1;//std::is_same<T, emp>::value ? 0 : 1;
			};

			//����Ϊemp���޷���ֵ
			template<>
			struct ReturnParamCounts<emp> {
				static constexpr int value = 0;
			};

			//����tuple
			template<typename ...T>
			struct ReturnParamCounts<std::tuple<T...>> {
				static constexpr int value = sizeof...(T);
			};
			
			//���ú���
			template<typename _Result, typename..._Args>
			_Result Call_x(const char *func, const _Args&...args) {
				_Result result{};
				auto top = lua_gettop(m_ls); // store stack
				if (luax_assume_func(func)) {
					auto sizeVar = sizeof...(args);
					auto topCheck = lua_gettop(m_ls);//����ջ��ԭʼ��������Ϊջ����1�����Ծ��ǵ�ǰջ��С
					LuaArg::setargs(m_ls,args...);
					auto pushedVar = lua_gettop(m_ls) - topCheck;//��������
					if (pushedVar == (int)sizeVar) {//�������
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
				//��� strFunc ��Ϊ�գ��򷵻�ֵ�ǵ��� strFunc�ķ���ֵ
				bool paramForInner = (strFunc && *strFunc);
				if (luaL_loadstring(m_ls, str) == 0 && lua_pcall(m_ls, 0, (paramForInner?0:rtValueCount), 0) == 0)//����ִ���ַ���
				{
					if (strFunc!=nullptr && *strFunc) {//��������к�����Ҫ����
						if (luax_assume_func(strFunc))
						{
							auto sizeVar = sizeof...(args);
							auto topCheck = lua_gettop(m_ls);//��ǰջ��
							LuaArg::setargs(m_ls,args...);

							auto pushedVar = lua_gettop(m_ls) - topCheck;//��������

							if (pushedVar == (int)sizeVar) {//�������

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
			//���ⷵ��ֵ�汾
			//������ò���Ϊvoid *,��ѹջΪ lightuserdata
			//�������Ϊ const mtable &,ѹջΪuserdata
			//�������ֵΪ void *,��lua�ɷ��� lightuserdata �� userdata
			template<typename _Result, typename..._Args>
			typename std::conditional<std::is_same<_Result, void>::value, lua::emp, _Result>::type Call(const char*  func, const _Args&...args) {
				//����return void;������ void = void;
				using RtType = typename std::conditional<std::is_same<_Result, void>::value, lua::emp, _Result>::type;
				return Call_x<RtType>(func, args...);
			}

			//���ⷵ��ֵ�汾��ע�⣺���strFunc��Ϊ�գ��򷵻�ֵ��strFunc�ķ���ֵ������Ϊstr����ķ���ֵ
			template<typename _Result, typename..._Args> 
			typename std::conditional<std::is_same<_Result, void>::value, lua::emp, _Result>::type  RunString(const char  *str, const char *strFunc = nullptr, const _Args&...args) {
				//����return void;
				using RtType = typename std::conditional<std::is_same<_Result, void>::value, lua::emp, _Result>::type;
				return RunString_x<emp>(str, strFunc, args...);
			}

			//�����ļ�
			bool LoadFile(const char *file);
			//���ز������ļ�
			bool DoFile(const char *file);
		};
	}
}