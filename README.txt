# Concept

Spoofs the function's return to a point
which is used to jump back and continue
execution flow, as if nothing happened.
All without modifying any portion of the text segment,
or making a copy of the function.


# Examples

Retcheck::init();
rL = std::get<0>(Retcheck::call(r_lua_newthread, "cdecl", { rL }));

Retcheck::call(r_lua_getfield, "fastcall", { rL, -10002, "printidentity" });
Retcheck::call(r_lua_pcall, "cdecl", { rL, 0, 1, 0 });
