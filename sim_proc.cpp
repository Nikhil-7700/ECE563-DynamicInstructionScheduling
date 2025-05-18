#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "sim_proc.h"
#include "sim_tables.h"

void simConfiguration(int rob_size,int iq_size,int width)
{
    tableROB = (ROB *)malloc(rob_size * sizeof(ROB));
	tableIQ = (IQ *)malloc(iq_size * sizeof(IQ));
	tableRenameMap = (tableRenameMapEntry *)malloc(67 * sizeof(tableRenameMapEntry));
	
	if (tableROB == NULL || tableIQ == NULL || tableRenameMap == NULL) {
		// Handle memory allocation failure
		perror("Memory allocation failed");
		exit(EXIT_FAILURE); // Or perform error handling as needed
	}
	
	//Ensure rob_size, iq_size, and 67 are valid values
	if (rob_size <= 0 || iq_size <= 0) {
		// Handle invalid size values
		fprintf(stderr, "Invalid size values for memory allocation\n");
		exit(EXIT_FAILURE); // Or perform error handling as needed
	}
	
    for(int i=0;i<(int)rob_size;i++) {tableROB[i] = ROB(false,0,0,false,false,false,0);} 
    for(int i=0;i<(int)iq_size;i++) {tableIQ[i] = tableIQ[i] = IQ(false,0,false,0,false,0,0,0,0,0,0);}    
    for(int i=0;i<67;i++) {tableRenameMap[i] = tableRenameMapEntries(false,0);}

    DE.regDE = (registers *)malloc(width * sizeof(registers));
    RN.regDE = (registers *)malloc(width * sizeof(registers));
    RR.regDE = (registers *)malloc(width * sizeof(registers));
    DI.regDE = (registers *)malloc(width * sizeof(registers));
    EX.regDE = (registers *)malloc(width*5 * sizeof(registers));
    WB.regDE = (registers *)malloc(width*5 * sizeof(registers));
	
	DE.flagFull = RN.flagFull = RR.flagFull = DI.flagFull = WB.flagFull = EX.flagFull = false;
    DE.flagEmpty = RN.flagEmpty = RR.flagEmpty = DI.flagEmpty = WB.flagEmpty = EX.flagEmpty = true;
	DE.sizeInst = RN.sizeInst = RR.sizeInst = DI.sizeInst = EX.sizeInst = WB.sizeInst = 0;  
		
    DE.size = RN.size = RR.size = DI.size = width;
	EX.size = WB.size = width*5; 

    for(int i=0;i<width;i++)
		DE.regDE[i] = RN.regDE[i] = RR.regDE[i] = DI.regDE[i] = registers(-1,0,0,0,0,0,0,0,0,0,0);

    for(int i=0;i<width*5;i++)
    {
        EX.regDE[i] = registers(-1,0,0,-1,0,0,0,0,0,0,0);
        WB.regDE[i] = registers(-1,0,0,0,0,0,0,0,0,0,0);
    } 

    cycleCount = 0;
    pipeInstructionCount = 0;
	instCount = 0;
	dynamicInstCount = 0;
    depletedTrace = false;
    scalarWidth = width;
    sizeROB = rob_size;
    sizeIQ = iq_size;
	programCounter = 0;
    headROB = tailROB = 0;
    pointerIQ = pointerEX = pointerWB = 0;
}

void print_results()
{
    for(int i=0;i<tableResults.size();i++)
    {
		cout << i << " ";
		cout << "fu{" << tableResults[i].fu << "} ";
		cout << "src{" << (int)tableResults[i].src.first << "," << (int)tableResults[i].src.second << "} ";
		cout << "dst{" << (int)tableResults[i].dest << "} ";
		cout << "FE{" << tableResults[i].FE.first << "," << tableResults[i].FE.second << "} ";
		cout << "DE{" << tableResults[i].DE.first << "," << tableResults[i].DE.second << "} ";
		cout << "RN{" << tableResults[i].RN.first << "," << tableResults[i].RN.second << "} ";
		cout << "RR{" << tableResults[i].RR.first << "," << tableResults[i].RR.second << "} ";
		cout << "DI{" << tableResults[i].DI.first << "," << tableResults[i].DI.second << "} ";
		cout << "IS{" << tableResults[i].IS.first << "," << tableResults[i].IS.second << "} ";
		cout << "EX{" << tableResults[i].EX.first << "," << tableResults[i].EX.second << "} ";
		cout << "WB{" << tableResults[i].WB.first << "," << tableResults[i].WB.second << "} ";
		cout << "RT{" << tableResults[i].RT.first << "," << tableResults[i].RT.second << "} ";
		cout << endl;
    }
}

