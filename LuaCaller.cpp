#include "LuaCaller.h"

namespace app {
	namespace lua {
		void LuaCaller::pusharg(int arg)
		{

			/* lua_checkstackȷ����ջ��������"n"�������λ�����費�ܰѶ�ջ��չ����Ӧ�ĳߴ磬��������"false"��
			 * ʧ�ܵ�ԭ���������ջ��չ���ȹ̶����ߴ绹�������Ǽ�ǧ��Ԫ�أ�������ڴ�ʧ�ܡ�
			 * ���������Զ������С��ջ�������ջ�Ѿ�����Ҫ�Ĵ��ˣ���ô�ͱ���ԭ����
			*/
			if (lua_checkstack(m_ls, 1))
				lua_pushinteger(m_ls, arg);
		}

		void LuaCaller::pusharg(long long arg)
		{
			if (lua_checkstack(m_ls, 1))
				lua_pushinteger(m_ls, arg);
		}

		void LuaCaller::pusharg(size_t arg)
		{
			if (lua_checkstack(m_ls, 1))
				lua_pushinteger(m_ls, arg);
		}

		void LuaCaller::pusharg(float arg)
		{
			if (lua_checkstack(m_ls, 1))
				lua_pushnumber(m_ls, arg);
		}

		void LuaCaller::pusharg(double arg)
		{
			if (lua_checkstack(m_ls, 1))
				lua_pushnumber(m_ls, arg);
		}

		void LuaCaller::pusharg(const char * arg)
		{
			if (lua_checkstack(m_ls, 1))
				lua_pushstring(m_ls, arg);
		}

		void LuaCaller::pusharg(const std::string & arg)
		{
			if (lua_checkstack(m_ls, 1))
				lua_pushlstring(m_ls, arg.c_str(), arg.length());
		}

		void LuaCaller::pusharg(void * arg)
		{
			if (lua_checkstack(m_ls, 1))
				lua_pushlightuserdata(m_ls, arg);
		}

		void LuaCaller::pusharg(nullptr_t arg)
		{
			if (lua_checkstack(m_ls, 1))
				lua_pushnil(m_ls);
		}

		bool LuaCaller::luax_assume_func(const char * func) {
			std::string source = func;
			//ջ�е�һ��Ԫ��(��һ����ջ)����Ϊ1�����һ��Ԫ��(ջ��)����Ϊ-1
			const char* orig = source.c_str();
			const char* name = orig;
			size_t offst = 0,end = 0;
			end = source.find_first_of('.', offst);
			if (end == std::string::npos)
			{ // ֱ���Ǻ���  _G.func
				lua_getglobal(m_ls, name);//��ȫ�ֵ�name��ջ , 5.3���� ֱ�ӷ������ͣ��ж��Ƿ� == LUA_TFUNCTION ������5.3û�� luajit
				if (lua_isfunction(m_ls, -1))//���ջ���Ƿ��Ǻ���
					return true;
				else
					return false;
			}

			//table ���� xxx.yyy.zzz
			source[end] = '\0';
			lua_getglobal(m_ls, name);//��tab��ջ����ջ��
			if (!lua_istable(m_ls, -1))
				return false;
			offst = end + 1;

			// һֱ�ҵ����� '.'
			while ((end = source.find_first_of('.', offst)) != std::string::npos)
			{ // ȫ��������ջ
				source[end] = '\0';
				name = orig + offst;
				lua_getfield(m_ls, -1, name);
				if (!lua_istable(m_ls, -1)) {
					return false;
				}

				offst = end + 1;
			}

			//���ĺ���
			name = orig + offst;
			lua_getfield(m_ls, -1, name);

			return !!lua_isfunction(m_ls, -1);
		}

		bool LuaCaller::LoadFile(const char * file)
		{
			return 0 == luaL_loadfile(m_ls, file);
		}

		bool LuaCaller::DoFile(const char * file)
		{
			return 0 == luaL_dofile(m_ls, file);
		}
	}
}

