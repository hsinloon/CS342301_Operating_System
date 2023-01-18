// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------
int compare_L1(Thread* , Thread* );
int compare_L2(Thread* , Thread* );
Scheduler::Scheduler()
{ 
    L1_ready_list = new SortedList<Thread *>(compare_L1); 
    L2_ready_list = new SortedList<Thread *>(compare_L2); 
    L3_ready_list = new List<Thread *>; 
    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete L1_ready_list;
    delete L2_ready_list;
    delete L3_ready_list;
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

//333333333333333333333333333333333333333333333333333333333333333

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);

    if(thread->get_priority() >= 100 && thread->get_priority() <= 149) {
        L1_ready_list->Insert(thread);

        DEBUG(dbgExpr, "[A] Tick [" << kernel->stats->totalTicks 
                << "]: Thread [" << thread->getID() 
                << "] is inserted into queue L[1]");

    }
    else if((thread->get_priority() >= 50 && thread->get_priority() <= 99)){
        L2_ready_list->Insert(thread);

        DEBUG(dbgExpr, "[A] Tick [" << kernel->stats->totalTicks 
                << "]: Thread [" << thread->getID() 
                << "] is inserted into queue L[2]");

    }
    else if((thread->get_priority() >= 0 && thread->get_priority() <= 49)){
        L3_ready_list->Append(thread);

        DEBUG(dbgExpr, "[A] Tick [" << kernel->stats->totalTicks 
                << "]: Thread [" << thread->getID() 
                << "] is inserted into queue L[3]");

    }
}

//333333333333333333333333333333333333333333333333333333333333333

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

//333333333333333333333333333333333333333333333333333333333333333

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if(!L1_ready_list->IsEmpty()){
    DEBUG(dbgExpr, "[B] Tick ["<< kernel->stats->totalTicks 
            << "]: Thread [" << L1_ready_list->Front()->getID() 
            << "] is removed from queue L[1]");
        return L1_ready_list->RemoveFront();
    }
    else if(!L2_ready_list->IsEmpty()){
    DEBUG(dbgExpr, "[B] Tick ["<< kernel->stats->totalTicks 
            << "]: Thread [" << L2_ready_list->Front()->getID() 
            << "] is removed from queue L[2]");
        return L2_ready_list->RemoveFront();
    }
    else if(!L3_ready_list->IsEmpty()){
    DEBUG(dbgExpr, "[B] Tick ["<< kernel->stats->totalTicks 
            << "]: Thread [" << L3_ready_list->Front()->getID() 
            << "] is removed from queue L[2]");
        return L3_ready_list->RemoveFront();
    }
    else return NULL;
}

//333333333333333333333333333333333333333333333333333333333333333

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    //333333333333333333333333333333333333333333333333333333
    nextThread->set_start_time(kernel->stats->totalTicks);

    DEBUG(dbgExpr, "[E] Tick ["<< kernel->stats->totalTicks 
            << "]: Thread ["<< nextThread->getID() 
            << "] is now selected for execution, thread ["
            << kernel->currentThread->getID() <<"] is replaced, and it has executed ["
            << kernel->stats->totalTicks - kernel->currentThread->get_start_time() << "] ticks");
        //kernel->currentThread->set_start_time(0);
    //333333333333333333333333333333333333333333333333333333

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running

    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyList->Apply(ThreadPrint);
}

//33333333333333333333333333333333333333333333333333333333
int compare_L1(Thread* t1, Thread* t2)
{
    if(t1->get_burst_time() != t2->get_burst_time())
        return (t1->get_burst_time() > t2->get_burst_time()) ? 1 : -1;
    else
        return (t1->getID() < t2->getID()) ? 1 : -1;
}

int compare_L2(Thread* t1, Thread* t2)
{
    if(t1->get_priority() != t2->get_priority())
        return (t1->get_priority() > t2->get_priority()) ? -1 : 1;
    else
        return (t1->getID() < t2->getID()) ? 1 : -1;
}




