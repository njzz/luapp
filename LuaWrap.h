#pragma once
#include "LuaExportClass.h"
//author:njzz
//github:https://github.com/njzz/luapp
//csdn:https://blog.csdn.net/m0_66399932

//LUA虚拟机类，lua_state 封装，新的LuaWrap,不代表新的lua_state
//可能是复用老的 lua_state，当新的lua_state产生时，调用回调函数
namespace app {
	namespace lua {
		class LuaWrap;
		//当新的LUA虚拟机产生时，会调用此函数
		using NewVMInit = std::function<bool(LuaWrap &)>;
		//设置全局虚拟机初始化函数
		void SetGlobalInit(const NewVMInit &);
		//清理所有的虚拟机
		void CleanAll();

		//Lua虚拟机封装，集成了 调用Lua函数，向Lua脚本输出函数等功能
		//LuaWrap 对象没意义，关键在于里面的状态机 lua_State *
		class LuaWrap:public LuaExportor {
		public:
			LuaWrap() = default;
			LuaWrap(LuaWrap &&r);
			~LuaWrap();

			//转化为bool
			operator bool() const { return m_ls != nullptr; }
			//重载 == 判断
			bool operator ==(const LuaWrap &r) const { return m_ls == r.m_ls; }
			//获取原始state，做一些其它操作
			lua_State* get() const { return m_ls; }
			//移动赋值
			LuaWrap &operator =(LuaWrap &&r);

			//初始化，并传递一个回调函数，如果同时设置全局回调，先调用全局的
			//注：只有新建状态机的时候才会回调，复用不会回调
			bool Init(const NewVMInit &f=nullptr);

			//销毁当前状态机
			void Destroy();

		protected:
			void operator =(const LuaWrap &) = delete;
		};

		//临时对象，方便调用基类函数
		class LuaTemp:public LuaWrap
		{
		public:
			LuaTemp(lua_State *s) {
				m_ls = s;
			}
			~LuaTemp() {
				m_ls = nullptr;//设置为nullptr，阻止LuaWrap调用 release
			}

		};
	}

}