bool ROBfull()
{
    int count = 0;
    for(int i=0;i<sizeROB;i++)
        if(!tableROB[i].valid)
            count+=1;
    
    if(count >= scalarWidth)
        return false;
    return true;
}

bool IQfull()
{
    int count = 0;
    for(int i=0;i<sizeIQ;i++)
        if(!tableIQ[i].valid)
            count += 1;

    if(count >= scalarWidth)
        return false;
    return true;
}

bool IQempty()
{
    int count = 0;
    for(int i=0;i<sizeIQ;i++)
        if(!tableIQ[i].valid)
            count += 1;

    if(count != sizeIQ)
        return false;
    return true;
}

void updateROB(long int DestReg, long long int ProgCounter){
	tableROB[tailROB].valid = true;
    tableROB[tailROB].dest = DestReg;
    tableROB[tailROB].value =  -1;
    tableROB[tailROB].ready = false;
    tableROB[tailROB].exception = false;
    tableROB[tailROB].predictionMiss = false;
    tableROB[tailROB].ProgCounter = ProgCounter;
    tailROB = (tailROB + 1) % sizeROB;
}

void advanceInstRN2RR(registerDE *src, registerDE *tar){
	for(int i=0;i<scalarWidth;i++)
    {
        int src1 = -1 ,src2 = -1,dest = -1;
        src1 = src->regDE[i].rs1;
        src2 = src->regDE[i].rs2;

        tar->regDE[i].src1ROB = false;
        tar->regDE[i].src2ROB = false;
        tar->regDE[i].src1Rdy = false;
        tar->regDE[i].src2Rdy = false;

        if(src->regDE[i].rs1!=-1)
        {
            if(tableRenameMap[src->regDE[i].rs1].valid)
            {
                src1 = tableRenameMap[src->regDE[i].rs1].tagROB;
                tar->regDE[i].src1ROB = true;
            }
        }

        if(src->regDE[i].rs2!=-1)
        {
            if(tableRenameMap[src->regDE[i].rs2].valid) 
            {
                src2 = tableRenameMap[src->regDE[i].rs2].tagROB;
                tar->regDE[i].src2ROB = true;
            }
        }

		if(src->regDE[i].rd == -1)
        {
			dest = src->regDE[i].rd;
		}
		else 
		{
			tableRenameMap[src->regDE[i].rd].valid = true;
            tableRenameMap[src->regDE[i].rd].tagROB = tailROB;
            dest = tailROB;
		}
		
		updateROB(src->regDE[i].rd, src->regDE[i].instructionCount);
		
		if(src->regDE[i].instructionCount != -1 && tableResults[src->regDE[i].instructionCount].RN.first == 0)
            tableResults[src->regDE[i].instructionCount].RN = make_pair(cycleCount,1);
        else if(src->regDE[i].instructionCount != -1)
            tableResults[src->regDE[i].instructionCount].RN.second += 1;
        
        tar->regDE[i].instructionCount = src->regDE[i].instructionCount;
        tar->regDE[i].cycles = src->regDE[i].cycles;
        tar->regDE[i].opcode = src->regDE[i].opcode;
        tar->regDE[i].optype = src->regDE[i].optype;
        tar->regDE[i].rd = dest;
        tar->regDE[i].rs1 = src1;
        tar->regDE[i].rs2 = src2;

        src->regDE[i].instructionCount = -1;
        src->regDE[i].cycles = -1;
        src->regDE[i].opcode = 0;
        src->regDE[i].optype = 0;
        src->regDE[i].rd = 0;
        src->regDE[i].rs1 = 0;
        src->regDE[i].rs2 = 0;
        src->regDE[i].src1ROB = false;
        src->regDE[i].src2ROB = false;
        src->regDE[i].src1Rdy = false;
        src->regDE[i].src2Rdy = false;

    }
}

