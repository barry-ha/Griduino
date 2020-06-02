//------------------------------------------------------------------------------
//  File name: sm.c
//
//  Description: State machine implementation file.
//
//------------------------------------------------------------------------------
//  The MIT License (MIT)
//
//  Copyright (c) 2020 A.C. Verbeck
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

#include "sm.h"

//------------------------------------------------------------------------------
//
// brief
//     Initialize state machine
//
// param
//     STATE_MACHINE_T*  sm:  Pointer to state machine structure
//     SM_IDX_T          st:  State table
//     uint32_t          ms:  Max state
//
// return
//     State machine status
//
//------------------------------------------------------------------------------
SM_STATUS_T SM_Init(STATE_MACHINE_T* sm, const SM_IDX_T st[], uint32_t ms)
{
    sm->MaxState    = ms;                                                       //  Set state machine max state
    sm->CurrState   = (*sm->InitFn)(sm);                                        //  Run the init function
    sm->NewState    = sm->CurrState;                                            //  Init new state to current state
    sm->PrevState   = sm->CurrState;                                            //  Init previous state to current state

    if(sm->CurrState < ms)                                                      //  If less than max state...
    {
        sm->Status = (*st[sm->CurrState].Enter)(sm, SM_INIT);                   //  ...enter state
    }
    else                                                                        //  If not...
    {
        sm->Status = SM_SC_ERROR;                                               //  ...set state count error
    }

    sm->StateChange = false;                                                    //  No state change on init
    return sm->Status;                                                          //  Return status
}

//------------------------------------------------------------------------------
//
// brief
//     Execute current state
//
// param
//     STATE_MACHINE_T*  sm: Pointer to current state machine
//     SM_IDX_T          st:  State table
//     SM_EVENT_T        ev: Event to be processed
//
// return
//     State machine status
//
//------------------------------------------------------------------------------
SM_STATUS_T SM_Process(STATE_MACHINE_T* sm, const SM_IDX_T st[], SM_EVENT_T ev)
{
    sm->Status = (*st[sm->CurrState].Process)(sm, ev);                          //  Run current state processing

    if(sm->StateChange == true)                                                 //  If SM to change states...
    {
        (*st[sm->CurrState].Exit)(sm, SM_EXIT);                                 //  ...exit old state...
        sm->PrevState = sm->CurrState;                                          //  ...set prev state...
        sm->CurrState = sm->NewState;                                           //  ...set new state...
        (*st[sm->CurrState].Enter)(sm, SM_ENTER);                               //  ...enter new state...
        sm->StateChange = false;                                                //  ...reset state change
    }

    return sm->Status;                                                          //  Return process status
}

//------------------------------------------------------------------------------
//
// brief
//     Change state machine state
//     These functions set a new state into the state machine
//     - SM_StateChange: Change to a new state
//     - SM_StateAuto:   Change states based on a table of events x states
//     - SM_StateNext:   Change states based on a state change table
//     - SM_StatePrev:   Change to the previous state
//
// param
//     STATE_MACHINE_T*  sm: Pointer to current state machine
//     SM_STATE_T        st: Next state
//
// return
//     SM_OK -- always returns this.
//
//------------------------------------------------------------------------------
SM_STATUS_T SM_StateChange(STATE_MACHINE_T* sm, SM_STATE_T st)
{
    if(st < sm->MaxState)                                                       //  If less than max state...
    {
        sm->NewState = st;                                                      //  ...set new state...
        sm->StateChange = true;                                                 //  ...set state change to true...
    }
    else                                                                        //  If new state is not in range...
    {
        sm->Status = SM_SC_ERROR;                                               //  ...set state count error
    }
    return sm->Status;                                                          //  Return state change status
}
SM_STATUS_T SM_StateNext(STATE_MACHINE_T* sm, SM_STATE_T const sa[])
{
    sm->NewState = sa[sm->CurrState];                                           //  Get next state from event table
    sm->StateChange = true;                                                     //  Set state change to true...
    return sm->Status;                                                          //  Return state change status
}
SM_STATUS_T SM_StatePrev(STATE_MACHINE_T* sm)
{
    sm->NewState = sm->PrevState;                                               //  Set state to previous state
    sm->StateChange = true;                                                     //  Set state change to true
    sm->Status = SM_OK;                                                         //  Set state machine status to OK
    return sm->Status;                                                          //  Return state change status
}

//------------------------------------------------------------------------------
//
// brief
//     Access function to retrieve state machine current state.
//
// param
//     STATE_MACHINE_T*  sm: Pointer to current state machine
//
// return
//     Current state
//
//------------------------------------------------------------------------------
SM_STATE_T SM_GetState(STATE_MACHINE_T* sm)
{
    return sm->CurrState;
}

//------------------------------------------------------------------------------
//
// brief
//     Access function to retrieve state machine current status.
//
// param
//     STATE_MACHINE_T*  sm: Pointer to current state machine
//
// return
//     State machine status
//
//------------------------------------------------------------------------------
SM_STATUS_T SM_GetStatus(STATE_MACHINE_T* sm)
{
    return sm->Status;
}

//
//  End: sm.c
//
