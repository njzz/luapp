#include "LuaCaller.h"

namespace app {
	namespace lua {

		int LuaArg::set_metatable(lua_State *ls, const mtable &mt, char gc) {
			//��new userdataѹջ������һ���ֽ�
			//���������Ķ����Ƿ���userdata���գ���Ҫ�͵����� GC ����
			//������һ���ֽ�  0:��ʾ������  1:��userdata ����
			constexpr auto szptr = sizeof(void*);
			auto a = (void*)lua_newuserdata(ls, szptr + sizeof(char));
			memcpy(a, &(mt.pointer), szptr);//��ָ������д��
			memcpy((char*)a + szptr, &gc, 1);//д�����һ���ֽ�
			auto type = luaL_getmetatable(ls, mt.table_name); //��tableѹջ
			int r = 0;
			if (type == LUA_TTABLE) {
				lua_setmetatable(ls, -2); //��ջ����table ����Ϊ -2 λ��userdata ��Ԫ������ջ����ջ��Ϊuserdata
				r = 1;
			}
			else {
				assert(false);
				lua_pop(ls, 2);//����2����ջ����userdata,userdata��lua����
			}
			return r;
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