void advanceInstWB2ROB(registerDE *src){
	for(int i=0;i<src->size;i++)
    {
        int numInst = src->regDE[i].instructionCount;
        for(int j=0;j<sizeROB;j++)
        {
            if(tableROB[j].valid == true && tableROB[j].ready == false && tableROB[j].ProgCounter == numInst)
            {
                tableROB[j].ready = true;

                if(tableRenameMap[tableROB[j].dest].valid == true && tableRenameMap[tableROB[j].dest].tagROB == j)
                    tableRenameMap[tableROB[j].dest].valid = false;

                if(numInst != -1)
                    tableResults[numInst].WB = make_pair(cycleCount,1);
                src->sizeInst -= 1;
                if(src->sizeInst == 0)
                    src->flagEmpty = true;

                src->regDE[i].cycles = 0;
                src->regDE[i].instructionCount = -1;
                src->regDE[i].opcode = 0;
                src->regDE[i].optype = 0;
                src->regDE[i].rd = 0;
                src->regDE[i].rs1 = 0;
                src->regDE[i].rs2 = 0;
            }
        }
    }
}

void advanceInstIQ2EX(registerDE *tar){
	for(int j=0;j<scalarWidth;j++)
	{
        {
            int valMin = INT_MAX;
            int numInst = -2;
            int invalidInst = -1;
            for(int i=0;i<sizeIQ;i++)
            {
                if(tableIQ[i].valid && (tableIQ[i].RS1Rdy && tableIQ[i].RS2Rdy) && (tableIQ[i].counter < valMin))
                {
                    
                    tar->regDE[pointerEX].instructionCount = tableIQ[i].instructionCount;
                    tar->regDE[pointerEX].cycles = tableIQ[i].cycles;
                    tar->regDE[pointerEX].opcode = tableIQ[i].opcode;
                    tar->regDE[pointerEX].optype = tableIQ[i].optype;
                    tar->regDE[pointerEX].rd = tableIQ[i].tagDestination;
                    tar->regDE[pointerEX].rs1 = tableIQ[i].tagRS1;
                    tar->regDE[pointerEX].rs2 = tableIQ[i].tagRS2;
                    valMin = tableIQ[i].counter;
                    numInst = tableIQ[i].instructionCount;
                    invalidInst = i;
                }
                
            }
            if(numInst!=-2)
            {   
                tableIQ[invalidInst].valid = false;
                tableIQ[invalidInst].tagDestination = 0;
                tableIQ[invalidInst].opcode =0;
                tableIQ[invalidInst].optype = 0;
                tableIQ[invalidInst].cycles = 0;
                tableIQ[invalidInst].instructionCount = -1;
                pointerEX = (pointerEX + 1) % tar->size;
            }
           
        }
    }
}

