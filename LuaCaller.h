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
		//�����װ����������LUA�ű���ĺ���
		//virtual public lua::VM 
		class LuaCaller :public lua::VM {			
		protected:
			//����lua��ѹ�����
			void pusharg(int arg);
			void pusharg(long long arg);
			void pusharg(size_t arg);
			void pusharg(float arg);
			void pusharg(double arg);
			void pusharg(const char* arg);
			void pusharg(const std::string& arg);
			void pusharg(void* arg);
			void pusharg(nullptr_t arg);
			void pusharg() {}//0�������汾


			template<typename _Ty, typename..._Args> inline
				void pusharg(_Ty arg1, const _Args&...args)
			{
				pusharg(arg1);//������������
				pusharg(args...);//���args��ʣ��һ��������������������������Լ�
			}

			//ȷ�Ϻ�������ѹջ
			bool luax_assume_func(const char* func);

			//����lua����ȡ����ֵ(��������tuple�ģ��������ܲ�����������ʹ�ýṹ)
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

			//�ղ����汾
			template<> struct LuaCallRt<emp> {
				static emp get(lua_State *ls, int idx = -1) {
					return emp{};
				}
			};

			//���ض���������tuple���������ܲ�����������ֻ��ʹ����/�ṹ
			template<typename ...T>
			struct LuaCallRt<std::tuple<T...>>{
				
				//����tuple���޲����汾
				static inline void r_tuple(lua_State *ls,int idx) {}

				//��tuple��1������������汾
				template<typename _Ty, typename..._Args>
				static inline void r_tuple(lua_State *ls,int idx,_Ty& arg1, _Args&...args)
				{
					arg1 = LuaCallRt<_Ty>::get(ls,idx);
					r_tuple(ls,idx+1,args...);//���args��ʣ��һ��������������������������Լ�
				}

				template<typename ...T, size_t... indexs>
				static void r_tuple_helper(lua_State *ls,std::tuple<T...> &v, const std::index_sequence<indexs... > &) {
					auto p_size = (int)sizeof...(T);//����ֵ������1�����Ǵ� -1��ȡ��2������ -2���Դ�����
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
					pusharg(args...);
					auto pushedVar = lua_gettop(m_ls) - topCheck;//��������
					if (pushedVar == (int)sizeVar) {//�������
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
				//��� strFunc ��Ϊ�գ��򷵻�ֵ�ǵ��� strFunc�ķ���ֵ
				bool paramForInner = (strFunc && *strFunc);
				if (luaL_loadstring(m_ls, str) == 0 && lua_pcall(m_ls, 0, (paramForInner?0:rtValueCount), 0) == 0)//����ִ���ַ���
				{
					if (strFunc!=nullptr && *strFunc) {//��������к�����Ҫ����
						if (luax_assume_func(strFunc))
						{
							auto sizeVar = sizeof...(args);
							auto topCheck = lua_gettop(m_ls);//��ǰջ��
							pusharg(args...);

							auto pushedVar = lua_gettop(m_ls) - topCheck;//��������

							if (pushedVar == (int)sizeVar) {//�������

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
			//���ⷵ��ֵ�汾
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