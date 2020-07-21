# Examples

Q: What do I need to auto-update my conventions?
A: Add disassembler.h and autoconventions.h in your project 
and include 'autoconventions.h' in main.cpp or whereever
you put your function typedefs.

Q: I included autoconventions.h, now what?
A: Use the function called 'createRoutine' on any functions
you know of that have a chance of getting its calling convention
changed.
For example, lua_getfield, lua_setfield, lua_pushcclosure, . . .
these are among the functions that ROBLOX commonly modifies.

Q: What args do I pass to createRoutine?
A: The first one is the function, so for lua_getfield,
you would put the function's address.
The second one is the number of args that you know it uses.
We know that lua_getfield will ALWAYS use 3 args.*
So put 3.

*Unless they change the function completely, in which case
you would have to update your source regardless and the # of args.


There is ONE exception to this method, that will I will
eventually fix -- lua_pushnumber pushes a double-sized arg
which isn't supported by createRoutine at the moment.

So  ask Celery#8969 for updates

