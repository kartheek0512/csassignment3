// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"

//Edited_Start

void doBackUp(unsigned pageToBeBacked){
	TranslationEntry * tempPageTableEntry = machine->physPageWhereAbouts[pageToBeBacked].pageTableEntry;
	tempPageTableEntry->loadFromBackUp = TRUE;
	tempPageTableEntry->dirty = FALSE;
	tempPageTableEntry->shared = FALSE;
	NachOSThread * tempThread = threadArray[machine->physPageWhereAbouts[pageToBeBacked].threadId];
	char * tempBackUp = tempThread->space->GetBackUp();
	int i;
	for (i = 0; i < PageSize; i++) tempBackUp[((tempPageTableEntry->virtualPage)*PageSize) + i] = machine->mainMemory[(pageToBeBacked*PageSize) + i];
}

unsigned getPageToBeReplaced(int exceptThisPage){
	int pageToBeReplaced;
	switch(pageReplacementAlgo){
		case RANDOM:
			pageToBeReplaced = Random()%NumPhysPages;
			while((pageToBeReplaced<0) || (pageToBeReplaced == exceptThisPage) || (((machine->physPageWhereAbouts[pageToBeReplaced]).pageTableEntry)->shared == TRUE))
				pageToBeReplaced = Random()%NumPhysPages;
			(machine->physPageWhereAbouts[pageToBeReplaced].pageTableEntry)->valid = FALSE;
			if(((machine->physPageWhereAbouts[pageToBeReplaced]).pageTableEntry)->dirty) doBackUp(pageToBeReplaced);
				machine->physPageWhereAbouts[pageToBeReplaced].numAddrSpacesAttached--;
				numPagesAllocated--;
			return pageToBeReplaced;
		case FIFO:

			break;
		case LRU:
			break;
		case LRU_CLOCK:
			break;
		default:
			ASSERT(FALSE);
	}
}

List * ListOfExecutables = new List();
executableEntry::executableEntry(OpenFile *executableGiven)
{
	this->executable = executableGiven;
	this->numAddrSpacesAttached = 1;
}
unsigned executableCount=0;
//Edited_Stop

char *
ProcessAddressSpace::GetBackUp()
{
	return this->backUp;
}

unsigned
ProcessAddressSpace::GetNumVPagesShared()
{
	return this->numVPagesShared;
}

int
ProcessAddressSpace::ShmAllocate(unsigned reqPages)
{

	numVPagesShared += reqPages;
	numPagesShared += reqPages;
	TranslationEntry * oldPageTable = KernelPageTable;
	unsigned oldNumVirtualPages = numVirtualPages;
	numVirtualPages = oldNumVirtualPages + reqPages;
	KernelPageTable = new TranslationEntry[numVirtualPages];
	int i;
	for(i=0; i<oldNumVirtualPages; i++)
	{
		KernelPageTable[i].virtualPage = oldPageTable[i].virtualPage;
		KernelPageTable[i].physicalPage = oldPageTable[i].physicalPage;
		KernelPageTable[i].valid = oldPageTable[i].valid;
		KernelPageTable[i].use = oldPageTable[i].use;
		KernelPageTable[i].dirty = oldPageTable[i].dirty;
		KernelPageTable[i].readOnly = oldPageTable[i].readOnly;
		KernelPageTable[i].shared = oldPageTable[i].shared;
		KernelPageTable[i].loadFromBackUp = oldPageTable[i].loadFromBackUp;
		machine->physPageWhereAbouts[oldPageTable[i].physicalPage].threadId = currentThread->GetPID();
		machine->physPageWhereAbouts[oldPageTable[i].physicalPage].numAddrSpacesAttached = 1;
		machine->physPageWhereAbouts[oldPageTable[i].physicalPage].pageTableEntry = KernelPageTable+i;
	}
	void * temp;
	int pageTemp;
	for(i=oldNumVirtualPages; i<numVirtualPages; i++)
	{
		if(!machine->ListOfPagesAvailable->IsEmpty()) temp = (void *)machine->ListOfPagesAvailable->SortedRemove(&pageTemp);
		else {
			pageTemp = getPageToBeReplaced(-1);
		}
		stats->numPageFaults++;
		KernelPageTable[i].virtualPage = i;
		KernelPageTable[i].physicalPage = pageTemp;
		bzero((((char *)machine->mainMemory)+(pageTemp*PageSize)),PageSize);
		// zero out the entire address space, to zero the unitialized data segment
		// and the stack segment
		KernelPageTable[i].valid = TRUE;
		KernelPageTable[i].use = FALSE;
		KernelPageTable[i].dirty = FALSE;
		KernelPageTable[i].readOnly = FALSE;  // if the code segment was entirely on
						// a separate page, we could set its
						// pages to be read-only
		KernelPageTable[i].shared = TRUE;
		KernelPageTable[i].loadFromBackUp = FALSE;
		machine->physPageWhereAbouts[pageTemp].threadId = currentThread->GetPID();
		machine->physPageWhereAbouts[pageTemp].numAddrSpacesAttached = 1;
		machine->physPageWhereAbouts[pageTemp].pageTableEntry = KernelPageTable+i;
	}
	numPagesAllocated += reqPages;
	delete oldPageTable;
	machine->KernelPageTable = KernelPageTable;
	machine->KernelPageTableSize = numVirtualPages;
	return oldNumVirtualPages*PageSize;
}