void advanceInstDI2IQ(registerDE *src){
	for(int i=0;i<scalarWidth;i++)
    {
        while(tableIQ[pointerIQ].valid == true)
            pointerIQ = (pointerIQ + 1) % sizeIQ;
		
        tableIQ[pointerIQ].valid = true;
        tableIQ[pointerIQ].tagDestination = src->regDE[i].rd;
        tableIQ[pointerIQ].opcode = src->regDE[i].opcode;
        tableIQ[pointerIQ].optype = src->regDE[i].optype;
        tableIQ[pointerIQ].cycles = src->regDE[i].cycles;
        tableIQ[pointerIQ].instructionCount = src->regDE[i].instructionCount;

        int valSrc1 = src->regDE[i].rs1;
        int valSrc2 = src->regDE[i].rs2;
        
        if(src->regDE[i].instructionCount != -1 && tableResults[src->regDE[i].instructionCount].DI.first == 0)
            tableResults[src->regDE[i].instructionCount].DI = make_pair(cycleCount,1);
        else if(src->regDE[i].instructionCount != -1)
			tableResults[src->regDE[i].instructionCount].DI.second += 1;

        tableIQ[pointerIQ].counter = instCount;
        instCount++;

        

        if(valSrc1 != -1)
        {
            if(valSrc1 < sizeROB && tableROB[valSrc1].valid && src->regDE[i].src1ROB)
            {
                tableIQ[pointerIQ].RS1Rdy = tableROB[valSrc1].ready;
                tableIQ[pointerIQ].tagRS1 = valSrc1;
            }
            else
            {
                tableIQ[pointerIQ].RS1Rdy = true;
                tableIQ[pointerIQ].tagRS1 = valSrc1;
            }

            if(src->regDE[i].src1Rdy)
                tableIQ[pointerIQ].RS1Rdy = true;

        }
        else
        {
            tableIQ[pointerIQ].RS1Rdy = true;
            tableIQ[pointerIQ].tagRS1 = 0;
        }

       if(valSrc2 != -1)
       {
            if(valSrc2 < sizeROB && tableROB[valSrc2].valid && src->regDE[i].src2ROB)
            {
                tableIQ[pointerIQ].RS2Rdy = tableROB[valSrc2].ready;
                tableIQ[pointerIQ].tagRS2 = valSrc2;
            }
            else
            {
                tableIQ[pointerIQ].RS2Rdy = true;
                tableIQ[pointerIQ].tagRS2 = valSrc2;
            }
            if(src->regDE[i].src2Rdy)
                tableIQ[pointerIQ].RS2Rdy = true;
       }

       else
       {
            tableIQ[pointerIQ].RS2Rdy = true;
            tableIQ[pointerIQ].tagRS2 = 0;
       }

       pointerIQ = (pointerIQ + 1) % sizeIQ;

       src->regDE[i].instructionCount = -1;
       src->regDE[i].cycles = -1;
       src->regDE[i].opcode = 0;
       src->regDE[i].optype = 0;
       src->regDE[i].rd = 0;
       src->regDE[i].rs1 = 0;
       src->regDE[i].rs2 = 0;
    }
}

void advanceInstEX2WB(registerDE *src, registerDE *tar){
	for(int i=0;i<src->size;i++)
    {
        if(src->regDE[i].cycles == 0)
        {
            tar->regDE[pointerWB].cycles = src->regDE[i].cycles;
            tar->regDE[pointerWB].instructionCount = src->regDE[i].instructionCount;
            tar->regDE[pointerWB].opcode = src->regDE[i].opcode;
            tar->regDE[pointerWB].optype = src->regDE[i].optype;
            tar->regDE[pointerWB].rd = src->regDE[i].rd;
            tar->regDE[pointerWB].rs1 = src->regDE[i].rs1;
            tar->regDE[pointerWB].rs2 = src->regDE[i].rs2;

            for(int j=0;j<sizeIQ;j++)
            {
                if(tableIQ[j].RS1Rdy == false && tableIQ[j].tagRS1 == tar->regDE[pointerWB].rd)
                    tableIQ[j].RS1Rdy = true;
                if(tableIQ[j].RS2Rdy == false && tableIQ[j].tagRS2 == tar->regDE[pointerWB].rd)
                    tableIQ[j].RS2Rdy = true;
            }

            for(int j=0;j<scalarWidth;j++)
            {
                if((RR.regDE[j].rs1 == tar->regDE[pointerWB].rd) && (RR.regDE[j].src1Rdy == false))
                    RR.regDE[j].src1Rdy = true;
                if((RR.regDE[j].rs2 == tar->regDE[pointerWB].rd) && (RR.regDE[j].src2Rdy == false))
                    RR.regDE[j].src2Rdy = true;
                if((DI.regDE[j].rs1 == tar->regDE[pointerWB].rd) && (DI.regDE[j].src1Rdy == false))
                    DI.regDE[j].src1Rdy = true;
                if((DI.regDE[j].rs2 == tar->regDE[pointerWB].rd) && (DI.regDE[j].src2Rdy == false))
                    DI.regDE[j].src2Rdy = true;
            }
			
			pointerWB = (pointerWB + 1) % WB.size;
			tar->sizeInst += 1;
			tar->flagEmpty = false;
			
			src->regDE[i].cycles = -1;
			src->regDE[i].instructionCount = -1;
			src->regDE[i].opcode = 0;
			src->regDE[i].optype = 0;
			src->regDE[i].rd = 0;
			src->regDE[i].rs1 = 0;
			src->regDE[i].rs2 = 0;
            

            
        }
    }
}

