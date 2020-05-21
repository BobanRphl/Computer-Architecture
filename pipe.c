/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 *
 * Reza Jokar and Gushu Li, 2016
 */
 
 
 /*********************************************
* EECE5610_Computer Architecture and Design  *
* Lab3                                       *
* Due 11-12-2018                             *
* Michael Tyburski                           *
* Boban Pallathucharry                       *
**********************************************/


#include "pipe.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/***********************global pipeline state*********************/
CPU_State CURRENT_STATE;

IF_ID fed;

ID_EX dex;

EX_MEM exme;

MEM_WB memrite;

uint32_t opMask = 0xFC000000;
uint32_t rsMask = 0x03E00000;
uint32_t rtMask = 0x001F0000;
uint32_t rdMask = 0x0000F800;
uint32_t shamtMask = 0x000007C0;
uint32_t functMask = 0x0000003F;
uint32_t immediateMask = 0x0000FFFF;
uint32_t addressMask = 0x03FFFFFF;

// variables to hold previous stage data 
uint32_t WriteData_Save;
int32_t WriteRegister_Save;
int32_t MEM_RegisterRd_Save;
int MEM_RegWrite_Save;
int WB_RegWrite_Save;

uint32_t op;
int32_t rd;
int32_t rs;
int32_t rt;
int32_t shamt;
int32_t funct;
int16_t immediate;
int32_t address;
int jump = 0;
int PCSrc = 0;

int Icount = 0;			// Instruction Count
int CycleCount = 5;		// Min. number of cycles =5
int stages = 5;			// 5 stage pipeline
int Flag = 1;			// Flag to signal end of instructions

void pipe_init()
{
    memset(&CURRENT_STATE, 0, sizeof(CPU_State));
    CURRENT_STATE.PC = 0x00400000;
}

void pipe_cycle()
{
	do
	{
		printf("Icount = 0x% " PRIx32 "\n", Icount);
		pipe_stage_wb();
		pipe_stage_mem();
		pipe_stage_execute();
		pipe_stage_decode();
		pipe_stage_fetch();
	}while(Icount != CycleCount);
	
	RUN_BIT = FALSE;
	printf("--------------RUN_BIT set to FALSE------------\n");
}

/***********************************************************************************/
/***************************************WRITE BACK STAGE****************************/
void pipe_stage_wb()
{
	printf("**********WriteBack Stage**********\n");
	printf("MEM/WB_RegWrite = 0x% " PRIx32 "\n", memrite.RegWrite);
	if(memrite.RegWrite)
	{
		printf("MEM/WB_MemtoReg = 0x% " PRIx32 "\n", memrite.MemtoReg);
		if(memrite.MemtoReg)
		{
			CURRENT_STATE.REGS[memrite.RegisterRd] = memrite.read_data;
			WriteData_Save = memrite.read_data;
			printf("Register = 0x% " PRIx32 "\n", memrite.RegisterRd);
			printf("Data = 0x% " PRIx32 "\n", memrite.read_data);
			//memrite.MemtoReg = 0;
		}
		else 
		{
			CURRENT_STATE.REGS[memrite.RegisterRd] = memrite.ALUCarry;
			WriteData_Save = memrite.ALUCarry;
			printf("Register = 0x% " PRIx32 "\n", memrite.RegisterRd);
			printf("Data = 0x% " PRIx32 "\n", memrite.ALUCarry);
			//memrite.MemtoReg = 0;
		}
	}
	
	WriteRegister_Save = memrite.RegisterRd;
	WB_RegWrite_Save = memrite.RegWrite;
	
/************************2:1 Multiplexer MemtoReg**********************/	
	//if(memrite.MemtoReg)		//control
	//	WriteData_Save = memrite.read_data;
	//else 
	//	WriteData_Save = memrite.ALUCarry;
/************************END 2:1 Multiplexer MemtoReg**********************/	

	
	//printf("**********WriteBack Stage**********\n");
	//printf("MEM/WB_RegWrite = 0x% " PRIx32 "\n", memrite.RegWrite);
	//printf("MEM/WB_MemtoReg = 0x% " PRIx32 "\n", memrite.MemtoReg);
	//printf("MEM/WB_ReadData = 0x% " PRIx32 "\n", memrite.read_data);
	//printf("MEM/WB_ALUCarry_IN = 0x% " PRIx32 "\n", memrite.ALUCarry);
	//printf("MEM/WB_RegisterRd = 0x% " PRIx32 "\n", memrite.RegisterRd);
	
}	