void
ProcessAddressSpace::pageFaultHandler(unsigned faultVAddr)
{
	ASSERT((NumPhysPages - numPagesShared) > 0)
	stats->numPageFaults++;
	void *temp;
	int pageTemp;
	unsigned faultVPage = faultVAddr/PageSize;
	if(!machine->ListOfPagesAvailable->IsEmpty())temp = (void *)machine->ListOfPagesAvailable->SortedRemove(&pageTemp);
	else {
		pageTemp = getPageToBeReplaced(-1);
	}
	numPagesAllocated++;
	machine->physPageWhereAbouts[pageTemp].threadId = currentThread->GetPID();
	machine->physPageWhereAbouts[pageTemp].numAddrSpacesAttached = 1;
	machine->physPageWhereAbouts[pageTemp].pageTableEntry = machine->KernelPageTable + faultVPage;
	machine->KernelPageTable[faultVPage].physicalPage = pageTemp;
	machine->KernelPageTable[faultVPage].valid = TRUE;
	machine->KernelPageTable[faultVPage].use = FALSE;
	machine->KernelPageTable[faultVPage].dirty = FALSE;
	machine->KernelPageTable[faultVPage].readOnly = FALSE;  // if the code segment was entirely on
					// a separate page, we could set its
					// pages to be read-only
	machine->KernelPageTable[faultVPage].shared = FALSE;
	if(machine->KernelPageTable[faultVPage].loadFromBackUp)
	{
		int i;
		for (i = 0; i < PageSize; i++) machine->mainMemory[(pageTemp*PageSize) + i] = *(currentThread->space->GetBackUp() + (faultVPage*PageSize) + i);
	}
	else
	{
		ASSERT(currentThread->space->executableKeyValid);
		executableEntry * tempExecutableEntry = (executableEntry *)ListOfExecutables->Search(currentThread->space->executableKey);
		NoffHeader noffH = currentThread->space->noffH;
		tempExecutableEntry->executable->ReadAt(&(machine->mainMemory[pageTemp * PageSize]),
	                        PageSize, noffH.code.inFileAddr + faultVPage * PageSize);
	}
}

//Edited_Stop

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// ProcessAddressSpace::ProcessAddressSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

ProcessAddressSpace::ProcessAddressSpace(OpenFile *executable)
{
		//Edited_Start
		numVPagesShared = 0;
		//Edited_Stop
    NoffHeader noffH;
    unsigned int i, size;
    unsigned vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);
		executableEntry *tempExecutableEntry = new executableEntry(executable);
		ListOfExecutables->SortedInsert((void *)tempExecutableEntry, executableCount);
		this->executableKey = executableCount;
		this->executableKeyValid = TRUE;
		this->noffH = noffH;
		executableCount++;

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numVirtualPages = divRoundUp(size, PageSize);
    size = numVirtualPages * PageSize;

		//Edited_Start
		backUp = new char[size];
		//Edited_Stop

		if(pageReplacementAlgo == None){
			ASSERT(numVirtualPages+numPagesAllocated <= NumPhysPages);
		}                // check we're not trying
    else{
			ASSERT((NumPhysPages - numPagesShared) > 0);
		}                                                                             // to run anything too big --

                                                                                // at least until we have


    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
					numVirtualPages, size);