void advanceInstDE2RN(registerDE *src, registerDE *tar)
{
    for(int i=0;i<scalarWidth;i++)
    {
        tar->regDE[i].instructionCount = src->regDE[i].instructionCount;
        tar->regDE[i].cycles = src->regDE[i].cycles;
        tar->regDE[i].opcode = src->regDE[i].opcode;
        tar->regDE[i].optype = src->regDE[i].optype;
        tar->regDE[i].rd = src->regDE[i].rd;
        tar->regDE[i].rs1 = src->regDE[i].rs1;
        tar->regDE[i].rs2 = src->regDE[i].rs2;
        tar->regDE[i].src1ROB = src->regDE[i].src1ROB;
        tar->regDE[i].src2ROB = src->regDE[i].src2ROB;
        tar->regDE[i].src1Rdy = src->regDE[i].src1Rdy;
        tar->regDE[i].src2Rdy = src->regDE[i].src2Rdy;

        src->sizeInst -= 1;

        if(tar->regDE[i].instructionCount != -1 && tableResults[tar->regDE[i].instructionCount].DE.first == 0)
            tableResults[tar->regDE[i].instructionCount].DE = make_pair(cycleCount,1);
        else if(tar->regDE[i].instructionCount != -1)
            tableResults[tar->regDE[i].instructionCount].DE.second += 1;
        

        src->regDE[i].instructionCount = -1;
        src->regDE[i].cycles = -1;
        src->regDE[i].opcode = 0;
        src->regDE[i].optype = 0;
        src->regDE[i].rd = 0;
        src->regDE[i].rs1 = 0;
        src->regDE[i].rs2 = 0;
        src->regDE[i].src2ROB = 0;
        src->regDE[i].src1ROB = 0;
        src->regDE[i].src1Rdy = false;
        src->regDE[i].src2Rdy = false;
    }
}

void advanceInstRR2DI(registerDE *src, registerDE *tar)
{
    for(int i=0;i<scalarWidth;i++)
    {
        tar->regDE[i].instructionCount = src->regDE[i].instructionCount;
        tar->regDE[i].cycles = src->regDE[i].cycles;
        tar->regDE[i].opcode = src->regDE[i].opcode;
        tar->regDE[i].optype = src->regDE[i].optype;
        tar->regDE[i].rd = src->regDE[i].rd;
        tar->regDE[i].rs1 = src->regDE[i].rs1;
        tar->regDE[i].rs2 = src->regDE[i].rs2;
        tar->regDE[i].src1ROB = src->regDE[i].src1ROB;
        tar->regDE[i].src2ROB = src->regDE[i].src2ROB;
        tar->regDE[i].src1Rdy = src->regDE[i].src1Rdy;
        tar->regDE[i].src2Rdy = src->regDE[i].src2Rdy;

		if(src->regDE[i].instructionCount != -1 && tableResults[src->regDE[i].instructionCount].RR.first == 0)
            tableResults[src->regDE[i].instructionCount].RR = make_pair(cycleCount,1);
        else if(RR.regDE[i].instructionCount != -1)
            tableResults[src->regDE[i].instructionCount].RR.second += 1;

        if(tableROB[src->regDE[i].rs1].valid && tableROB[src->regDE[i].rs1].ready && tar->regDE[i].src1Rdy == false && tar->regDE[i].src1ROB == true)
            tar->regDE[i].src1Rdy = true;

        if(tableROB[src->regDE[i].rs2].valid && tableROB[src->regDE[i].rs2].ready && tar->regDE[i].src2Rdy == false && tar->regDE[i].src2ROB == true)
            tar->regDE[i].src2Rdy = true;

        src->regDE[i].instructionCount = -1;
        src->regDE[i].cycles = -1;
        src->regDE[i].opcode = 0;
        src->regDE[i].optype = 0;
        src->regDE[i].rd = 0;
        src->regDE[i].rs1 = 0;
        src->regDE[i].rs2 = 0;
        src->regDE[i].src2ROB = 0;
        src->regDE[i].src1ROB = 0;
        src->regDE[i].src1Rdy = false;
        src->regDE[i].src2Rdy = false;
        
    }
}

