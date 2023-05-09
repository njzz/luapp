#include "LuaCaller.h"

namespace app {
	namespace lua {
		void LuaCaller::pusharg(int arg)
		{

			/* lua_checkstack确保堆栈上至少有"n"个额外空位。假设不能把堆栈扩展到相应的尺寸，函数返回"false"。
			 * 失败的原因包括将把栈扩展到比固定最大尺寸还大（至少是几千个元素）或分配内存失败。
			 * 这个函数永远不会缩小堆栈，假设堆栈已经比须要的大了，那么就保持原样。
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

