#pragma once
#include "LuaCaller.h"
#include "callable.h"
#include "lockt.h"
#include <list>
#include <unordered_map>

//author:njzz
//github:https://github.com/njzz/luapp
//csdn:https://blog.csdn.net/m0_66399932

namespace app {
	namespace lua {
		////回调对象基类，派生类包含原始类型，基类不包含，方便转换/调用/保存
		struct pcb_set {
			virtual ~pcb_set() = default;
			virtual int call(lua_State *) = 0;
			//virtual bool release() = 0;

			inline const std::string &name() const {
				return m_name;
			};

			//通用回调函数
			static int LuaCallBack(lua_State* L) {
				auto pv = (pcb_set*)lua_topointer(L, lua_upvalueindex(1));
				return pv->call(L);
			}

			lua_State *m_ls;//关联的状态机
			std::string m_name;//name
		};

		//设置返回值给lua的参数
		template <typename T>
		struct RLUA {
			static int set(lua_State *, T) {
				static_assert(std::is_integral<T>::value, "return type not int support list: [bool,int,long long,size_t,float,double,char * ,std::string,std::tuple");
				return 0;
			}
		};

		template <>	struct RLUA<bool> {
			static int set(lua_State *l, bool v) {
				lua_pushboolean(l, v ? 1 : 0);
				return 1;
			}
		};

		template <>	struct RLUA<int> {
			static int set(lua_State *l, int v) {
				lua_pushinteger(l, (lua_Integer)v);
				return 1;
			}
		};

		template <>	struct RLUA<long long> {
			static int set(lua_State *l, long long v) {
				lua_pushinteger(l, (lua_Integer)v);
				return 1;
			}
		};
		template <>	struct RLUA<size_t> {
			static int set(lua_State *l, size_t v) {
				lua_pushinteger(l, (lua_Integer)v);
				return 1;
			}
		};

		template <>	struct RLUA<float> {
			static auto set(lua_State *l, float v) {
				lua_pushnumber(l, v);
				return 1;
			}
		};

		template <>	struct RLUA<double> {
			static auto set(lua_State *l, double v) {
				lua_pushnumber(l, v);
				return 1;
			}
		};

		template <>	struct RLUA<char *> {
			static auto set(lua_State *l, char *v) {
				lua_pushstring(l, v);
				return 1;
			}
		};

		template <>	struct RLUA<std::string> {
			static auto set(lua_State *l, const std::string &v) {
				lua_pushlstring(l, v.c_str(), v.length());
				return 1;
			}
		};


		//返回参数，tuple 特例化
		template <typename ...T> struct RLUA<std::tuple<T...>> {
			//解tuple，无参数版本
			static inline void tuple_deparam(lua_State *states) {}

			//解tuple，1个至多个参数版本
			template<typename _Ty, typename..._Args>
			static inline void tuple_deparam(lua_State *states, const _Ty& arg1, const _Args&...args)
			{
				RLUA<_Ty>::set(states, arg1);
				tuple_deparam(states, args...);//如果args还剩下一个，调用其它函数，否则调用自己
			}

			template<size_t... indexs>
			static void dehelper(lua_State *l, const std::tuple<T...> &v, const std::index_sequence<indexs... > &) {
				tuple_deparam(l, std::get<indexs>(v)...);
			}

			static int set(lua_State *l, const std::tuple<T...> &v) {
				constexpr auto rt_size = (int)std::tuple_size<std::tuple<T...>>::value;
				using  typeindex = std::make_index_sequence<rt_size>;
				dehelper(l, v, typeindex{});
				return rt_size;
			}
		};

		//调用并设置返回值
		template <typename R>
		struct LuaInvoke {
			template <typename T, typename ...Args>
			static int call(lua_State *states, const T &f, Args&&... args) {
				return RLUA<R>::set(states, f(std::forward<Args>(args)...));//包含返回值的情况
			}
		};

		//无参数版本，需要根据返回值区分，在RP里不能特化，因为set的void参数不能实例化
		template <>
		struct LuaInvoke<void> {
			template <typename T, typename ...Args>
			static int call(lua_State *states, const T &f, Args&&...args) {
				f(std::forward<Args>(args)...);//用tuple调用函数
				return 0;//没有返回值
			}
		};

