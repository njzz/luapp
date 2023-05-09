#pragma once
#include <lua.hpp>

namespace app {
	namespace lua {
		struct VM {
			lua_State* m_ls = nullptr;//lua×´Ì¬»ú/ĞéÄâ»úÖ¸Õë
		};
	}

}