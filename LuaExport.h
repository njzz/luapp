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
				auto pv = (pcb_set*)lua_touserdata(L, lua_upvalueindex(1));
				return pv->call(L);
			}

			std::string m_name;//name
		};

		//调用并设置返回值
		template <typename R>
		struct InvokeCppFunction {
			template <typename T, typename ...Args>
			static int call(lua_State *states, const T &f, Args&&... args) {
				return LuaArg::set(states, f(std::forward<Args>(args)...));//包含返回值的情况
			}
		};

		//无参数版本，需要根据返回值区分，在RP里不能特化，因为set的void参数不能实例化
		template <>
		struct InvokeCppFunction<void> {
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
			using CallParamTuple = typename base::function_traits<Function>::param_tuple_d;
			using FuncRtType = typename base::function_traits<Function>::rt_type;

			LuaBinder(const char *pName,const Function &f) : m_f(f) {
				m_name = pName;
			}
			~LuaBinder() {
				//一般不需要，直接关闭状态机即可，所以不写在析构函数里
				//release()
			}

			template<size_t... indexs>
			int FillParamsAndCall(lua_State *states, const std::index_sequence<indexs... > &) {
				//constexpr auto param_size = std::tuple_size<CallParamTuple>::value;
				CallParamTuple params{};//参数
				LuaArg::getargs(states, 1, std::get<indexs>(params)...);//lua参数从1开始，用tuple存储参数
				return InvokeCppFunction<FuncRtType>::call(states, m_f, std::get<indexs>(params)...);//用tuple里的参数去调用c函数
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
		};

		template<typename FuncT>
		pcb_set *CreatePCB(lua_State *l,const FuncT &v, const char *n) {
			using FuncType = typename base::function_traits<FuncT>::f_type;
			//每注册一个函数，产生一个LuaBinder 对象
			auto pcb = new  LuaBinder<FuncType>( n, v);
			LuaExportor::PCBCache(l,pcb);//缓存起来
			return pcb;
		}

		//向lua导出函数
		class LuaExportor :public lua::LuaCaller {		
		public:
			using PCBList = std::list<lua::pcb_set *>;//不在不同状态机共享 pcb，可能在不同状态机同名不同函数，同函数不同名。
			using CacheList = base::lock_t<std::unordered_map<lua_State *,PCBList>>;
			using Lock_Guard = std::lock_guard<CacheList>;			

			//缓存起来?
			static CacheList &PCBCache(lua_State *l=nullptr, lua::pcb_set *pcb = nullptr) {
				static CacheList cachePCBList;
				if (pcb) {
					Lock_Guard gb(cachePCBList);
					cachePCBList[l].push_back(pcb);
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

			static std::vector<std::string > SS(const std::string& name) {
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

			static bool TopStackRegisteName( lua_State *ls ,const std::string &name) {
				auto vss = SS(p->name());
				auto sz = vss.size();
				auto curTop = lua_gettop(ls);//记录当前栈顶
				assert(curTop > 0);
				if (vss.size() == 1) {//xxx 类型
					lua_pushvalue(ls, -1);//栈顶复制入栈，保持堆栈平衡，因为下面 setglobal会弹出栈顶，最后的settop只能push一个nil到栈顶了
					lua_setglobal(ls, name.c_str());//设置全局符号 t[k] = v  t:全局表  k:name  v:栈顶元素  完成后弹出栈顶
				}
				else {
					//table 类型 xxx.yyy.zzz
					auto curType = lua_getglobal(ls, vss[0].c_str());//全局，将tab入栈顶
					if (curType != LUA_TTABLE) {//第一个不是table
						assert(curType == LUA_TNIL);//如果不是table，是其它的，弹出调试警告
						lua_pop(ls, 1);//弹出1个元素(栈顶的null)	
						lua_newtable(ls);//新表到栈顶  lua_createtable(m_ls, 0, sizeof(luaL_Reg) - 1);
						lua_setglobal(ls, vss[0].c_str());//t[k]=v   t:全局表 k:table名  v:栈顶新表，将第一个设置为全局表   完成后弹出栈顶
						lua_getglobal(ls, vss[0].c_str());//表重新入栈顶
					}
					size_t index = 1;//到这里栈顶是表，从1开始，0已经是表了
					while (index < sz - 1) {//检查表，直到倒数第二个，最后一个为函数
						//检查类型/重新入栈  将-1表处名vss[index]的表压入栈顶并返回类型
						curType = lua_getfield(ls, -1, vss[index].c_str());
						if (curType != LUA_TTABLE) {//不是表
							assert(curType == LUA_TNIL);//如果不是table，是其它的，弹出调试警告
							lua_pop(ls, 1);//弹出1个(栈顶)元素	
							lua_newtable(ls);//新表到栈顶 -1
							lua_setfield(ls, -2, vss[index].c_str());//t[k]=v 设置 设置老表(-2)的k(name)为新表(-1)，并弹出栈顶新表
							//不增加 index++ ，让表入栈
						}
						else {
							index++;
						}
					}

					//最后一个
					lua_pushvalue(ls, curTop);//将top处内容复制到栈顶
					lua_setfield(ls, -2, vss[index].c_str()); // t[k] = v  t:idx 处的表(-2)   k:vss[index].c_str()  v:栈顶元素   然后弹出栈顶
					//////////////////////////////////////////////////
				}
				lua_settop(ls, curTop);//栈恢复
				return true;
			}

			static bool CFuncGlobalRegister(lua_CFunction cf, const std::string &name, lua_State *ls) {
				if (cf && !name.empty() && ls) {
					lua_pushcfunction(ls, cf);//函数入栈
					TopStackRegisteName(ls, name);//注册
					lua_pop(ls, 1);//恢复栈
					return true;
				}
				return false;
			}

			static bool PCBGlobalRegister( pcb_set *p,lua_State *ls) {
				//auto curTop = lua_gettop(ls);//记录当前栈顶
				if (p && ls) {
					lua_pushlightuserdata(ls, p);//push 一个轻量级用户数据到栈顶，lua不管理该指针内存
					lua_pushcclosure(ls, &pcb_set::LuaCallBack, 1);//闭包入栈，并关联栈上的1个参数(从栈顶开始)，完成后弹出关联的参数，函数内用 lua_upvalueindex 获取关联参数
					TopStackRegisteName(ls, p->name());//将栈顶闭包注册到全局
					//lua_settop(ls, curTop);//栈恢复
					lua_pop(ls, 1);//push一个闭包，恢复它
					return true;
				}
				return false;
			}

			//导出函数可以使用引用参数，仅为了提高效率，而不会修改lua参数里的值
			//可以返回多个参数，使用std::tuple
			//可以导出 aaa.bbb.ccc这种函数(会自动创建table(aaa.bbb))
			//可以导出类的成员函数 std::function<xxx> = std::bind(&XXX::xxx,&v,std::placeholders::_1,...);
			//如果需要的参数是 void *，则表示为 userdata 或者 lightuserdata
			//如果返回值为 void *，则压栈给lua为 lightuserdata，如果为 app::lua::mtable ，则表示userdata
			template <typename FuncT>
			bool RegFunction(const char *pName, const  FuncT &fnc) {
				auto pcb = CreatePCB(m_ls, fnc, pName);
				return PCBGlobalRegister(pcb,m_ls);
			}					
		};
	}
}

