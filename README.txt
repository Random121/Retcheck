This is EyeStep's final release.
It is now purely designed for internal/DLL application.

I made EyeStep to be the smallest, portable yet accurate
disassembler for x86 machine code.


The intention was to dump structs from memory by
being able to easily read offsets within the instruction.

Addresses in memory are translated to asm like so:

eyestep::inst i = eyestep::read(0xBADF00D);
printf("Disassembly: %s.\n", i.data);
// i is automatically freed up afterwards




Let's say you want to list all offsets of the ESI register,
for the first 20 instructions in a function:

int at = 0xDEADBEEF; // function address
for (int j = 0; j < 20; j++) {
    eyestep::inst i = eyestep::read(at);
    
    // printf("%s\n", i.data); 
    // /\ uncomment if you want to view all 20 instructions
    
    if (i.flags & Fl_dest_imm8){ // does this instruction have a BYTE sized(imm8) offset in the `dest` operand?
        if (i.dest.r32 == eyestep::reg_32::esi) { // does the `dest` operand use the ESI register?
            printf("%i.\n", i.dest.imm8); // then display the offset!
        }
    }
    at += i.len; // move to next instruction
}


Just a side note, I know that the disassembling code itself isn't very clean.
Basically, I understand it exactly how it works and that's what matters cause
it's my project.
So enjoy and feel free to report bugs to the commentary on this github!


