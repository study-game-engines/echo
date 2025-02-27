#include "lua_binder.h"
#include "engine/core/log/Log.h"
#include "engine/core/base/method_bind.h"
#include "engine/core/base/class_method_bind.h"
#include "engine/core/memory/MemAllocDef.h"
#include "engine/core/util/PathUtil.h"
#include "engine/core/io/IO.h"
#include "engine/core/script/scratch/scratch.h"

namespace Echo
{
	bool tryLuaFile(lua_State* L, const String& fileName, int& errCode)
	{
		String luaFile = "Res://" + fileName + ".lua";
		MemoryReader memReader(luaFile);
		if (memReader.getSize())
		{
			errCode = luaL_loadbuffer(L, memReader.getData<char*>(), memReader.getSize(), luaFile.c_str());
			return errCode ? false : true;
		}

		return false;
	}

	bool tryScratchFile(lua_State* L, const String& fileName, int& errCode)
	{
		String scratchFile = "Res://" + fileName + ".scratch";
		ScratchPtr scratch = ECHO_DOWN_CAST<Scratch*>(Res::get(scratchFile));
		if(scratch->compile() && scratch->getLua().size()>0)
		{
			errCode = luaL_loadbuffer(L, scratch->getLua().c_str(), scratch->getLua().size(), scratchFile.c_str());
			return errCode ? false : true;
		}

		return false;
	}

	int luaLoaderEcho(lua_State* L)
	{
		String fileName(luaL_checkstring(L, 1));
		fileName = StringUtil::Replace(fileName, ".", "/");

		int errCode;
		if (tryLuaFile(L, fileName, errCode))
		{
			return 1;
		}
		else if (tryScratchFile(L, fileName, errCode))
		{
			return 1;
		}

		if (errCode)
		{
			int top = lua_gettop(L);
			EchoLogError("lua load [%s] failed", lua_tostring(L, -1));
			lua_settop(L, top);
			return errCode;
		}

		return 1;
	}

//	static void stackDump(lua_State *L)
//	{
//#ifdef ECHO_EDITOR_MODE
//		struct Element
//		{
//			int		m_idx;
//			String  m_type;
//			String  m_value;
//		};
//		static vector<Element>::type  luaStack;
//		luaStack.clear();
//
//		int top = lua_gettop(L);
//		for (int i = 1; i <= top; i++)
//		{
//			Element elem;
//			elem.m_idx = i;
//
//			int t = lua_type(L, i);
//			elem.m_type = lua_typename(L, t);
//
//			switch (t)
//			{
//			case LUA_TSTRING:
//				elem.m_value = lua_tostring(L, i);
//				break;
//
//			case LUA_TBOOLEAN:
//				elem.m_value = (lua_toboolean(L, i) ? "true" : "false");
//				break;
//
//			case LUA_TNUMBER:
//				elem.m_value =StringUtil::Format("%g", lua_tonumber(L, i));
//				break;
//
//			default:
//				elem.m_value = lua_typename(L, t);
//				break;
//			}
//
//			luaStack.emplace_back(elem);
//		}
//#endif
//	}

	static int method_cb(lua_State* L)
	{
		// get method ptr
		MethodBind* methodPtr = static_cast<MethodBind*>(lua_touserdata(L, lua_upvalueindex(1)));
		if (methodPtr)
		{
			return methodPtr->call(L);
		}

		return 0;
	}

	static int class_method_cb(lua_State* L)
	{
		// get object ptr
		if (lua_istable(L, 1))
		{
			lua_getfield(L, 1, "this");
			Object* objPtr = static_cast<Object*>(lua_touserdata(L, -1));
			lua_pop(L, 1);

			// get method ptr
			ClassMethodBind* methodPtr = static_cast<ClassMethodBind*>(lua_touserdata(L, lua_upvalueindex(1)));
			if (objPtr && methodPtr)
			{
				return methodPtr->call(objPtr, L);
			}
		}

		return 0;
	}

	LuaBinder* LuaBinder::instance()
	{
		static LuaBinder* binder = EchoNew(LuaBinder);
		return binder;
	}

	// set state
	void LuaBinder::init()
	{
		m_luaState = luaL_newstate();
		luaL_openlibs(m_luaState);

		addLoader(luaLoaderEcho);
		setSearchPath("Res://");
	}

	// set search path
	void LuaBinder::setSearchPath(const String& path)
	{
		setGlobalVariableStr("package.path", StringUtil::Format("%s?.lua", path.c_str()).c_str());
	}

