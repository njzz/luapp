#include "LuaWrap.h"
#include "singleuse.h"

#ifdef _WIN32
#pragma comment(lib,"lua_lib5.3.lib") //��̬��
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
			//��������״̬��
			cached().clear_all(lua_close,0);
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
				if (!cached().release(m_ls, m_version)) {//�汾��ƥ��
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
				//��ȡһ��δʹ�õ� state ,û�оʹ����µ�
				//����汾��ƥ�䣬���ͷŻ���
				m_ls = cached().get([&bNewed](long long v) {
					bNewed = true;//�´����Ĳŵ��øûص�����
					return luaL_newstate();//��lua����ջ
				},lua_close,v);
				m_version = v;//��ǰ�汾

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
				cached().clear(m_ls);//�������״̬��
				lua_close(m_ls);//�ر�״̬��
				LuaExportor::FreePCB(m_ls);//����״̬��pcb
				m_ls = nullptr;
			}
		}

		
	}

}