		//调用对象
		template <typename Function>
		class LuaBinder :public pcb_set {
		public:
			using CallParamTuple = typename base::function_traits<Function>::param_tuple;
			using FuncRtType = typename base::function_traits<Function>::rt_type;

			LuaBinder(lua_State *ls, const char *pName, Function &&f) : m_f(std::move(f)) {
				m_ls=ls;
				m_name = pName;
			}
			~LuaBinder() {
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
				size_t t=0;
				auto sr = luaL_checklstring(states, index,&t);
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
				getargs(states, 1, std::get<indexs>(m_p)...);//lua参数从1开始，用tuple存储参数
				return LuaInvoke<FuncRtType>::call(states, m_f, std::get<indexs>(m_p)...);//用tuple里的参数去调用c函数
			}

			//继承函数，开始调用
			int call(lua_State *args) override {
				constexpr auto psize = std::tuple_size<CallParamTuple>::value;
				using  typeindex = std::make_index_sequence<psize>;
				return FillParamsAndCall(args, typeindex{});
			}
			//注销
			//bool release() override{
			//	//一般不需要，直接关闭状态机即可，所以不写在析构函数里
			//	if (m_ls) {
			//		lua_pushnil(m_ls);//null
			//		lua_setglobal(m_ls, m_n.c_str());//设置全局符号
			//		m_ls = nullptr;
			//	}
			//	return true;
			//}

		protected:
			Function m_f;//回调函数
			CallParamTuple m_p{};//参数
		};

		//向lua导出函数
		class LuaExportor :public lua::LuaCaller {		
		public:
			using PCBList = std::list<lua::pcb_set *>;
			using CacheList = base::lock_t<std::unordered_map<lua_State *,PCBList>>;
			using Lock_Guard = std::lock_guard<CacheList>;			

			//缓存起来?
			static CacheList &PCBCache(lua::pcb_set *pcb = nullptr) {
				static CacheList cachePCBList;
				if (pcb) {
					Lock_Guard gb(cachePCBList);
					cachePCBList[pcb->m_ls].push_back(pcb);
				}
				return cachePCBList;
			}

			//取消状态机l里注册的函数pName，不支持类(类里还存在metatable信息)			
			//如果l为空，取消所有状态机里pName的注册，每个状态机最多取消一个(函数不会重复，只会替代)
			//一般不需要取消，状态机关闭的时候FreePCB即可 (实际上取消注册并没有什么意义，实际上基本不会用到)
			//返回被取消注册的数量
			/*static int UnRegPCB(lua_State *l, const char *pName) {
				auto &map = PCBCache();
				int unreged = 0;
				auto UnRegList = [&unreged, pName](PCBList &ll) {
					for (auto it = ll.begin(); it != ll.end(); ++it) {
						auto pcb = *it;
						if (pcb->name() == pName) {
							if (pcb->release()) { //能取消注册
								delete pcb;
								ll.erase(it);//清理
							}
							else {
								++unreged;
							}
							break;//函数只有一个
						}
					}
				};

				Lock_Guard gb(map);
				if (l != nullptr) {
					auto fc = map.find(l);
					if (fc != map.end()) {
						auto &ll = fc->second;
						UnRegList(ll);
					}
				}
				else {
					for (auto &i : map) {
						UnRegList(i.second);
					}
				}
				return unreged;
			}*/

			//释放状态机l所有PCB内存:注，仅在释放完状态机后使用
			//如果为nullptr则释放所有状态机的
			static void FreePCB(lua_State *l) {
				auto &map = PCBCache();
				auto FreeList = [](PCBList &lst) {
					for (auto ptr : lst) {
						delete ptr;
					}
					//lst.clear();//不用clear了，要么整个map都clear，要么lst从map里清理掉
				};
				Lock_Guard gb(map);
				if (l == nullptr) {//释放所有的
					for (auto &ll : map) {
						FreeList(ll.second);
					}
					map.clear();
				}
				else {//释放单独虚拟机的
					auto f = map.find(l);
					if (f != map.end()) {
						FreeList(f->second);
						map.erase(f);
					}
				}
			}