/*******************************************************************************/
/***************************************MEMORY STAGE****************************/
void pipe_stage_mem()
{	
	//CURRENT_STATE.PC = exme.effective_address;
	// forward control bits to write back stage
	memrite.RegWrite = exme.RegWrite;
	memrite.MemtoReg = exme.MemtoReg;
	MEM_RegWrite_Save = exme.RegWrite;
	MEM_RegisterRd_Save = exme.RegisterRd;
	
	
	if(exme.MemRead == 1) //instruction is "lw"
	{
		memrite.read_data = mem_read_32(exme.ALUResult);
		memrite.ALUCarry = exme.ALUResult;
		//exme.MemRead = 0;
	}
	if(exme.MemWrite == 1) //instruction is "sw"
	{
		mem_write_32(exme.ALUResult , exme.ForwardB_Out);
		//exme.MemWrite = 0;
	}
	
	memrite.ALUCarry = exme.ALUResult;
	memrite.RegisterRd = exme.RegisterRd;
	printf("**********Memory Stage**********\n");
	printf("EX/MEM_RegWrite = 0x% " PRIx32 "\n", exme.RegWrite);
	printf("EX/MEM_MemtoReg = 0x% " PRIx32 "\n", exme.MemtoReg);
	printf("EX/MEM_RegisterRd = 0x% " PRIx32 "\n", exme.RegisterRd);
}

