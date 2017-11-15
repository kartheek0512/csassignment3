// addrspace.h
//	Data structures to keep track of executing user programs
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
//Edited_Start
#include "noff.h"
//Edited_Stop

#define UserStackSize		1024 	// increase this as necessary!

//Edited_Start
class executableEntry {
public:
  executableEntry(OpenFile *executableGiven);
  OpenFile * executable;
  unsigned numAddrSpacesAttached;
};
//Edited_Stop

class ProcessAddressSpace {
  public:
    ProcessAddressSpace(OpenFile *executable);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"

    ProcessAddressSpace (ProcessAddressSpace *parentSpace, unsigned childPID);	// Used by fork

    ~ProcessAddressSpace();			// De-allocate an address space

    void InitUserModeCPURegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveContextOnSwitch();			// Save/restore address space-specific
    void RestoreContextOnSwitch();		// info on a context switch
    //Edited_Start
    void pageFaultHandler(unsigned faultVAddr);
    //Edited_Stop

    unsigned GetNumPages();
    unsigned GetNumVPagesShared();

    TranslationEntry* GetPageTable();

    //Edited_Start
    int ShmAllocate(unsigned reqPages);
    NoffHeader noffH;
    int executableKey;
    bool executableKeyValid;
    char *GetBackUp();
    //Edited_Stop

  private:
    TranslationEntry *KernelPageTable;	// Assume linear page table translation
    //Edited_Start
    char * backUp;
    //Edited_Stop
					// for now!
    unsigned int numVirtualPages;		// Number of pages in the virtual
    unsigned  numVPagesShared;
					// address space
};

#endif // ADDRSPACE_H