/*********************************************************************************/
/*********************************************************************************/
/****************************** PIPELINE FUNCTIONS *******************************/
bool Advance_Cycle()
{
    cycleCount++;
    if((depletedTrace) && (pipeInstructionCount <= 0))
        return false;
    return true;
}

void retire()
{    
    for(int k=0;k<sizeROB;k++)
	{
        if(tableROB[k].valid == true && tableROB[k].ready == true)
            if(tableROB[k].ProgCounter != -1 && tableResults[tableROB[k].ProgCounter].RT.first == 0)
                tableResults[tableROB[k].ProgCounter].RT = make_pair(cycleCount,1);
            else if(tableROB[k].ProgCounter != -1)
                tableResults[tableROB[k].ProgCounter].RT.second += 1;
	}

    for(int i=0;i<scalarWidth;i++)
        if(tableROB[headROB].ready == true)
        {
            tableROB[headROB].valid = false;
            headROB = (headROB + 1) % sizeROB;
            pipeInstructionCount -= 1;
        }
}

void writeback()
{
    if(!WB.flagEmpty)
		advanceInstWB2ROB(&WB);
    return;
}


void execute()
{
    if(!EX.flagEmpty)
    {
        for(int i=0;i<EX.size;i++)
        {
            if(EX.regDE[i].cycles > 0)
                EX.regDE[i].cycles -= 1;
            
            int numInst = EX.regDE[i].instructionCount;
            if(numInst != -1 && tableResults[numInst].EX.first == 0)
            {
                int cycle_num = (EX.regDE[i].optype == 0) ? 1 : ((EX.regDE[i].optype == 1) ? 2 : 5);
                tableResults[numInst].EX = make_pair(cycleCount,cycle_num);
            }
        }
		advanceInstEX2WB(&EX, &WB);
		
    }
    return;
}

void issue()
{    
    if(!IQempty())
    {
        for(int x=0;x<sizeIQ;x++)
        {
            int numInst = tableIQ[x].instructionCount;
            if(numInst != -1 && tableIQ[x].valid == true && tableResults[numInst].IS.first == 0) 
                tableResults[numInst].IS = make_pair(cycleCount,1);
            else if(numInst != -1 && tableIQ[x].valid == true)
                tableResults[numInst].IS.second += 1;
        }
        
		advanceInstIQ2EX(&EX);
        EX.flagEmpty = false;
        EX.flagFull = false;
    } 
    return;

}

void dispatch()
{
    if(!DI.flagEmpty)
    {
        if(IQfull())
        {
            for(int i=0;i<scalarWidth;i++)
            {
                if(DI.regDE[i].instructionCount != -1 && tableResults[DI.regDE[i].instructionCount].DI.first == 0)
                    tableResults[DI.regDE[i].instructionCount].DI = make_pair(cycleCount,1);
                else if(DI.regDE[i].instructionCount != -1)
                    tableResults[DI.regDE[i].instructionCount].DI.second += 1;
            }
            return;
        }
        else
        {	
			advanceInstDI2IQ(&DI);
            DI.flagEmpty = true;
            DI.flagFull = false;
        }
    }
}

void regRead()
{
    if(!RR.flagEmpty)
    {
        if(DI.flagFull)
        {
            for(int i=0;i<scalarWidth;i++)
            {
                if(RR.regDE[i].instructionCount != -1 && tableResults[RR.regDE[i].instructionCount].RR.first == 0)
                    tableResults[RR.regDE[i].instructionCount].RR = make_pair(cycleCount,1);
                else if(RR.regDE[i].instructionCount != -1)
                    tableResults[RR.regDE[i].instructionCount].RR.second += 1;
            }
            return;
        }
        else
        {
            advanceInstRR2DI(&RR, &DI);
            RR.flagEmpty = true;
            RR.flagFull = false;
            DI.flagEmpty = false;
            DI.flagFull = true; 
        }
    }
    
    return;
   
}