// first, set up the translation
    KernelPageTable = new TranslationEntry[numVirtualPages];
		//Edited_Start
		for (i = 0; i < numVirtualPages; i++) {
			KernelPageTable[i].virtualPage = i;
			KernelPageTable[i].valid = FALSE;
			KernelPageTable[i].use = FALSE;
			KernelPageTable[i].dirty = FALSE;
			KernelPageTable[i].readOnly = FALSE;  // if the code segment was entirely on
							// a separate page, we could set its
							// pages to be read-only
			KernelPageTable[i].shared = FALSE;
			KernelPageTable[i].loadFromBackUp = FALSE;
	    }
/*		void * temp;
    int pageTemp;
    for (i = 0; i < numVirtualPages; i++) {
			temp = (void *)machine->ListOfPagesAvailable->SortedRemove(&pageTemp);
	KernelPageTable[i].virtualPage = i;
	KernelPageTable[i].physicalPage = pageTemp;
	bzero((((char *)machine->mainMemory)+(pageTemp*PageSize)),PageSize);
	// zero out the entire address space, to zero the unitialized data segment
	// and the stack segment
	KernelPageTable[i].valid = TRUE;
	KernelPageTable[i].use = FALSE;
	KernelPageTable[i].dirty = FALSE;
	KernelPageTable[i].readOnly = FALSE;  // if the code segment was entirely on
					// a separate page, we could set its
					// pages to be read-only
	KernelPageTable[i].shared = FALSE;
    }
		//Edited_Stop
    numPagesAllocated += numVirtualPages;

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
			noffH.code.virtualAddr, noffH.code.size);
        vpn = noffH.code.virtualAddr/PageSize;
        offset = noffH.code.virtualAddr%PageSize;
        entry = &KernelPageTable[vpn];
        pageFrame = entry->physicalPage;
        executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
			noffH.initData.virtualAddr, noffH.initData.size);
        vpn = noffH.initData.virtualAddr/PageSize;
        offset = noffH.initData.virtualAddr%PageSize;
        entry = &KernelPageTable[vpn];
        pageFrame = entry->physicalPage;
        executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }*/

}

//----------------------------------------------------------------------
// ProcessAddressSpace::ProcessAddressSpace (ProcessAddressSpace*) is called by a forked thread.
//      We need to duplicate the address space of the parent.
//----------------------------------------------------------------------