void Scheduler::update_priority()
{
    ListIterator<Thread* > *iter1 = new ListIterator<Thread *>(L1_ready_list);
    ListIterator<Thread* > *iter2 = new ListIterator<Thread *>(L2_ready_list);
    ListIterator<Thread* > *iter3 = new ListIterator<Thread *>(L3_ready_list);

    int old_priority;
    int new_priority;
    //L1
    for( ; !iter1->IsDone(); iter1->Next() ){
        ASSERT( iter1->Item()->getStatus() == READY);

        iter1->Item()->set_waiting_time(iter1->Item()->get_waiting_time()+TimerTicks);
        if(iter1->Item()->get_waiting_time() >= 1500 && iter1->Item()->getID() > 0 ){
            old_priority = iter1->Item()->get_priority();
            new_priority = old_priority + 10;
            if (new_priority > 149){
                new_priority = 149;
            }
            iter1->Item()->set_priority(new_priority);
            iter1->Item()->set_waiting_time(iter1->Item()->get_waiting_time() - 1500);
        }
    }
    //L2
    for( ; !iter2->IsDone(); iter2->Next() ){
        ASSERT( iter2->Item()->getStatus() == READY);

        iter2->Item()->set_waiting_time(iter2->Item()->get_waiting_time()+TimerTicks);
        if(iter2->Item()->get_waiting_time() >= 1500 && iter2->Item()->getID() > 0 ){
            old_priority = iter2->Item()->get_priority();
            new_priority = old_priority + 10;
            DEBUG(dbgExpr, "[C] Tick ["<< kernel->stats->totalTicks 
                    << "]: Thread [" << iter2->Item()->getID() 
                    << "] changes its priority from ["<< old_priority 
                    << "] to ["<< new_priority <<"]");

            if (new_priority > 99){
                L2_ready_list->Remove(iter2->Item());
                DEBUG(dbgExpr, "[B] Tick ["<< kernel->stats->totalTicks 
                        << "]: Thread [" << iter2->Item()->getID() 
                        << "] is removed from queue L[2]");

                L1_ready_list->Insert(iter2->Item());
                DEBUG(dbgExpr, "[A] Tick [" << kernel->stats->totalTicks 
                        << "]: Thread [" << iter2->Item()->getID() 
                        << "] is inserted into queue L[1]");

            }
            iter2->Item()->set_priority(new_priority);
            iter2->Item()->set_waiting_time(iter2->Item()->get_waiting_time() - 1500);
        }
    }
    //L3
    for( ; !iter3->IsDone(); iter3->Next() ){
        ASSERT( iter3->Item()->getStatus() == READY);

        iter3->Item()->set_waiting_time(iter3->Item()->get_waiting_time()+TimerTicks);
        if(iter3->Item()->get_waiting_time() >= 1500 && iter3->Item()->getID() > 0 ){
            old_priority = iter3->Item()->get_priority();
            new_priority = old_priority + 10;
            DEBUG(dbgExpr, "[C] Tick ["<< kernel->stats->totalTicks 
                    << "]: Thread [" << iter3->Item()->getID() 
                    << "] changes its priority from ["<< old_priority 
                    << "] to ["<< new_priority <<"]");

            if (new_priority > 49){
                L3_ready_list->Remove(iter3->Item());
                DEBUG(dbgExpr, "[B] Tick ["<< kernel->stats->totalTicks 
                        << "]: Thread [" << iter3->Item()->getID() 
                        << "] is removed from queue L[3]");

                L2_ready_list->Insert(iter3->Item());
                DEBUG(dbgExpr, "[A] Tick [" << kernel->stats->totalTicks 
                        << "]: Thread [" << iter3->Item()->getID() 
                        << "] is inserted into queue L[2]");
            }
            iter3->Item()->set_priority(new_priority);
            iter3->Item()->set_waiting_time(iter3->Item()->get_waiting_time() - 1500);
        }
    }
    
}

//33333333333333333333333333333333333333333333333333333333