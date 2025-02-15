/*
 * This file is part of OpenPNGStudio. 
 * Copyright (C) 2024-2025 LowByteFox
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <lua_ctx.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <string.h>
#include <context.h>
#include <stdlib.h>

extern struct context ctx;

void expand_import_path(lua_State *L, const char *path)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");

    lua_pushstring(L, path);

    lua_concat(L, 2);

    lua_setfield(L, -2, "path");
   
    lua_pop(L, 1);
}

void script_manager_add_script(struct script_mgr *mgr, struct lua_script
    *script)
{
    mgr->scripts = realloc(mgr->scripts, (++mgr->script_count) * sizeof(*script));
    mgr->scripts[mgr->script_count - 1] = *script;
    mgr->to_import = script->name.buffer;
}

int lua_script_loader(lua_State *L)
{
    const char* const modname = lua_tostring(L, 1);

    struct script_mgr *mgr = &ctx.editor.script_manager;
    for (size_t i = 0; i < mgr->script_count; i++) {
        struct lua_script *script = mgr->scripts + i;
        if (strcmp(modname, script->name.buffer) == 0) {
            const int res = luaL_loadbufferx(L, script->buffer,
                script->buffer_size, modname, "t");

            if (res != LUA_OK)
                return lua_error(L);

            return 1;

        }
    }

    lua_pushfstring(L, "unknown module \"%s\"", modname);
    return 1;
}

int lua_script_load_req(lua_State *L)
{
    if (ctx.editor.script_manager.to_import == NULL)
        lua_pushnil(L);
    else {
        lua_pushstring(L, ctx.editor.script_manager.to_import);
        ctx.editor.script_manager.to_import = NULL;
    }

    return 1;
}