ProcessAddressSpace::ProcessAddressSpace(ProcessAddressSpace *parentSpace, unsigned childPID)
{
		numVirtualPages = parentSpace->GetNumPages();
    unsigned i, size = numVirtualPages * PageSize, j;
		//Edited_Start
		numVPagesShared = parentSpace->GetNumVPagesShared();
		//backUp = new char[(numVirtualPages - numVPagesShared)* PageSize];
		backUp = new char[numVirtualPages*PageSize];
		//Edited_Stop

    if(pageReplacementAlgo == None){
			ASSERT(numVirtualPages+numPagesAllocated <= NumPhysPages);
		}                 // check we're not trying
    else {
			ASSERT((NumPhysPages - numPagesShared) > 1);                                                                            // to run anything too big --
		}                                                                          // at least until we have
                                                                               // virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
                                        numVirtualPages, size);
    // first, set up the translation
    TranslationEntry* parentPageTable = parentSpace->GetPageTable();
    KernelPageTable = new TranslationEntry[numVirtualPages];
		//Edited_Start
		void * temp;
    int pageTemp;
    for (i = 0; i < numVirtualPages; i++) {
			KernelPageTable[i].loadFromBackUp = FALSE;
			if(parentPageTable[i].valid && parentPageTable[i].shared)
			{
				KernelPageTable[i].physicalPage = parentPageTable[i].physicalPage;
				machine->physPageWhereAbouts[parentPageTable[i].physicalPage].numAddrSpacesAttached++;
			}
			else if(parentPageTable[i].valid && !(parentPageTable[i].shared))
			{
				if(!machine->ListOfPagesAvailable->IsEmpty())temp = (void *)machine->ListOfPagesAvailable->SortedRemove(&pageTemp);
				else {
					pageTemp = getPageToBeReplaced(parentPageTable[i].physicalPage);
					printf("Fork\n" );
				}
				stats->numPageFaults++;
				machine->physPageWhereAbouts[pageTemp].threadId = childPID;
				machine->physPageWhereAbouts[pageTemp].numAddrSpacesAttached = 1;
				machine->physPageWhereAbouts[pageTemp].pageTableEntry = &KernelPageTable[i];
				KernelPageTable[i].physicalPage = pageTemp;
				bzero((((char *)machine->mainMemory)+(pageTemp*PageSize)),PageSize);
				// zero out the entire address space, to zero the unitialized data segment
				// and the stack segmentKernelPageTable[i].physicalPage = pageTemp;
				for(j=0;j<PageSize;j++){
					machine->mainMemory[((KernelPageTable[i].physicalPage)*PageSize)+j] = machine->mainMemory[((parentPageTable[i].physicalPage)*PageSize)+j];
					if(parentPageTable[i].loadFromBackUp || parentPageTable[i].dirty){
							KernelPageTable[i].loadFromBackUp = TRUE;
							backUp[(i*PageSize) + j] = machine->mainMemory[(((*(parentPageTable+i)).physicalPage)*PageSize)+j];
						}
					}
				numPagesAllocated++;
			}
			else {
				if(parentPageTable[i].loadFromBackUp){
					KernelPageTable[i].loadFromBackUp = TRUE;
					for(j=0;j<PageSize;j++) backUp[(i*PageSize) + j] = *(parentSpace->GetBackUp() + (i*PageSize) + j);
				}
			}
			  KernelPageTable[i].virtualPage = i;
				KernelPageTable[i].valid = parentPageTable[i].valid;
        KernelPageTable[i].use = parentPageTable[i].use;
        KernelPageTable[i].dirty = FALSE;
        KernelPageTable[i].readOnly = parentPageTable[i].readOnly;  	// if the code segment was entirely on
                                        			// a separate page, we could set its
                                        			// pages to be read-only
				KernelPageTable[i].shared = parentPageTable[i].shared;
    }

		//Edited_Start
		this->executableKey = parentSpace->executableKey;
		this->executableKeyValid = TRUE;
		this->noffH = parentSpace->noffH;
		executableEntry* tempExecutableEntry = (executableEntry *)ListOfExecutables->Search(this->executableKey);
		tempExecutableEntry->numAddrSpacesAttached++;
		//Edited_Stop

		// Copy the contents
    /*unsigned startAddrParent = parentPageTable[0].physicalPage*PageSize;
    unsigned startAddrChild = numPagesAllocated*PageSize;
    for (i=0; i<size; i++) {
       machine->mainMemory[startAddrChild+i] = machine->mainMemory[startAddrParent+i];
    }*/

}

//----------------------------------------------------------------------
// ProcessAddressSpace::~ProcessAddressSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

ProcessAddressSpace::~ProcessAddressSpace()
{
	//Edited_Start
	int pageTemp,i;
	executableEntry * tempExecutableEntry = (executableEntry *)ListOfExecutables->Search(this->executableKey);
	tempExecutableEntry->numAddrSpacesAttached--;
	if(!(tempExecutableEntry->numAddrSpacesAttached>0))ListOfExecutables->SearchAndRemove(this->executableKey);
    for (i = 0; i < numVirtualPages; i++) {
			if(KernelPageTable[i].valid)
			{
				pageTemp = KernelPageTable[i].physicalPage;
				machine->physPageWhereAbouts[pageTemp].numAddrSpacesAttached--;
				if(machine->physPageWhereAbouts[pageTemp].numAddrSpacesAttached==0)
					{
						machine->ListOfPagesAvailable->SortedInsert(NULL,pageTemp);
						numPagesAllocated--;
						if(KernelPageTable[i].shared)numPagesShared--;
					}
			}
		}
	//Edited_Stop
	 delete KernelPageTable;
}

//----------------------------------------------------------------------
// ProcessAddressSpace::InitUserModeCPURegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
ProcessAddressSpace::InitUserModeCPURegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numVirtualPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numVirtualPages * PageSize - 16);
}

//----------------------------------------------------------------------
// ProcessAddressSpace::SaveContextOnSwitch
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void ProcessAddressSpace::SaveContextOnSwitch()
{}

//----------------------------------------------------------------------
// ProcessAddressSpace::RestoreContextOnSwitch
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void ProcessAddressSpace::RestoreContextOnSwitch()
{
    machine->KernelPageTable = KernelPageTable;
    machine->KernelPageTableSize = numVirtualPages;
}

unsigned
ProcessAddressSpace::GetNumPages()
{
   return numVirtualPages;
}

TranslationEntry*
ProcessAddressSpace::GetPageTable()
{
   return KernelPageTable;
}
