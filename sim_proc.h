#ifndef SIM_PROC_H
#define SIM_PROC_H

#include <vector>
#include <limits.h>
using namespace std;

typedef unsigned long int ulong;
typedef unsigned int uint;

typedef struct proc_params{
    ulong rob_size;
    ulong iq_size;
    ulong width;
}proc_params;

// Put additional data structures here as per your requirement

typedef struct registerEntries
{
    ulong opcode;
    long long int instructionCount;
    int optype;
    int cycles;
    long int rd,rs1,rs2;
    bool src1ROB,src2ROB;
    bool src1Rdy,src2Rdy;

    registerEntries(long long int instructionCount,ulong opcode,int optype,int cycles,long int rd,long int rs1,long int rs2,bool src1ROB,bool src2ROB,bool src1Rdy,bool src2Rdy)
    {
        this->instructionCount = instructionCount;
        this->opcode = opcode;
        this->optype = optype;
        this->cycles = cycles;
        this->rd = rd;
        this->rs1 = rs1;
        this->rs2 = rs2;
        this->src1ROB = src1ROB;
        this->src2ROB = src2ROB;
        this->src1Rdy = src1Rdy;
        this->src2Rdy = src2Rdy;
    }

}registers;

//Functions
bool Advance_Cycle();


//variables for simulator
ulong 	cycleCount			;	
int 	pipeInstructionCount;	
bool 	depletedTrace		;	
int 	scalarWidth			;	
uint 	sizeROB				;	
uint 	sizeIQ				;	
ulong 	programCounter		;	
int 	instCount			;	
ulong 	dynamicInstCount	;	


#endif