/********************************************************************************/
/***************************************EXECUTE STAGE****************************/
void pipe_stage_execute()
{
	// forward the control bits to memory stage
	exme.Branch = dex.Branch;
	exme.MemRead = dex.MemRead;
	exme.MemWrite = dex.MemWrite;
	exme.RegWrite = dex.RegWrite;
	exme.MemtoReg = dex.MemtoReg;
	
	int32_t ForwardA_Out; // Input one to the ALU
	int32_t ForwardB_Out; // Input two to the ALU
	int32_t ALUSrc;
	
	int ForwardA;
	int ForwardB;
	
/************************Forwarding Unit**********************/
	
	printf("********Forwarding unit inputs *******\n");
	printf("WriteData_Save = 0x% " PRIx32 "\n", WriteData_Save);
	printf("EX/MEM_RegWrite = 0x% " PRIx32 "\n", exme.RegWrite);
	printf("EX/MEM_RegisterRd = 0x% " PRIx32 "\n", exme.RegisterRd);
	printf("ID/EX_RegisterRs = 0x% " PRIx32 "\n", dex.RegisterRs);
	printf("MEM/WB_RegWrite = 0x% " PRIx32 "\n", memrite.RegWrite);
	printf("WriteRegister_Save = 0x% " PRIx32 "\n", WriteRegister_Save);
	printf("ID/EX_RegisterRt = 0x% " PRIx32 "\n", dex.RegisterRt);
	printf("********Forwarding unit outputs*********\n");
		
	if((exme.RegWrite && (exme.RegisterRd != 0)) && (exme.RegisterRd == dex.RegisterRs))	// EX Hazard
		ForwardA = 2;
		else if((WB_RegWrite_Save && (WriteRegister_Save != 0))&& !(MEM_RegWrite_Save & (MEM_RegisterRd_Save != 0)) && (exme.RegisterRd != dex.RegisterRs) && (WriteRegister_Save == dex.RegisterRs))	// MEM Hazard
			ForwardA = 1;
			else
				ForwardA = 0;
	
	if((exme.RegWrite && (exme.RegisterRd != 0)) && (exme.RegisterRd == dex.RegisterRt))	// EX Hazard
		ForwardB = 2;
		else if((WB_RegWrite_Save && (WriteRegister_Save != 0))&&!(MEM_RegWrite_Save && (exme.RegisterRd != 0)) && (exme.RegisterRd != dex.RegisterRt) && (WriteRegister_Save == dex.RegisterRt))	// MEM Hazard
			ForwardB = 1;
			else
				ForwardB = 0;
/************************END Forwarding Unit**********************/

/************************First ALU Operand**********************/
	if(ForwardA == 2)
		ForwardA_Out = exme.ALUResult;
	else if(ForwardA == 1)
		ForwardA_Out = WriteData_Save;
	else
		ForwardA_Out = dex.data1;
	printf("ForwardA = 0x% " PRIx32 "\n", ForwardA);
	printf("ForwardA_Out = 0x% " PRIx32 "\n", ForwardA_Out);
/************************END First ALU Operand**********************/

/************************Second ALU Operand**********************/
	if(ForwardB == 2)
		ForwardB_Out = exme.ALUResult;
	else if(ForwardB == 1)
		ForwardB_Out = WriteData_Save;
	else
		ForwardB_Out = dex.data2;
	printf("ForwardB = 0x% " PRIx32 "\n", ForwardB);
	printf("ForwardB_Out = 0x% " PRIx32 "\n", ForwardB_Out);
/************************END Second ALU Operand**********************/

/************************2:1 Multiplexer ALUSrc**********************/
	if(dex.ALUSrc)
		ALUSrc = dex.immediate;
	else
		ALUSrc = ForwardB_Out;
/************************END 2:1 Multiplexer ALUSrc**********************/

	
	//*******NEED to UPDATE IF Case*******//
	if((dex.ALUOp1 == 1) && (dex.ALUOp0 == 0) && (funct == 0x20)) //instruction is "add"
	{
		exme.ALUResult = ForwardA_Out + ALUSrc;
		//dex.ALUOp1 = 0;
		//dex.ALUOp0 = 0;
	}
	
	/*if(op == 0 & funct == 0x21) //instruction is "addu"
	{
		//NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
		exme.ALUResult = dex.data1 + dex.data2;
	}
	
	if(op == 0 & funct == 0x22) //instruction is "sub"
	{
		//NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
		exme.ALUResult = dex.data1 - dex.data2;
	}
	
	if(op == 0 & funct == 0x23) //instruction is "subu"
	{
		//NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
		exme.ALUResult = dex.data1 - dex.data2;
	}
	
	if(op == 0 & funct == 0x2A) //instruction is "slt"
	{
		if(dex.data1 < dex.data2)
		{
			exme.ALUResult = 1;
		}
		else exme.ALUResult = 0;
	}
	
	if(op == 0 & funct == 0x2B) //instruction is "sltu"
	{
		if(dex.data1 < dex.data2)
		{
			exme.ALUResult = 1;
		}
		else exme.ALUResult = 0;
	}
	
	if(op == 0x2) // instruction is "jump"
	{
		exme.effective_address = dex.address + (dex.immediate*4);
		jump = 1;
	}
	
	if(op == 0x4) // instruction is "beq"
	{
		if(dex.data1 == dex.data2)
		{
			exme.effective_address = dex.address + (dex.immediate*4);
			jump = 1;
		}
	}
	
	if(op == 0x5) // instruction is "bne"
	{
		if(dex.data1 != dex.data2)
		{
			exme.effective_address = dex.address + (dex.immediate*4);
			jump = 1;
		}
	}
	
	if(op == 0x7) //instruction is "bgtz"
	{
		if(dex.data1 > 0)
		{
			exme.effective_address = dex.address + (dex.immediate*4);
			jump = 1;
		}
	}
	*/
	if((dex.ALUOp1 == 0) && (dex.ALUOp0 == 0) && (op == 0x8)) //instruction is "addi"
	{
		exme.ALUResult = ForwardA_Out + ALUSrc;
		//dex.ALUOp1 = 0;
		//dex.ALUOp0 = 0;
	}
	/*
	if(op == 0x9) //instruction is "addiu"
	{
		exme.ALUResult = dex.data1 + dex.immediate;
	}
	
	if(op == 0xA) //instruction is "slti"
	{
		if(dex.data1 < dex.immediate)
		{
			exme.ALUResult = 1;
		}
		else exme.ALUResult = 0;
	}
	
	if(op == 0xD) //instruction is "ori"
	{
		exme.ALUResult = dex.data1 | dex.immediate;
	}
	
	if(op == 0xF) //instruction is "lui"
	{
		exme.ALUResult = dex.immediate << 16;
	}
	
	if((op == 0x2 || op == 0x4 || op == 0x5 || op == 0x7) && jump == 1)
	{
		//do nothing, update PC per jump instructions
		jump = 0;
	}
	else exme.effective_address = dex.address;
	*/
	exme.ForwardB_Out = ForwardB_Out;
	printf("exme.ALUResult = 0x% " PRIx32 "\n\n", exme.ALUResult);
	
/************************2:1 Multiplexer RegDest**********************/	
	if(dex.RegDst)
		exme.RegisterRd = dex.RegisterRd; // 15 downto 11
	else 
		exme.RegisterRd = dex.RegisterRt; // 20 downto 16
/************************END 2:1 Multiplexer RegDest**********************/


	
	/*printf("**********Execute Stage**********\n");
	printf("MemtoReg = 0x% " PRIx32 "\n", dex.MemtoReg);
	printf("data1 = 0x% " PRIx32 "\n", dex.data1);
	printf("data2 = 0x% " PRIx32 "\n", dex.data2);
	printf("ALUResult = 0x% " PRIx32 "\n", exme.ALUResult);
	printf("ALUSrc = 0x% " PRIx32 "\n", dex.ALUSrc);
	printf("ALUSrcResult = 0x% " PRIx32 "\n", ALUSrcResult);*/
}

