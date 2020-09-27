# Concept

Spoofs the function's return to a point in roblox's
own code (retcheck = bypassed) which is used to jump back
and continue execution flow, as if nothing even happened.


# HOW TO USE IT

Retcheck::init();
rL = std::get<0>(Retcheck::call(r_lua_newthread, "cdecl", { rL }));

Retcheck::call(r_lua_getfield, "fastcall", { rL, -10002, "printidentity" });
Retcheck::call(r_lua_pcall, "cdecl", { rL, 0, 1, 0 });
