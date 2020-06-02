//------------------------------------------------------------------------------
//  File name: sm.h
//
//  Description: State machine header file
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
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//  Typedef variables for state machine
//------------------------------------------------------------------------------
typedef uint32_t    SM_EVENT_T;                                                 //    Events passed to state machine
typedef uint16_t    SM_STATE_T;                                                 //    Current state
typedef enum
{
    SM_OK,                                                                      //    State machine is running well
    SM_ERROR,                                                                   //    State machine error
    SM_SC_ERROR                                                                 //    State machine state count error
} SM_STATUS_T;                                                                  //    Current status of the state machine

//------------------------------------------------------------------------------
//  Define SM control events
//------------------------------------------------------------------------------
enum
{
    SM_INIT,
    SM_ENTER,
    SM_EXIT,
};

//------------------------------------------------------------------------------
//  Forward reference so that STATE_FN can be declared
//------------------------------------------------------------------------------
typedef struct STATE_MACHINEtag STATE_MACHINE_T;                                //  Forward reference

//------------------------------------------------------------------------------
//  Typedef functions for state machine
//------------------------------------------------------------------------------
typedef SM_STATUS_T(*STATE_FN)(STATE_MACHINE_T* sm, SM_EVENT_T ev);             //  State processing function
typedef SM_STATE_T(*STATE_INIT)(STATE_MACHINE_T* sm);                           //  Init function

//------------------------------------------------------------------------------
//  Typedef structures for state machine
//------------------------------------------------------------------------------
typedef struct SM_STAT_FNEtag
{
    STATE_FN    Enter;                                                          //    Enter function
    STATE_FN    Process;                                                        //    Processing function
    STATE_FN    Exit;                                                           //    Exit function
} SM_IDX_T;

struct STATE_MACHINEtag
{
    SM_STATE_T    CurrState;                                                    //    SM state
    SM_STATE_T    NewState;                                                     //    SM next state
    SM_STATE_T    PrevState;                                                    //    SM last state
    SM_STATUS_T   Status;                                                       //    SM status
    STATE_INIT    InitFn;                                                       //    SM initial function
    bool          StateChange;                                                  //    Change state
    uint8_t       MaxState;                                                     //    Max size of state array: so we don't go off the end!
    uint8_t       x, y;                                                         //    Size of the auto array
};

//------------------------------------------------------------------------------
//  Function declaration
//------------------------------------------------------------------------------
SM_STATUS_T   SM_Init(STATE_MACHINE_T* sm, const SM_IDX_T st[], uint32_t ms);
SM_STATUS_T   SM_Process(STATE_MACHINE_T* sm, const SM_IDX_T st[], SM_EVENT_T ev);
SM_STATUS_T   SM_StateChange(STATE_MACHINE_T* sm, SM_STATE_T st);
SM_STATUS_T   SM_StateNext(STATE_MACHINE_T* sm, SM_STATE_T const sa[]);
SM_STATUS_T   SM_StatePrev(STATE_MACHINE_T* sm);

SM_STATE_T    SM_GetState(STATE_MACHINE_T* sm);
SM_STATUS_T   SM_GetStatus(STATE_MACHINE_T* sm);

#ifdef __cplusplus
}
#endif