/*******************************************************************************/
/***************************************DECODE STAGE****************************/
void pipe_stage_decode() 
{
	if(Flag)
	{
		
		if(fed.Instruction != 0x0)
		{
			op = (fed.Instruction & opMask) >> 26;  //mask instruction to get op
			printf("op = 0x% " PRIx32 "\n", op);
	
			rd = (fed.Instruction & rdMask) >> 11;
			rs = (fed.Instruction & rsMask) >> 21;
			rt = (fed.Instruction & rtMask) >> 16;
	
/************************Hazard Detection Unit**********************/
			if(dex.MemRead && ((dex.RegisterRt == rs) || (dex.RegisterRt == rt))) //stall the pipeline
			{
				printf("-------------Stall the pipeline-------------------\n");
				dex.RegDst = 0;
				dex.ALUOp1 = 0;
				dex.ALUOp0 = 0;
				dex.ALUSrc = 0;
				dex.Branch = 0;
				dex.MemRead = 0;
				dex.MemWrite = 0;
				dex.RegWrite = 0;
				dex.MemtoReg = 0;
			}
			else
			{
/************************END Hazard Detection Unit**********************/	
	
			//dex.RegisterRd = rd;
			//dex.RegisterRs = rs;
			//dex.RegisterRt = rt;
	
				if(op == 0)  //Instruction is R Type
				{
					printf("-------------Instruction is R Type-------------------\n");
					funct = fed.Instruction & functMask;	//get function code
					dex.RegisterRd = rd;	// Set destination register address
					dex.RegisterRt = rt;
					dex.RegisterRs = rs;
					dex.data1 = CURRENT_STATE.REGS[rs];	// read data from source 1(rs) register address
					dex.data2 = CURRENT_STATE.REGS[rt];	// read data from source 2(rt) register address
					//dex.immediate = 0;
		
					// set control bits
					dex.RegDst = 1;
					dex.ALUOp1 = 1;
					dex.ALUOp0 = 0;
					dex.ALUSrc = 0;
					dex.Branch = 0;
					dex.MemRead = 0;
					dex.MemWrite = 0;
					dex.RegWrite = 1;
					dex.MemtoReg = 0;
		
				}
				else if(op == 0x2) //Instruction is "jump"
				{
					address = fed.Instruction & addressMask;	//mask for jump address
					dex.immediate = address;
					dex.data1 = 0;
					dex.data2 = 0;
					dex.RegisterRt = 0;
					dex.RegisterRd = 0;	
		
					//set control bits for jump
					dex.RegDst = 1;
					dex.ALUOp1 = 0;
					dex.ALUOp0 = 1;
					dex.ALUSrc = 0;
					dex.Branch = 1;
					dex.MemRead = 0;
					dex.MemWrite = 0;
					dex.RegWrite = 0;
					dex.MemtoReg = 0;
				}
				else //by default instruction is I type
				{
					printf("-------------Instruction is I type-------------------\n");
					dex.RegisterRt = rt;
					dex.RegisterRs = rs;
					//dex.RegisterRd = 0;
					dex.data1 = CURRENT_STATE.REGS[rs];
					dex.data2 = CURRENT_STATE.REGS[rt];
					immediate = fed.Instruction & immediateMask;	//get immediate value to add
					dex.immediate = immediate;
				
					if(op == 0x8)	//Set addi control bits
					{	
						dex.RegDst = 0;
						dex.ALUOp1 = 0;
						dex.ALUOp0 = 0;
						dex.ALUSrc = 1;
						dex.Branch = 0;
						dex.MemRead = 0;
						dex.MemWrite = 0;
						dex.RegWrite = 1;
						dex.MemtoReg = 0;
					}
				}
			}
		dex.address = CURRENT_STATE.PC;
		}
		/*
		printf("**********Decode Stage Output**********\n");
		printf("memrite.RegisterRd = 0x% " PRIx32 "\n", memrite.RegisterRd);
		printf("WriteData_Save = 0x% " PRIx32 "\n", WriteData_Save);
		printf("Icount = 0x% " PRIx32 "\n", Icount);
		printf("CycleCount = 0x% " PRIx32 "\n", CycleCount);
		printf("memrite.RegWrite = 0x% " PRIx32 "\n", memrite.RegWrite);
		printf("rs = 0x% " PRIx32 "\n", rs);
		printf("data1 = 0x% " PRIx32 "\n", dex.data1);
		printf("rt = 0x% " PRIx32 "\n", rt);
		printf("data2 = 0x% " PRIx32 "\n", dex.data2);
		*/
	}
}

/******************************************************************************/
/***************************************FETCH STAGE****************************/
void pipe_stage_fetch() 
{
	if(Flag)
	{
		uint32_t fetchInst;
		printf("PC                : 0x%" PRIx32 "\n", CURRENT_STATE.PC);
		fetchInst = mem_read_32(CURRENT_STATE.PC); 								// fetch instruction pointed to by PC
		printf("fetchInst = 0x%" PRIx32 "\n", fetchInst);
		fed.Instruction = fetchInst;
		
		if(Flag)
		{
			if((fetchInst == 0) && (mem_read_32(CURRENT_STATE.PC+4)==0) && (mem_read_32(CURRENT_STATE.PC+8)==0))
			{
					CycleCount = (stages + Icount - 1);
					Flag = 0;
					printf("-------------Finishing Last Instruction-------------------\n");
			}
			else
			{
				CycleCount++;
			}
		}
	}
	
	Icount++;
	//fed.address = CURRENT_STATE.PC;
	CURRENT_STATE.PC = CURRENT_STATE.PC + 4;
	
}	