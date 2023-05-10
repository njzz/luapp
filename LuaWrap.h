#pragma once
#include "LuaExportClass.h"
//author:njzz
//github:https://github.com/njzz/luapp
//csdn:https://blog.csdn.net/m0_66399932

//LUA������࣬lua_state ��װ���µ�LuaWrap,�������µ�lua_state
//�����Ǹ����ϵ� lua_state�����µ�lua_state����ʱ�����ûص�����
namespace app {
	namespace lua {
		class LuaWrap;
		//���µ�LUA���������ʱ������ô˺���
		using NewVMInit = std::function<bool(LuaWrap &)>;
		//����ȫ���������ʼ������
		void SetGlobalInit(const NewVMInit &);
		//�������е������
		void CleanAll();

		//Lua�������װ�������� ����Lua��������Lua�ű���������ȹ���
		//LuaWrap ����û���壬�ؼ����������״̬�� lua_State *
		class LuaWrap:public LuaExportor {
		public:
			LuaWrap() = default;
			LuaWrap(LuaWrap &&r);
			~LuaWrap();

			//ת��Ϊbool
			operator bool() const { return m_ls != nullptr; }
			//���� == �ж�
			bool operator ==(const LuaWrap &r) const { return m_ls == r.m_ls; }
			//��ȡԭʼstate����һЩ��������
			lua_State* get() const { return m_ls; }
			//�ƶ���ֵ
			LuaWrap &operator =(LuaWrap &&r);

			//��ʼ����������µ�״̬��������� SetGlobalInit ���õĳ�ʼ������
			//����ֵ 0:�ɹ�  1:�½�״̬��ʧ��  2:ȫ�ֳ�ʼ������ʧ��
			int Init();

			//���ٵ�ǰ״̬��
			void Destroy();

		protected:
			void operator =(const LuaWrap &) = delete;
		};

		//��ʱ���󣬷�����û��ຯ��
		class LuaTemp:public LuaWrap
		{
		public:
			LuaTemp(lua_State *s) {
				m_ls = s;
			}
			~LuaTemp() {
				m_ls = nullptr;//����Ϊnullptr����ֹLuaWrap���� release
			}

		};
	}

}