#ifndef SIM_TABLES_H
#define SIM_TABLES_H

#include <vector>
#include <limits.h>
using namespace std;

typedef unsigned long int ulong;
typedef unsigned int uint;

typedef struct ROBEntries{
    bool valid;
    long int value;
    long int dest;
    bool ready;
    bool exception;
    bool predictionMiss;
    ulong ProgCounter;

    ROBEntries(bool valid,long int value,long int dest,bool ready,bool exception,bool predictionMiss, ulong ProgCounter)
    {
        this->valid = valid;
        this->value = value;
        this->dest = dest;
        this->ready = ready;
        this->exception = exception;
        this->predictionMiss = predictionMiss;
        this->ProgCounter = ProgCounter;
    }

}ROB;

typedef struct IQEntries{
    bool valid;
    long int tagDestination;
    bool RS1Rdy;
    long int tagRS1;
    bool RS2Rdy;
    long int tagRS2;
    long int counter;
    ulong opcode;
    int optype;
    int cycles;
    ulong instructionCount;

    IQEntries(bool valid,long int tagDestination,bool RS1Rdy,long int tagRS1,bool RS2Rdy,long int tagRS2,long int counter,ulong opcode,int optype,int cycles,ulong instructionCount)
    {
        this->valid = valid;
        this->tagDestination = tagDestination;
        this->RS1Rdy = RS1Rdy;
        this->tagRS1 = tagRS1;
        this->RS2Rdy = RS2Rdy;
        this->tagRS2 = tagRS2;
        this->counter = counter;
        this->opcode = opcode;
        this->optype = optype;
        this->cycles = cycles;
        this->instructionCount = instructionCount;
    }

}IQ;

typedef struct tableRenameMapEntries{
    bool valid;
    long int tagROB;

    tableRenameMapEntries(bool valid,long int tagROB)
    {
        this->valid = valid;
        this->tagROB = tagROB;
        
    }
}tableRenameMapEntry;

typedef struct storeResultEntries
{
    unsigned int fu;
    pair<ulong,ulong> src;
    unsigned int dest;
    pair<ulong,ulong> FE,DE,RN,RR,DI,IS,EX,WB,RT;

    storeResultEntries(unsigned int fu,pair<ulong,ulong> src,unsigned int dest,pair<ulong,ulong> FE,pair<ulong,ulong> DE,pair<ulong,ulong> RN,pair<ulong,ulong> RR,pair<ulong,ulong> DI,pair<ulong,ulong> IS,pair<ulong,ulong> EX,pair<ulong,ulong> WB,pair<ulong,ulong> RT)
    {
    this->fu = fu;
    this->src = src;
    this->dest = dest;
    this->FE = FE;
    this->DE = DE;
    this->RN = RN;
    this->RR = RR;
    this->DI = DI;
    this->IS = IS;
    this->EX = EX;
    this->WB = WB;
    this->RT = RT;
    }

}results;

typedef struct registerDE
{
    bool flagFull;
    bool flagEmpty;
    unsigned int size;
    unsigned int sizeInst;
    registers *regDE;
}registerDE;

//Registers
registerDE DE, RN, RR, DI, EX, WB;

// Tables
ROB *tableROB;
IQ *tableIQ;
tableRenameMapEntry *tableRenameMap;
vector<results> tableResults;

ulong headROB,tailROB;
ulong pointerIQ;
ulong pointerEX;
ulong pointerWB;

#endif