	// register
	bool LuaBinder::registerMethod(const String& methodName, MethodBind* method)
	{
		LUA_STACK_CHECK(m_luaState);

		String currentLayerName;
		int upperTableCount = lua_get_upper_tables(m_luaState, methodName, currentLayerName);
		int parentIdx = lua_gettop(m_luaState);

		lua_pushlightuserdata(m_luaState, method);
		lua_pushcclosure(m_luaState, method_cb, 1);

		if (upperTableCount != 0)
		{
			lua_setfield(m_luaState, parentIdx, currentLayerName.c_str());
			lua_pop(m_luaState, upperTableCount);
		}
		else
		{
			lua_setglobal(m_luaState, methodName.c_str());
		}

		lua_settop(m_luaState, 0);

		return true;
	}

	bool LuaBinder::registerClass(const char* className, const char* parentClassName)
	{
		LUA_STACK_CHECK(m_luaState);

		//create metatable for class
		luaL_newmetatable(m_luaState, className);
		const int metatable = lua_gettop(m_luaState);

		//change the metatable's __index to metatable itself;
		lua_pushliteral(m_luaState, "__index");
		lua_pushvalue(m_luaState, metatable);
		lua_settable(m_luaState, metatable);

		// inherits from parent class
		if (parentClassName && strlen(parentClassName))
		{
			// lookup metatable in Lua registry
			luaL_getmetatable(m_luaState, parentClassName);
			lua_setmetatable(m_luaState, metatable);
		}

		lua_pop(m_luaState, 1);

		return true;
	}

	bool LuaBinder::registerClassMethod(const String& className, const String& methodName, ClassMethodBind* method)
	{
		LUA_STACK_CHECK(m_luaState);

		luaL_getmetatable(m_luaState, className.c_str());	// stack 1
		lua_pushstring(m_luaState, methodName.c_str());     // stack 2
		lua_pushlightuserdata(m_luaState, method);			// stack 3
		lua_pushcclosure(m_luaState, class_method_cb, 1);	// stack 3

		lua_settable(m_luaState, 1);
		lua_pop(m_luaState, 1);

		return true;
	}

	bool LuaBinder::registerObject(const String& className, const String& objectName, void* obj)
	{
		if ( !className.empty() && !objectName.empty() && obj)
		{
			LUA_STACK_CHECK(m_luaState);

			String currentLayerName;
			int upperTableCount = lua_get_upper_tables(m_luaState, objectName, currentLayerName);
			int parentIdx = lua_gettop(m_luaState);

			lua_newtable(m_luaState);
			int objIdx = lua_gettop(m_luaState);

			lua_pushstring(m_luaState, "this");
			lua_pushlightuserdata(m_luaState, obj);
			lua_settable(m_luaState, objIdx);

			luaL_getmetatable(m_luaState, className.c_str());
			lua_setmetatable(m_luaState, objIdx);

			if (upperTableCount != 0)
			{
				lua_setfield( m_luaState, parentIdx, currentLayerName.c_str());
				lua_pop(m_luaState, upperTableCount);
			}
			else
			{
				lua_setglobal(m_luaState, objectName.c_str());
			}	
		}

		return true;
	}

	void LuaBinder::getClassMethods(const String& className, StringArray& methods)
	{
		LUA_STACK_CHECK(m_luaState);

		// create metatable for class
		luaL_getmetatable(m_luaState, className.c_str());
		const int metatable = lua_gettop(m_luaState);
	
		if (!lua_isnil(m_luaState, metatable))
		{
			lua_pushnil(m_luaState);
			while (lua_next(m_luaState, metatable))
			{
				const char* key = lua_tostring(m_luaState, -2);
				const char* type = lua_typename(m_luaState, lua_type(m_luaState, -1));
				if(strcmp(type, "function")==0)
					methods.emplace_back(key);

				lua_pop(m_luaState, 1);
			}
		}

		// pop meta table
		lua_pop(m_luaState, 1);
	}

	void LuaBinder::addSearchPath(const String& path)
	{
		String curPath = getGlobalVariableStr("package.path");
		setGlobalVariableStr("package.path", StringUtil::Format("%s?.lua;%s", path.c_str(), curPath.c_str()));
	}

