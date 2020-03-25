// 
// static's x86 disassembler, copyright 2020
// 
#ifndef H_EYESTEP
#define H_EYESTEP

#include <Windows.h>
#include <string>
#include <vector>

#define Fl_none					0x00000000
#define Fl_src_only				0x10000000
#define Fl_src_dest				0x20000000

#define Fl_src_r8				0x00000001
#define Fl_src_r16				0x00000002
#define Fl_src_r32   			0x00000004
#define Fl_src_imm8				0x00000008
#define Fl_src_imm16			0x00000010
#define Fl_src_imm32   			0x00000020
#define Fl_src_rm8				0x00000040
#define Fl_src_rm16				0x00000080
#define Fl_src_rm32				0x00000100
#define Fl_src_rxmm				0x00000200
#define Fl_src_disp8			0x00000400
#define Fl_src_disp16			0x00000800
#define Fl_src_disp32			0x00001000

#define Fl_dest_r8				0x00002000
#define Fl_dest_r16				0x00004000
#define Fl_dest_r32   			0x00008000
#define Fl_dest_imm8			0x00010000
#define Fl_dest_imm16			0x00020000
#define Fl_dest_imm32   		0x00040000
#define Fl_dest_rm8				0x00080000
#define Fl_dest_rm16			0x00100000
#define Fl_dest_rm32			0x00200000
#define Fl_dest_rxmm			0x00400000
#define Fl_dest_disp8			0x00800000
#define Fl_dest_disp16			0x40000000
#define Fl_dest_disp32			0x80000000

#define Fl_rel8					0x01000000
#define Fl_rel16				0x02000000
#define Fl_rel32				0x04000000

#define Fl_condition			0x08000000

#define PRE_BYTE_PTR			0x1
#define PRE_WORD_PTR			0x2
#define PRE_DWORD_PTR			0x4
#define PRE_QWORD_PTR			0x8

#define pre_repne   			0x1
#define pre_repe   				0x2
#define pre_66   				0x4
#define pre_67   				0x8
#define pre_lock 				0x10
#define pre_seg  				0x20
#define seg_cs					0x2E
#define seg_ss					0x36
#define seg_ds					0x3E
#define seg_es					0x26
#define seg_fs					0x64
#define seg_gs					0x65


#define addoff8(s,b,l,v)		char m[16]; v=*(uint8_t*)(b+l); (v<=0x7F)		? sprintf_s(m,"+%02X",v) : sprintf_s(m,"-%02X",(0xFF+1)-v);			strcat_s(s,m); l+=1
#define addoff16(s,b,l,v)		char m[16]; v=*(uint16_t*)(b+l);(v<=0x7FFF)		? sprintf_s(m,"+%04X",v) : sprintf_s(m,"-%04X",(0xFFFF+1)-v);		strcat_s(s,m); l+=2
#define addoff32(s,b,l,v)		char m[16]; v=*(uint32_t*)(b+l);(v<=0x7FFFFFFF)	? sprintf_s(m,"+%08X",v) : sprintf_s(m,"-%08X",(0xFFFFFFFF+1)-v);	strcat_s(s,m); l+=4
#define getr1(b,l)				(b[l] % 64 / 8)
#define getr2(b,l)				(b[l] % 64 % 8)
#define getmode20(b,l)			(b[l] / 0x20)
#define getmode40(b,l)			(b[l] / 0x40)

#define asm_out_none			0x0
#define asm_out_strings			0x1
#define asm_out_offs_1b			0x2
#define asm_out_offs_2b			0x4
#define asm_out_offs_4b			0x10

namespace eyestep {
	extern int base;

	namespace convert {
		// used for translating or converting hex-to-text, etc. and vice versa
		short to_short(BYTE a, BYTE b);
		int	to_int(BYTE a, BYTE b, BYTE c, BYTE d);
		int	pbtodw(BYTE* b);
		BYTE* dwtopb(UINT v);
		BYTE to_hex(char* x);
		std::string to_str(BYTE b);
		std::string to_str(int address);
		int to_addr(char* addr);
		std::string to_bytes(int addr);
		std::string to_bytes(const char* str);
	}

	class operand {
	public:
		BYTE r8; // 8 bit register
		BYTE r16; // 16 bit register
		BYTE r32; // 32 bit register
		BYTE rxmm; // 64 bit register
		BYTE r_2; // second register in source operand (same bit type as others) (use discretion/no flag indicator yet)
		BYTE mul; // multiplier (0 by default, if there is a multiplier it wont be 0)
		BYTE imm8; // 8 bit offset value
		USHORT imm16; // 16 bit offset value
		UINT imm32; // 32 bit offset value
		BYTE disp8;
		USHORT disp16;
		UINT disp32; // a fixed value (not an 'offset' (+/-))
		BYTE pref; // prefix for this operand (byte/dword/qword ptr, etc.)
	};


	class inst {
	public:
		inst();
		~inst();

		char data[128]; // readable output/translation of the instruction
		char opcode[16]; // just the instruction opcode
		void set_op(const char* x);

		UINT address;

		BYTE len; // overall length of the instruction in bytes (defaults to 1)
		BYTE p_rep; // rep
		BYTE p_lock; // lock
		BYTE p_seg; // seg
		BYTE r_mod; // register mode

		UINT pref; // global instruction prefix (cs/ds/lock/repne/etc.)
		UINT flags; // global instruction flags (which also determine src/dest values)

		BYTE rel8; // 8 bit relative value (for jmp or call)
		USHORT rel16; // 16 bit relative value
		UINT rel32; // 32 bit relative value

		BYTE condition; // if Fl_condition flag is set, so is this (if its a ja/jne/jnl, this will be set to the condition enum a/ne/nl)

		operand src;
		operand dest;
	};

	enum reg_8	 {  al,    cl,    dl,    bl,    ah,    ch,    dh,    bh   };
	enum reg_16	 {  ax,    bx,    cx,    dx,    sp,    bp,    si,    di   };
	enum reg_32	 {  eax,   ecx,   edx,   ebx,   esp,   ebp,   esi,   edi  };
	enum reg_xmm {  xmm0,  xmm1,  xmm2,  xmm3,  xmm4,  xmm5,  xmm6,  xmm7 };
	enum conds   { o, no, b, nb, e, ne, na, a, s, ns, p, np, l, nl, le, g };
	
	extern const char* c_reg1;
	extern const int* c_reg2;
	extern const char* c_reg_8[8];
	extern const char* c_reg_16[8];
	extern const char* c_reg_32[8];
	extern const char*c_reg_xmm[8];
	extern const char* c_conds[16];
	const BYTE mults[] = { 0, 2, 4, 8 };
	
	// Returns the equivalent x86 instruction at the given address
	inst read(int);
}

#endif