void rename()
{
    if(!RN.flagEmpty)
    {
        if(RR.flagFull || ROBfull())
        {
            for(int i=0;i<scalarWidth;i++)
            {
                if(RN.regDE[i].instructionCount != -1 && tableResults[RN.regDE[i].instructionCount].RN.first == 0)
                    tableResults[RN.regDE[i].instructionCount].RN = make_pair(cycleCount,1);
                else if(RN.regDE[i].instructionCount != -1)
                    tableResults[RN.regDE[i].instructionCount].RN.second += 1;
            }
            return;
        }
        else
        {
			advanceInstRN2RR(&RN, &RR);
            RN.flagEmpty = true;
            RN.flagFull = false;
            RR.flagEmpty = false;
            RR.flagFull = true;
        }
    } 
    return;
}

void decode()
{
    if(!DE.flagEmpty)
    {
        if(RN.flagFull)
        {
            for(int i=0;i<scalarWidth;i++)
            {
                if(DE.regDE[i].instructionCount != -1 && tableResults[DE.regDE[i].instructionCount].DE.first == 0)
                    tableResults[DE.regDE[i].instructionCount].DE = make_pair(cycleCount,1);
                else if(DE.regDE[i].instructionCount != -1)
                    tableResults[DE.regDE[i].instructionCount].DE.second += 1;
            }

            return;
        }
        else
        {
            advanceInstDE2RN(&DE,&RN);
            DE.flagEmpty = true;
            DE.flagFull = false;
            RN.flagEmpty = false;
            RN.flagFull = true;
        }
    }
    return;
}

void fetch(FILE **fp)
{    
    char buf[60];
    if(DE.flagFull || depletedTrace)
        return;

    for(int i=0;i<scalarWidth;i++)
    {
        unsigned long int pc;
        int op_type,dest,src1,src2;
        if(fscanf(*fp, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
        {
			tableResults.push_back(results(op_type, make_pair(src1,src2),dest,make_pair(cycleCount,1),make_pair(0,0),make_pair(0,0),make_pair(0,0),make_pair(0,0),make_pair(0,0),make_pair(0,0),make_pair(0,0),make_pair(0,0)));
            dynamicInstCount++;
            DE.regDE[i].instructionCount = programCounter++;
            DE.regDE[i].opcode = (unsigned long int)pc;
            DE.regDE[i].optype = op_type;
            DE.regDE[i].rd = dest;
            DE.regDE[i].rs1 = src1;
            DE.regDE[i].rs2 = src2;
            DE.sizeInst+=1;
            DE.flagEmpty = false;
            if(DE.sizeInst == DE.size)
                DE.flagFull = true;
            
            if(op_type == 0)
                DE.regDE[i].cycles = 1;
            else if(op_type == 1)
                DE.regDE[i].cycles = 2;
            else
                DE.regDE[i].cycles = 5;
			
            DE.regDE[i].src1ROB = false;
            DE.regDE[i].src2ROB = false;
            DE.regDE[i].src1Rdy = false;
            DE.regDE[i].src2Rdy = false;
			

            pipeInstructionCount += 1;
        }
        else
        {
            depletedTrace = true;
            break;
        }
    }
}
/*********************************************************************************/
/*********************************************************************************/


int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    int op_type, dest, src1, src2;  // Variables are read from trace file
    unsigned long int pc; // Variable holds the pc read from input file
    
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
 
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    simConfiguration(params.rob_size,params.iq_size,params.width);
    do
    {
        retire();
        writeback();
        execute();
        issue();
        dispatch();
        regRead();
        rename();
        decode();   
        fetch(&FP);   
    } while (Advance_Cycle());

    print_results();


    printf("# === Simulator Command =========\n");
    printf("# %s %lu %lu %lu %s\n", argv[0], params.rob_size, params.iq_size, params.width, trace_file);
    printf("# === Processor Configuration ===\n");
    printf("# ROB_SIZE = %lu \n"
            "# IQ_SIZE  = %lu \n"
            "# WIDTH    = %lu\n", params.rob_size, params.iq_size, params.width);
    printf("# === Simulation Results ========\n");
    printf("# Dynamic Instruction Count    = %d\n",dynamicInstCount);
    printf("# Cycles                       = %d\n",cycleCount);
    printf("# Instructions Per Cycle (IPC) = %.2f\n",((float)(dynamicInstCount)/ (float)(cycleCount)));
    return 0;
}