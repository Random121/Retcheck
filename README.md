# UPDATES

I finally did some organizing, though it's probably even messier than before.

But the good news is, I got the code separated into both .h and .cpp.
The bad news is, even though it is better this way, you now
need to include both eyestep.h and eyestep.cpp.

Extra files (not required to use eyestep) are:
eyestep_util.h, peb.h and eyestep_util.cpp (These 3 must also be used altogether),
retcheck.h,



# About

EyeStep is a nearly full x86 disassembler written from scratch
exclusively aimed towards exploiting, but also
as a hobby project.

eyestep_util.h contains functions for
manipulating all things memory.
It has been fully built to handle and support memory editing in both external (.exe) applications and DLL's.

You can place untraceable hooks with
smooth adaption to the surrounding instructions.

You can scavenge through memory with predictable, stable functions
that take into account future adjustments.

You can identify and distinguish IMM32 offsets, DISP32 values,
and readout entire structs if need be.



# Function Usage (eyestep_util.h)
Please note that "direction" can be direction::ahead, or direction::behind.
this determines if it will look forwards or backwards starting at "function".

nextprologue(function, direction);    goes to the start address of the very next function


nextcall(function, direction);        goes to the very next call instruction, returns the call function


getcalls(function);                   gets all calls used in a function; returns those functions in a list.


fsize(function);                      returns the overall size of a function in bytes


fretn(function);                      returns the stack size ret'd at the end of a function


getconv(function);                    returns the calling convention of a function (as an ID; e.g. 1 = conv_stdcall).


getsconv(conv);                       translates a convention ID to a string, e.g. 1 --> "stdcall"


debug(address, register, offset);     Places a hook, reading the value of [register+offset] at address. For example, if we have a mov ecx,[ebp+8] instruction at 0x12000000, and we do this: uint32_t x=debug(0x12000000, ebp, 8), as soon as that instruction is executed, you will have the value at the offset 8 from ebp. The hook is removed instantly after, preventing detection.



# Writing instructions

use eyestep::write(location, "[instruction here]");

Please note:
All values and offsets are hexidecimal (this is a limited parser atm) and MUST be aligned by 2.
For example do [ebx+0C] instead of [ebx+C].

Examples of usage + notes:

uint32_t addr = 0x12000000;
addr += eyestep::write(addr, "push ebp").len;
addr += eyestep::write(addr, "mov ebp,esp").len;
addr += eyestep::write(addr, "mov [00000040],ecx").len;
addr += eyestep::write(addr, "add [edx+0C],40").len; // technically "add byte ptr" since it adds 0x40
addr += eyestep::write(addr, "sbb ebx,40").len;
addr += eyestep::write(addr, "mov ebx,00000040").len; // use 4-byte int for mov
addr += eyestep::write(addr, "call base+1F20C0").len; // calls RobloxPlayerBeta.exe+1F20C0
addr += eyestep::write(addr, "jmp 12000030").len; // inserts a short jmp to 0x12000030
addr += eyestep::write(addr, "jmp 1300FF30").len; // inserts a long jmp to 0x1300FF30
addr += eyestep::write(addr, "pop ebp").len;
addr += eyestep::write(addr, "ret 04").len;
addr += eyestep::write(addr, "int3").len;

this can make writing asm in another process easier.
however it is completely in development still and
needs optimization, and some rewriting.





# Reading instructions

Instructions are interpreted like so:
eyestep::inst i = eyestep::read(0x847EFB);

or, full text disassembly can be done with two options:
std::string strasm = eyestep::sread(0x847EF0, 0x847F00); // start address, end address
and:
std::string strasm = eyestep::sread(0x847EF0, 10); // start address, number of instructions to read

printf("Output:\n\n%s\n", strasm.c_str());



Let's say we wanna dump a struct; and we have this instruction in a lua function:
.text:00847EFB                 mov eax, [esi+8]

We'll pretend esi is lua state, and we know this instruction uses the top offset of lua state. (L->top).
how can we grab the +8 which is going to be the offset to lua state's "top" property?

// first we read it with eyestep

eyestep::inst i = eyestep::read(0x847EFB);

// then, we can check if the instruction here has both operands (it does)

if (i.flags & Fl_src_dest){

  // then, we can check if the destination (second) operand uses an 8 bit offset ("imm8")
  
  if (i.flags & Fl_dest_imm8){

    // Now we know it uses an 8 bit offset; let's figure out what the offset is
    
    uint8_t offset = i.dest.imm8;
    
    printf("offset: +%i\n", offset); // offset: +8

  }
  
}


Notes:


i.src and i.dest have the exact same properties;

they are the first and second part of most instructions.

Here's an explanation of their properties:

dest.imm8 is an 8 bit(1 byte) offset from a register if there is one. (like mov eax,[edi+3C])

dest.imm32 is a 32 bit(4 byte/int) offset from a register if there is one. (like mov eax,[edi+0028CDBE])

dest.disp8 is an 8 bit(1 byte) fixed value. (like mov eax,3C)

dest.disp32 is a 32 bit(4 byte/int) fixed value. (like mov eax,0028CDBE)

dest.r32 is the first 32-bit register in the destination operand

dest.r_2 is the second register in the destination operand, following the same bit-size as the first one.
