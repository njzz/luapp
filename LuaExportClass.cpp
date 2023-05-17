#pragma once
#include "LuaExportClass.h"

namespace app {
	namespace lua {
		bool ExportClassToLua(lua_State *L, const ClassRegSet &rs) {
			bool r = false;
			auto top = lua_gettop(L);

			//可以不定义构造，这样则无法在lua里构造，只能在lua里使用
			if (rs.cons_pcb) {//pcb方式
				app::lua::LuaExportor::PCBGlobalRegister(rs.cons_pcb,L);
			}
			else if (rs.cons_fnc && rs.cons_name) {//原始方式
				lua_pushcfunction(L, rs.cons_fnc);
				lua_setglobal(L, rs.cons_name);
			}

			if ( rs.metatable && !rs.funcs.empty()) {
				//创建新的meta并且设置gc
				luaL_newmetatable(L, rs.metatable);//创建一个新表( 作为 userdata 的元表)，将它添加到注册表registry中，入栈
				if (rs.gc_fnc) {
					lua_pushstring(L, "__gc"); //压入gc名，以重新定义回收
					lua_pushcfunction(L, rs.gc_fnc);//gc函数入栈
					lua_settable(L, -3); //t[k] = v  t:index在栈中的值(元表) v:栈顶(元表) k:是栈顶下的内容(__gc)  然后弹出k和v，栈顶依然是元表
				}

				for (auto &pcb : rs.funcs) {
					lua_pushstring(L, pcb->m_name.c_str()); //函数字符串入栈
					lua_pushlightuserdata(L, pcb);//创建lightuserdata PCB 回调函数通过lua_upvalueindex(1)索引取值，入栈
					lua_pushcclosure(L, &app::lua::pcb_set::LuaCallBack, 1);//闭包入栈，带上栈顶的 PCB，并弹出关联的PCB栈
					lua_settable(L, -3);//将元表name方法设置为闭包函数 t[k] = v，t:index在栈中的值(元表) v:栈顶(闭包方法) k:是栈顶下的内容(函数名字)  然后弹出k和v，元表依旧在栈顶
				}

				lua_pushstring(L, "__index");//index入栈
				lua_pushvalue(L, -2);//-2元表复制入栈  完成后栈 -1: 元表 -2:__index -3:元表
				lua_settable(L, -3);//将-3位置的元表的index设置为自己
				//也可以将__index,__newindex 设置为C函数，然后在C代码里查找调用函数
				r = true;
			}
			lua_settop(L, top);//恢复栈
			return r;
		}

	}
}
