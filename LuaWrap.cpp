#include "LuaWrap.h"
#include "singleuse.h"

#ifdef _WIN32
#pragma comment(lib,"lua_lib5.3.lib") //��̬��
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
			//��������״̬��
			cached().clean_all(lua_close);
			//�������е����ڴ�
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
				m_ls = cached().get([&bNewed]() {//��ȡһ��δʹ�õ� state ,û�оʹ����µ�
					bNewed = true;//�´����Ĳŵ��øûص�����
					return luaL_newstate();//��lua����ջ
				});

				if (bNewed && m_ls) {//�´�����
					luaL_openlibs(m_ls);//��lua��׼��
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
				cached().clean(m_ls);//�������״̬��
				lua_close(m_ls);//�ر�״̬��
				LuaExportor::FreePCB(m_ls);//����״̬��pcb
				m_ls = nullptr;
			}
		}

		
	}

}