#include "LuaCaller.h"

namespace app {
	namespace lua {

		int LuaArg::set_metatable(lua_State *ls, const mtable &mt, char gc) {
			//将new userdata压栈，最多后一个字节
			//表明关联的对象是否随userdata回收，需要和导出类 GC 联动
			//如果最后一个字节  0:表示不回收  1:随userdata 回收
			constexpr auto szptr = sizeof(void*);
			auto a = (void*)lua_newuserdata(ls, szptr + sizeof(char));
			memcpy(a, &(mt.pointer), szptr);//将指针数据写入
			memcpy((char*)a + szptr, &gc, 1);//写入最后一个字节
			auto type = luaL_getmetatable(ls, mt.table_name); //将table压栈
			int r = 0;
			if (type == LUA_TTABLE) {
				lua_setmetatable(ls, -2); //将栈顶的table 设置为 -2 位置userdata 的元表，弹出栈顶，栈顶为userdata
				r = 1;
			}
			else {
				assert(false);
				lua_pop(ls, 2);//弹出2个，栈顶和userdata,userdata由lua回收
			}
			return r;
		}

		bool LuaCaller::luax_assume_func(const char * func) {
			std::string source = func;
			//栈中第一个元素(第一个入栈)索引为1，最后一个元素(栈顶)索引为-1
			const char* orig = source.c_str();
			const char* name = orig;
			size_t offst = 0,end = 0;
			end = source.find_first_of('.', offst);
			if (end == std::string::npos)
			{ // 直接是函数  _G.func
				lua_getglobal(m_ls, name);//将全局的name入栈 , 5.3以上 直接返回类型，判断是否 == LUA_TFUNCTION ，但是5.3没有 luajit
				if (lua_isfunction(m_ls, -1))//检查栈顶是否是函数
					return true;
				else
					return false;
			}

			//table 类型 xxx.yyy.zzz
			source[end] = '\0';
			lua_getglobal(m_ls, name);//将tab入栈，在栈顶
			if (!lua_istable(m_ls, -1))
				return false;
			offst = end + 1;

			// 一直找到最后的 '.'
			while ((end = source.find_first_of('.', offst)) != std::string::npos)
			{ // 全部依次入栈
				source[end] = '\0';
				name = orig + offst;
				lua_getfield(m_ls, -1, name);
				if (!lua_istable(m_ls, -1)) {
					return false;
				}

				offst = end + 1;
			}

			//最后的函数
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

