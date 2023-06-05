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
		//���ݸ�lua��mtable�����������µ�userdata,����Ϊpointer,������Ԫ��table_name��һ�����ڰ���c++���洴���Ķ��󣬴��ݸ�luaʹ��
		//��lua��ȡ�� void *(����ֵ/����) Ϊ userdata �� lightuserdata����Ҫ���ݾ��庯��ת�� ���Ϊ��������T����ת��Ϊ T**
		//PS:������mtable��ʽ���ݸ�lua��userdata������lua�����ڴ棬�����p����������
		// 1.���p��c++�����������lua������Ȼ���δ��봩��������ԭʼ������userdata����ʱ���� 
		// 2.���p��c++���봴��������p��userdata����ʱ��p����������
		struct mtable {
			mtable(void *p, const char *name) :pointer(p),table_name(name){}
			void *pointer;
			const char *table_name;//һ��Ϊ����
		};

		//�����󶨵Ķ��� bind-table
		//���ݸ�lua��btable������pointer�����Ű󶨵�userdataһ��������������pointer�´������������
		struct btable {
			btable(void *p, const char *name):mt(p,name) {}
			mtable mt;
		};

		
		//Lua��������
		struct LuaArg {
			//set ѹջ��lua 1.����lua����ʱѹջ������lua����  2.lua����c++�󷵻ظ�lua���
			//��֧�ֵİ汾��ƥ��ɹ���k = void
			template <typename T, typename K = typename std::enable_if<!std::is_integral<T>::value>::type>
			inline static int set(lua_State *ls, T ,K *k=nullptr) {
				//��֧�ֵĲ�����������ԣ���lua����ȡ��һ��nil(��������/����ֵ)
#ifndef LUA_IGNORE_UNSUPPORT_TYPE				
				static_assert(false, "LuaArg::set type not support,use mtable or btable? or define LUA_IGNORE_UNSUPPORT_TYPE to ingore.");
#else
				lua_pushnil(ls);
#endif
				return 1;
			}

			//integer �汾���������� bool ,���� unsigned
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
			inline static int set(lua_State *ls, T arg) {
				// lua_checkstackȷ����ջ��������"n"�������λ�����費�ܰѶ�ջ��չ����Ӧ�ĳߴ磬��������"false"��
				// ʧ�ܵ�ԭ���������ջ��չ���ȹ̶����ߴ绹�������Ǽ�ǧ��Ԫ�أ�������ڴ�ʧ�ܡ�
				// lua ջ�Ѿ��㹻�󣬲��ü��
				//if (lua_checkstack(ls, 1))
				lua_pushinteger(ls,(lua_Integer) arg);
				return 1;
			}

			inline static int set(lua_State *ls, bool arg) {
				lua_pushboolean(ls, arg?1:0);//��������bool������lua��integer 0Ϊ��
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
			//flag:�Ƿ��� userdata ���� mt.pointer
			static int set_metatable(lua_State *ls, const mtable &mt, char flag);
			inline static int set(lua_State *ls, const mtable &mt) {
				return set_metatable(ls, mt, 0);
			}

			inline static int set(lua_State *ls,const btable &bt){
				return set_metatable(ls, bt.mt, 1);
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
				arg = (bool)lua_toboolean(states, index);//���lua����true�� luaL_checkinteger �ᱨ���ʹ���
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

			//ֻ�п�ת�������εĻ�ƥ�䵽�ú��������� bool,char ���Լ����� unsigned �汾
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
			static void get(lua_State *states,int idx, T &arg) {
				arg = (T)luaL_checkinteger(states, idx);
			}

			//��ȡָ�����:����ʹ�� void *�����︨���� lightuserdataת��Ϊָ�룬���lua���ݵ���userdata����Ҫ����ת��
			//set����Ҫ������Ϊ��ȷ������Ϊʲô����Ҫ�ֶ�Ϊ void *(light) ���� mtable/btable
			template<typename T,bool V>
			struct UnKnowPointerHelper {
				static void get(lua_State *, int, T &){}
			};

			template<typename T>
			struct UnKnowPointerHelper<T,true> {
				static void get(lua_State *ls, int idx, T& v) {
					LuaArg::get(ls, idx, (void*&)v);
				}
			};


			//��֧�ֵİ汾��ƥ��ɹ���k = void
			template <typename T, typename K = typename std::enable_if<!std::is_integral<T>::value>::type>
			static void get(lua_State *l, int i, T& t,K *k= nullptr ) {
				//δ֪���ͣ�������ԣ���c++��ΪĬ��ֵ
#ifndef LUA_IGNORE_UNSUPPORT_TYPE
				static_assert(false, "LuaArg::get type not support,define LUA_IGNORE_UNSUPPORT_TYPE to ingore?");
#else
				UnKnowPointerHelper<T, std::is_pointer<T>::value>::get(l, i, t);
#endif
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
			std::string m_err;//���Ĵ���

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
					constexpr auto sizeVar = (int)sizeof...(args);
					//auto topCheck = lua_gettop(m_ls);//����ջ��ԭʼ��������Ϊջ����1�����Ծ��ǵ�ǰջ��С
					LuaArg::setargs(m_ls,args...);
					//auto pushedVar = lua_gettop(m_ls) - topCheck;//��������
					//if (pushedVar == (int)sizeVar) {//�������
						const int rtValueCount = ReturnParamCounts<_Result>::value;
						if (lua_pcall(m_ls, sizeVar, rtValueCount, 0) == 0) {
							if (rtValueCount > 0){
								LuaArg::get(m_ls,-1,result);
							}
						}
						else {
							catch_lasterr();
						}
					//}
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
							constexpr auto sizeVar = sizeof...(args);
							//auto topCheck = lua_gettop(m_ls);//��ǰջ��
							LuaArg::setargs(m_ls,args...);

							//auto pushedVar = lua_gettop(m_ls) - topCheck;//��������

							//if (pushedVar == (int)sizeVar) {//�������

								if (lua_pcall(m_ls, sizeVar, rtValueCount, 0) == 0) {
									if (rtValueCount > 0)
									{
										LuaArg::get(m_ls, -1, result);
									}
								}
								else {
									catch_lasterr();
								}
							//}
						}
					}
					else if (rtValueCount > 0) {
						LuaArg::get(m_ls, -1, result);
					}
				}
				else {
					catch_lasterr();
				}
				lua_settop(m_ls, top); // resume stack		
				return result;
			}

			void catch_lasterr() {//��ȡ��������Ϣ
				LuaArg::get(m_ls,-1, m_err);
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
				return RunString_x<RtType>(str, strFunc, args...);
			}

			//�����ļ�
			bool LoadFile(const char *file);
			//���ز������ļ�
			bool DoFile(const char *file);
			//��ȡ���Ĵ�����Ϣ
			const std::string &GetLastError() const {
				return m_err;
			}
			//���������Ϣ
			void ClearError() {
				m_err.clear();
			}
		};
	}
}