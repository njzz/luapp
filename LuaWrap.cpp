#include "LuaWrap.h"
#include "singleuse.h"

#ifdef _WIN32
#pragma comment(lib,"lua_lib5.3.lib") //静态库
//#pragma comment(lib,"lua_dll5.3.lib") //dll
#endif

namespace app {

	namespace lua {

		using cached_state = base::single_use<lua_State*,long long>;

		cached_state &cached() {
			static cached_state mgr(0);
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
			cached().clear_all(lua_close,0);
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
				if (!cached().release(m_ls, m_version)) {//版本不匹配
					lua_close(m_ls);
				}
		}

		LuaWrap & LuaWrap::operator=(LuaWrap && r)
		{
			m_ls = r.m_ls;
			r.m_ls = nullptr;
			return *this;
		}

		int LuaWrap::Init(long long v)
		{
			bool golbalFuncInited = true;
			if (m_ls == nullptr) {
				bool bNewed = false;
				//获取一个未使用的 state ,没有就创建新的
				//如果版本不匹配，则释放缓存
				m_ls = cached().get([&bNewed](long long v) {
					bNewed = true;//新创建的才调用该回调函数
					return luaL_newstate();//打开lua，新栈
				},lua_close,v);
				m_version = v;//当前版本

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
				cached().clear(m_ls);//清理缓存的状态机
				lua_close(m_ls);//关闭状态机
				LuaExportor::FreePCB(m_ls);//清理状态机pcb
				m_ls = nullptr;
			}
		}

		
	}

}