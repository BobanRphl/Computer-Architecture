/*********************************************
* EECE5610_Computer Architecture and Design  *
* Lab3                                       *
* Due 11-12-2018                             *
* Michael Tyburski                           *
* Boban Pallathucharry                       *
**********************************************/

#ifndef _PIPE_H_
#define _PIPE_H_

#include "shell.h"
#include "stdbool.h"
#include <limits.h>

#define MIPS_REGS 32

typedef struct CPU_State_Struct {
  uint32_t PC;		/* program counter */
  int32_t REGS[MIPS_REGS]; /* register file. */
  int FLAG_N;        /* negative flag or sign flag*/
  int FLAG_Z;        /* zero flag */
  int FLAG_V;        /* overflow flag */
  int FLAG_C;        /* carry flag */
} CPU_State;

int RUN_BIT;

typedef struct pipereg_IF_ID 
{
	uint32_t Instruction;	//Holds instruction that was read during fetch
	uint32_t address;		//PC + 4
}IF_ID;

typedef struct pipereg_ID_EX {
	int32_t immediate;		//hold the sign extended immediate value
	int32_t RegisterRt;			// register to hold Instruction(20:16)(Rt)(I-type)
	int32_t RegisterRd;			// register to hold Instruction(15:11)(Rd)(R-type)
	int32_t RegisterRs;		// register to hold Instruction(25:21)(Rs)
	int32_t data1;			// register1 read data
	int32_t data2;			// register2 read data
	uint32_t address;		//PC + 4, carried from IF_ID register
	// control bits
	int		RegDst;
	int		ALUOp1;
	int		ALUOp0;
	int		ALUSrc;
	int		Branch;
	int		MemRead;
	int		MemWrite;
	int		RegWrite;
	int		MemtoReg;
}ID_EX;

typedef struct pipereg_EX_MEM {
	uint32_t effective_address;	// calculated address
	int32_t ALUResult;			
	int ALUZero;
	int32_t ForwardB_Out;			// data from register2 read, bypass MUX(ALUSrc)
	uint32_t RegisterRd;			// destination register, from MUX(RegDst)
	// control bits
	int		Branch;
	int		MemRead;
	int		MemWrite;
	int		RegWrite;
	int		MemtoReg;
}EX_MEM;

 typedef struct pipereg_MEM_WB {
	int32_t read_data;			// data read from memory
	int32_t ALUCarry;			// carry ALUResult from EX_MEM register
	uint32_t RegisterRd;			// carry destination register from EX_MEM
	// control bits
	int		RegWrite;
	int		MemtoReg;
}MEM_WB;

/* global variable -- pipeline state */
extern CPU_State CURRENT_STATE;

/* called during simulator startup */
void pipe_init();

/* this function calls the others */
void pipe_cycle();

/* each of these functions implements one stage of the pipeline */
void pipe_stage_fetch();
void pipe_stage_decode();
void pipe_stage_execute();
void pipe_stage_mem();
void pipe_stage_wb();
#endif
