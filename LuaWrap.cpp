#include "LuaWrap.h"
#include "singleuse.h"

#ifdef _WIN32
#pragma comment(lib,"lua_lib5.3.lib") //静态库
//#pragma comment(lib,"lua_dll5.3.lib") //dll
#endif

namespace app {

	namespace lua {

		using cached_state = base::single_use_mgr<lua_State*>;

		cached_state &cached() {
			static cached_state mgr;
			return mgr;
		}

		static const NewVMInit &GLVMNew(const NewVMInit &vs=nullptr) {
			static NewVMInit nwf;
			if (vs) nwf = vs;
			return nwf;
		}

		void SetGlobalInit(const NewVMInit &f)
		{
			GLVMNew(f);
		}

		void CleanAll()
		{
			//清理所有状态机
			cached().clean_all(lua_close);
			//清理所有导出内存
			LuaExportor::FreePCB(nullptr);
		}

		///////////////////---------------------------LuaWrap---------------

		LuaWrap::LuaWrap(LuaWrap && r)
		{
			*this = std::move(r);
		}

		LuaWrap::~LuaWrap()
		{
			if (m_ls)
				cached().release(m_ls);
		}

		LuaWrap & LuaWrap::operator=(LuaWrap && r)
		{
			m_ls = r.m_ls;
			r.m_ls = nullptr;
			return *this;
		}

		int LuaWrap::Init()
		{
			bool golbalFuncInited = true;
			if (m_ls == nullptr) {
				bool bNewed = false;
				m_ls = cached().get([&bNewed]() {//获取一个未使用的 state ,没有就创建新的
					bNewed = true;//新创建的才调用该回调函数
					return luaL_newstate();//打开lua，新栈
				});

				if (bNewed && m_ls) {//新创建的
					luaL_openlibs(m_ls);//打开lua标准库
					auto &g = GLVMNew();
					if(g)
						golbalFuncInited = g(*this);
				}
			}
			//0:success  1:ls=nullptr  2:globalinit failed
			return (m_ls == nullptr) ?1:(golbalFuncInited?0:2);
		}

		void LuaWrap::Destroy()
		{
			if (m_ls) {
				cached().clean(m_ls);//清理缓存的状态机
				lua_close(m_ls);//关闭状态机
				LuaExportor::FreePCB(m_ls);//清理状态机pcb
				m_ls = nullptr;
			}
		}

		
	}

}