			std::vector<std::string > SS(const std::string& name) {
				size_t offst = 0, end = 0;
				end = name.find_first_of('.', offst);
				std::vector<std::string > r;
				while (end != name.npos) {
					r.push_back(name.substr(offst, end - offst));
					offst = end + 1;
					end = name.find_first_of('.', offst);
				}
				r.push_back(name.substr(offst));
				return r;
			}

			bool PCBRegister( pcb_set *p) {
				auto vss = SS(p->name());
				auto sz = vss.size();
				auto curTop = lua_gettop(m_ls);
				if (vss.size() == 1) {
					//分配一个内存 lua_newuserdata
					lua_pushlightuserdata(m_ls, p);//push 一个轻量级用户数据到栈顶，lua不管理该指针内存
					lua_pushcclosure(m_ls, &pcb_set::LuaCallBack, 1);//闭包入栈，并关联栈上的1个参数(从栈顶开始)，完成后弹出关联的参数，函数内用 lua_upvalueindex 获取关联参数
					lua_setglobal(m_ls, vss[0].c_str());//设置全局符号 t[k] = v  t:全局表  k:name  v:栈顶元素闭包  完成后弹出栈顶
				}
				else {
					//table 类型 xxx.yyy.zzz
					auto curType = lua_getglobal(m_ls, vss[0].c_str());//全局，将tab入栈顶
					if (curType != LUA_TTABLE) {//第一个不是table
						assert(curType == LUA_TNIL);//如果不是table，是其它的，弹出调试警告
						lua_pop(m_ls, 1);//弹出1个元素(栈顶的null)	
						lua_newtable(m_ls);//新表到栈顶  lua_createtable(m_ls, 0, sizeof(luaL_Reg) - 1);
						lua_setglobal(m_ls, vss[0].c_str());//t[k]=v   t:全局表 k:table名  v:栈顶新表，将第一个设置为全局表   完成后弹出栈顶
						lua_getglobal(m_ls, vss[0].c_str());//表重新入栈顶
					}
					size_t index = 1;//到这里栈顶是表，从1开始，0已经是表了
					while (index < sz - 1) {//检查表，直到倒数第二个，最后一个为函数
						curType = lua_getfield(m_ls, -1, vss[index].c_str());//检查类型/重新入栈
						if (curType != LUA_TTABLE) {//不是表
							assert(curType == LUA_TNIL);//如果不是table，是其它的，弹出调试警告
							lua_pop(m_ls, 1);//弹出1个(栈顶)元素	
							lua_newtable(m_ls);//新表到栈顶 -1
							lua_setfield(m_ls, -2, vss[index].c_str());//t[k]=v 设置 设置老表(-2)的k(name)为新表(-1)，并弹出栈顶新表
							//不增加 index++ ，让表入栈
						}
						else {
							index++;
						}
					}

					//最后一个
					lua_pushlightuserdata(m_ls, p);//push 一个轻量级用户数据到栈顶	
					lua_pushcclosure(m_ls, &pcb_set::LuaCallBack, 1);//闭包入栈，并关联栈上的1个参数(从栈顶开始)，完成后弹出关联的参数，函数内用 lua_upvalueindex 获取关联参数
					lua_setfield(m_ls, -2, vss[index].c_str()); // t[k] = v  t:idx 处的表(-2)   k:函数名  v:栈顶元素(闭包)
					//////////////////////////////////////////////////
				}
				lua_settop(m_ls, curTop);//栈恢复
				PCBCache(p);
				return true;
			}

			//注意，导出函数不能使用引用参数，可以使用const char *
			//可以返回多个参数，使用std::tuple ,不使用std::pair
			//可以导出 aaa.bbb.ccc这种函数(会自动创建table(aaa.bbb))
			//可以导出类的成员函数 std::function<xxx> = std::bind(&XXX::xxx,&v,std::placeholders::_1,...);
			template <typename FuncT>
			bool RegFunction(const char *pName, const  FuncT &fnc) {
				using FuncType = typename base::function_traits<FuncT>::f_type;
				//每注册一个函数，产生一个LuaBinder 对象
				auto newExport = new  LuaBinder<FuncType>(m_ls,pName, FuncType(fnc));
				return PCBRegister(newExport);
			}					
		};
	}
}
