# Examples

rL = std::get<0>(Retcheck::call(r_lua_newthread, "cdecl", { rL }));

Retcheck::call(r_lua_getfield, "fastcall", { rL, -10002, "printidentity" });
Retcheck::call(r_lua_pcall, "cdecl", { rL, 0, 1, 0 });