	void LuaBinder::addLoader(lua_CFunction func)
	{
		if (func)
		{
			lua_getglobal(m_luaState, "package");
			lua_getfield(m_luaState, -1, "searchers");

			lua_gettop(m_luaState);

			// << loader func
            int rawLen = static_cast<int>(lua_rawlen(m_luaState, -1));
			for (int i = rawLen; i >= 1; --i)
			{
				lua_rawgeti(m_luaState, -1, i);
				lua_rawseti(m_luaState, -2, i+1);
			}

			lua_pushcfunction(m_luaState, func);
			lua_rawseti(m_luaState, -2, 1);
			lua_settop(m_luaState, 0);
		}
	}

	bool LuaBinder::execString(const String& script, bool execute)
	{
		LUA_STACK_CHECK(m_luaState);

		if (!luaL_loadstring(m_luaState, script.c_str()))
		{
			if (execute)
			{
				if (!lua_pcall(m_luaState, 0, LUA_MULTRET, 0))
					return true;
			}
			else
			{
				lua_pop(m_luaState, 1);
				return true;
			}
		}

		outputError();
		return false;
	}

	bool LuaBinder::getGlobalVariableBoolean(const String& varName)
	{
		LUA_STACK_CHECK(m_luaState);

		lua_getglobal(m_luaState, varName.c_str());

	#ifdef ECHO_EDITOR_MODE
		if (lua_isnil(m_luaState, 1))	EchoLogError("Lua global variable [%s == nil]", varName.c_str());
	#endif

		bool result = lua_toboolean(m_luaState, 1)!=0;

		// clear stack
		lua_settop(m_luaState, 0);

		return result;
	}

	float LuaBinder::getGlobalVariableFloat(const String& varName)
	{
		return static_cast<float>(getGlobalVariableDouble(varName));
	}

	double LuaBinder::getGlobalVariableDouble(const String& varName)
	{
		LUA_STACK_CHECK(m_luaState);

		lua_getglobal(m_luaState, varName.c_str());

	#ifdef ECHO_EDITOR_MODE
		if (lua_isnil(m_luaState, 1))	EchoLogError("lua global variable [%s == nil]", varName.c_str());
	#endif

		double result = lua_tonumber(m_luaState, 1);

		// clear stack
		lua_settop(m_luaState, 0);

		return result;
	}

	String LuaBinder::getGlobalVariableStr(const String& varName)
	{
		LUA_STACK_CHECK(m_luaState);

		StringArray vars = StringUtil::Split(varName, ".");

		lua_getglobal(m_luaState, vars[0].c_str());
		for (size_t idx = 1; idx < vars.size(); idx++)
			lua_getfield(m_luaState, -1, vars[idx].c_str());

#ifdef ECHO_EDITOR_MODE
		if (lua_isnil(m_luaState, vars.size())) EchoLogError("lua global variable [%s == nil]", varName.c_str());
#endif
		lua_gettop(m_luaState);
		String result = lua_tostring(m_luaState, int(vars.size()));
		lua_gettop(m_luaState);

		// clear stack
		lua_settop(m_luaState, 0);

		return result;
	}

	void LuaBinder::setGlobalVariableStr(const String& varName, const String& value)
	{
		LUA_STACK_CHECK(m_luaState);

		StringArray vars = StringUtil::Split(varName, ".");

		lua_getglobal(m_luaState, vars[0].c_str());
		for (size_t idx = 1; idx < vars.size()-1; idx++)
			lua_getfield(m_luaState, -1, vars[idx].c_str());

		lua_pushstring(m_luaState, value.c_str());

		lua_setfield(m_luaState, -2, vars.back().c_str());

		lua_settop(m_luaState, 0);
	}

	void LuaBinder::outputError(int pop)
	{
		// output error
		Log::instance()->error(lua_tostring(m_luaState, -1));
		lua_pop(m_luaState, 1);

		if (pop > 0)
			lua_pop(m_luaState, pop);
	}

	static bool isLuaIdentifier(char c)
	{
		if (c >= 'A' && c <= 'Z')
			return true;
		if (c >= 'a' && c <= 'z')
			return true;
		if (c >= '0' && c <= '9')
			return true;
		if (c == '_')
			return true;
		return false;
	}

	int LuaBinder::parseName(char* name, char** result, const int resultSize, char* lastSeparator)
	{
		char*	p = name;
		int		count = 0;
		bool	start = false;
		while (true)
		{
			const char c = *p++;
			if (isLuaIdentifier(c))
			{
				if (!start)
				{
					start = true;
					result[count++] = p - 1;
					if (count >= resultSize)
						break;
				}
			}
			else
			{
				if (NULL != lastSeparator && c != 0)
					*lastSeparator = c;

				*(p - 1) = 0;
				start = false;
			}

			if (c == 0)
				break;
		}

		return count;
	}
}
