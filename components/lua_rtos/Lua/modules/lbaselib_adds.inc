#include "lua.h"
#include "lauxlib.h"

int luac(const char *src, const char *dst);

int stackDump(lua_State *L) {
    int i;
    int top = lua_gettop(L);

    printf("top %d\n", top);
    printf("------------------------\n");

    for (i = 1; i <= top; i++) {  /* repeat for each level */
      int t = lua_type(L, i);
      switch (t) {

        case LUA_TSTRING:  /* strings */
          printf("%d (%d): ", i, t);
          printf("`%s'\n", lua_tostring(L, i));
          break;

        case LUA_TBOOLEAN:  /* booleans */
          printf("%d (%d): ", i, t);
          printf(lua_toboolean(L, i) ? "true" : "false");
          printf("\n");
          break;

        case LUA_TNUMBER:  /* numbers */
  		  printf("%d (%d): ", i, t);
          printf("%g\n", lua_tonumber(L, i));
          break;

        default:  /* other values */
          printf("%d (%d): ", i, t);
          printf("%s\n", lua_typename(L, t));
          break;

      }
    }
    printf("\n\n");  /* end the listing */

    return 0;
}

static int luaB_try (lua_State *L) {
	int i, status;

    // At least one function is needed
    luaL_checkany(L, 1);

    // Check that all arguments are functions
    int total = lua_gettop(L);

    for(i=1;i <= total;i++) {
    	luaL_checktype(L, i, LUA_TFUNCTION);
    }

    // Call try function
	lua_insert(L, 1);
    if (lua_gettop(L) > 2) {
    	lua_insert(L, 1);
    }

    status = lua_pcall(L, 0, 0, 0);
    if ((status != LUA_OK) && (status != LUA_YIELD)) {
    	// The is an error

    	// Get and parse error message, with format
		// where:line-number error-code:error-message
		//
		// where and line-number are mandatory
		// error-code is optional
		const char *msg = lua_tostring(L, -1);

		const char *whereEnd = NULL;
		const char *lineEnd = NULL;
		const char *messageStart = NULL;;
		int error_code;

		whereEnd = strchr(msg,':');
		if (whereEnd) {
			lineEnd = strchr(whereEnd + 1,':');
		}

		if (lineEnd) {
			if (!sscanf(lineEnd + 2,"%d:", &error_code)) {
				error_code = 0;
				messageStart = lineEnd + 2;
			} else {
				messageStart = strchr(lineEnd + 2,':');
				messageStart++;
			}
		}

		// Remove error message
		lua_pop(L, 1);

		if (lua_gettop(L) > 0) {
			// Have catch function
    		// Call catch

    		if (lua_gettop(L) > 1) {
    			lua_insert(L, 1);
    		}

    		// Push where (where error occurs)
    		if (whereEnd) {
    			lua_pushlstring(L, msg, whereEnd - msg);
    		} else {
    			lua_pushnil(L);
    		}

            // Push line (line number where error occurs)
    		if (lineEnd) {
    			lua_pushlstring(L, whereEnd + 1, lineEnd -whereEnd - 1);
    		} else {
    			lua_pushnil(L);
    		}

            // Push error code
            if (!error_code) {
                // No error code, push nil value
                lua_pushnil(L);
            } else {
                lua_pushinteger(L, error_code);
            }

            // Push error message
            if (messageStart) {
            	lua_pushstring(L, messageStart);
            } else {
            	lua_pushstring(L, msg);
            }

    		status = lua_pcall(L, 4, 0, 0);
    	    if ((status != LUA_OK) && (status != LUA_YIELD)) {
    	    	// Catch function has failed, raise an error
				return luaL_error(L, msg);
    	    }
    	}
    }

    if (total == 3) {
    	status = lua_pcall(L, 0, 0, 0);
    	if ((status != LUA_OK) && (status != LUA_YIELD)) {
	    	// Finally function has failed, raise an error
			return luaL_error(L, lua_tostring(L, -1));
    	}
    }

    return 0;
}

static int luaB_compile (lua_State *L) {
	const char *src = luaL_checkstring(L, 1);
	const char *dst = luaL_optstring(L, 2, "");

	if (strcmp(src, dst) == 0) {
		luaL_error(L, "%s and %s are identical. Can't compile.", src, dst);
	}

	if (!*dst) {
		char *tmp;

		tmp = malloc(strlen(src) + 2);
		if (!tmp) {
			luaL_error(L, "not enough memory");
		}

		strcpy(tmp, src);
		strcat(tmp, "c");

		luac(src, (const char *)tmp);

		free(tmp);
	} else {
		luac(src, dst);
	}

	return 0;
}
