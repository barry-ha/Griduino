//------------------------------------------------------------------------------
//  File name: sm_button.c
//
//  Description: Button processing implementation file.
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
#include <stdio.h>
#include <stdint.h>                                                             //  Standard int types
#include <stdbool.h>                                                            //  Standard bool type

#include "sm.h"
#include "sm_button.h"

//------------------------------------------------------------------------------
// pragmas
//------------------------------------------------------------------------------
#pragma GCC diagnostic ignored "-Wunused-parameter"

//------------------------------------------------------------------------------
// Local enum
//------------------------------------------------------------------------------
enum {ST_IDLE, ST_SCLICK_TST, ST_CLICK_IDLE, ST_DCLICK_TST, ST_CT   };

//------------------------------------------------------------------------------
// Local functions
//------------------------------------------------------------------------------
static SM_STATE_T sm_button_init(STATE_MACHINE_T* sm);

static SM_STATUS_T idle_enter(STATE_MACHINE_T* sm, SM_EVENT_T ev);
static SM_STATUS_T idle_process(STATE_MACHINE_T* sm, SM_EVENT_T ev);
static SM_STATUS_T idle_exit(STATE_MACHINE_T* sm, SM_EVENT_T ev);

static SM_STATUS_T sclick_test_enter(STATE_MACHINE_T* sm, SM_EVENT_T ev);
static SM_STATUS_T sclick_test_process(STATE_MACHINE_T* sm, SM_EVENT_T ev);
static SM_STATUS_T sclick_test_exit(STATE_MACHINE_T* sm, SM_EVENT_T ev);

static SM_STATUS_T click_idle_enter(STATE_MACHINE_T* sm, SM_EVENT_T ev);
static SM_STATUS_T click_idle_process(STATE_MACHINE_T* sm, SM_EVENT_T ev);
static SM_STATUS_T click_idle_exit(STATE_MACHINE_T* sm, SM_EVENT_T ev);

static SM_STATUS_T dclick_test_enter(STATE_MACHINE_T* sm, SM_EVENT_T ev);
static SM_STATUS_T dclick_test_process(STATE_MACHINE_T* sm, SM_EVENT_T ev);
static SM_STATUS_T dclick_test_exit(STATE_MACHINE_T* sm, SM_EVENT_T ev);

//------------------------------------------------------------------------------
// Local data
//------------------------------------------------------------------------------
static const SM_IDX_T states[] =
{
    [ST_IDLE]           = { idle_enter,         idle_process,           idle_exit           },  //  state: idle
    [ST_SCLICK_TST]     = { sclick_test_enter,  sclick_test_process,    sclick_test_exit    },  //  state: single click test
    [ST_CLICK_IDLE]     = { click_idle_enter,   click_idle_process,     click_idle_exit     },  //  state: click-idle
    [ST_DCLICK_TST]     = { dclick_test_enter,  dclick_test_process,    dclick_test_exit    },  //  state: double click test
};
static const uint32_t sm_state_ct = sizeof(states) / sizeof(SM_IDX_T);

static STATE_MACHINE_T sm =
{
    ST_IDLE,                                                  //  Curr state
    ST_IDLE,                                                  //  New state
    ST_IDLE,                                                  //  Prev state
    SM_OK,                                                    //  status
    sm_button_init,                                           //  fn: initialization
    false,                                                    //  Changing states: nope
	4,
	2,
	2
};

static btn_status_t	btn_status;

//------------------------------------------------------------------------------
//  @brief
//      Next test state machine
//      Init the state machine
//
//  @param
//      none
//
//  @return
//      none
//------------------------------------------------------------------------------
void btn_init(void)
{
    SM_Init(&sm, states, sm_state_ct);
    btn_status = btn_idle;
}

//------------------------------------------------------------------------------
//  @brief
//      Next test state machine
//      Run the state machine -- process incoming events
//
//  @param
//      int ev: incoming event
//
//  @return
//      none
//------------------------------------------------------------------------------
void btn_process(BTN_EVENT_T ev)
{
    SM_Process(&sm, states, (SM_EVENT_T)ev);                                    //  Process current state
}

//------------------------------------------------------------------------------
//  @brief
//      Next test state machine
//      Run the state machine -- process incoming events
//
//  @param
//      int ev: incoming event
//
//  @return
//      none
//------------------------------------------------------------------------------
btn_status_t btn_status_get(void)
{
	return btn_status;
}

void btn_status_reset(void)
{
	btn_status = btn_idle;
}

//------------------------------------------------------------------------------
//  @brief
//      State machine state processing
//      Initialize the state machine, return the first state
//
//  @param
//      sm: state machine
//      ev: state machine event
//
//  @return
//      state machine first state
//------------------------------------------------------------------------------
static SM_STATE_T sm_button_init(STATE_MACHINE_T* sm)
{
    return ST_IDLE;
}

//------------------------------------------------------------------------------
//  brief
//      idle state processing:
//      - idle_enter:       enter state
//      - idle_process:     process state
//      - idle_exit:        exit state
//
//  param
//      sm: state machine
//      ev: state machine event
//
//  return
//      state machine status
//------------------------------------------------------------------------------
static SM_STATUS_T idle_enter(STATE_MACHINE_T* sm, SM_EVENT_T ev)
{
    return SM_OK;                                                               //  Status is OK
}
static SM_STATUS_T idle_process(STATE_MACHINE_T* sm, SM_EVENT_T ev)
{
    switch(ev)                                                                  //  process button events
    {
    case EVENT_100MS:                                                           //  100ms timer event
        break;
    case EVENT_BUTTON_DN:                                                       //  button down
        SM_StateChange(sm, ST_SCLICK_TST);                                      //  Change to click test
        break;
    case EVENT_BUTTON_UP:                                                       //  button up
        break;
    }
    return SM_OK;                                                               //  Status is OK
}
static SM_STATUS_T idle_exit(STATE_MACHINE_T* sm, SM_EVENT_T ev)
{
    return SM_OK;                                                               //  Status is OK
}

//------------------------------------------------------------------------------
//  brief
//      single click test state processing:
//      - sclick_test_enter:        enter state
//      - sclick_test_process:      process state
//      - sclick_test_exit:         exit state
//
//  param
//      sm: state machine
//      ev: state machine event
//
//  return
//      state machine status
//------------------------------------------------------------------------------
static uint32_t sclick_tmr_ct = 0;
static SM_STATUS_T sclick_test_enter(STATE_MACHINE_T* sm, SM_EVENT_T ev)
{
    sclick_tmr_ct = 0;                                                          //  reset the timer count
    return SM_OK;                                                               //  Status is OK
}
static SM_STATUS_T sclick_test_process(STATE_MACHINE_T* sm, SM_EVENT_T ev)
{
    switch(ev)                                                                  //  process button events
    {
    case EVENT_100MS:                                                           //  100ms timer event
        sclick_tmr_ct += 1;                                                     //  bump timer
        if(sclick_tmr_ct == 50)                                                 //  go to idle after 5 seconds
        {
            SM_StateChange(sm, ST_IDLE);                                        //  Change to idle state
        }
        break;
    case EVENT_BUTTON_DN:                                                       //  button down
        break;
    case EVENT_BUTTON_UP:                                                       //  button up
        if(sclick_tmr_ct <= 2)                                                  //  200ms
        {
        	btn_status = btn_click;
            SM_StateChange(sm, ST_CLICK_IDLE);                                  //  Change to idle state
        }
        else if(sclick_tmr_ct <= 10)                                            //  1000ms
        {
        	btn_status = btn_press;
            SM_StateChange(sm, ST_IDLE);                                        //  Change to idle state
        }
        else                                                                    //  1000ms
        {
        	btn_status = btn_long_press;
            SM_StateChange(sm, ST_IDLE);                                        //  Change to idle state
        }
        break;
    }
    return SM_OK;                                                               //  Status is OK
}
static SM_STATUS_T sclick_test_exit(STATE_MACHINE_T* sm, SM_EVENT_T ev)
{
    return SM_OK;                                                               //  Status is OK
}

//------------------------------------------------------------------------------
//  brief
//      click idle state processing:
//      - click_idle_enter:         enter state
//      - click_idle_process:       process state
//      - click_idle_exit:          exit state
//
//  param
//      sm: state machine
//      ev: state machine event
//
//  return
//      state machine status
//------------------------------------------------------------------------------
static uint32_t cidle_tmr_ct = 0;
static SM_STATUS_T click_idle_enter(STATE_MACHINE_T* sm, SM_EVENT_T ev)
{
    cidle_tmr_ct = 0;                                                           //  reset the click idle timer
    return SM_OK;                                                               //  Status is OK
}
static SM_STATUS_T click_idle_process(STATE_MACHINE_T* sm, SM_EVENT_T ev)
{
    switch(ev)                                                                  //  process button events
    {
    case EVENT_100MS:                                                           //  100ms timer event
        cidle_tmr_ct += 1;                                                      //  bump click idle timer count
        if(cidle_tmr_ct == 6)                                                   //  go to idle after 5 seconds
        {
            SM_StateChange(sm, ST_IDLE);                                        //  Change to idle state
        }
        break;
    case EVENT_BUTTON_DN:                                                       //  button down
        if(cidle_tmr_ct <= 5)
        {
            SM_StateChange(sm, ST_DCLICK_TST);                                  //  Change to double click test state
        }
        else
        {
            SM_StateChange(sm, ST_IDLE);                                        //  Change to idle state
        }
        break;
    case EVENT_BUTTON_UP:                                                       //  button up
        break;
    }
    return SM_OK;                                                               //  Status is OK
}
static SM_STATUS_T click_idle_exit(STATE_MACHINE_T* sm, SM_EVENT_T ev)
{
    return SM_OK;                                                               //  Status is OK
}

//------------------------------------------------------------------------------
//  brief
//      double click test processing:
//      - dclick_test_enter:        enter state
//      - dclick_test_process:      process state
//      - dclick_test_exit:         exit state
//
//  param
//      sm: state machine
//      ev: state machine event
//
//  return
//      state machine status
//------------------------------------------------------------------------------
static uint32_t dclick_tmr_ct = 0;
static SM_STATUS_T dclick_test_enter(STATE_MACHINE_T* sm, SM_EVENT_T ev)
{
    dclick_tmr_ct = 0;
    return SM_OK;                                                               //  Status is OK
}
static SM_STATUS_T dclick_test_process(STATE_MACHINE_T* sm, SM_EVENT_T ev)
{
    switch(ev)                                                                  //  process button events
    {
    case EVENT_100MS:                                                           //  100ms timer event
        dclick_tmr_ct += 1;                                                     //  bump timer
        if(dclick_tmr_ct == 50)                                                 //  go to idle after 5 seconds
        {
            SM_StateChange(sm, ST_IDLE);                                        //  Change to idle state
        }
        break;
    case EVENT_BUTTON_DN:                                                       //  button down
        break;
    case EVENT_BUTTON_UP:                                                       //  button up
        if(dclick_tmr_ct <= 2)                                                  //  200ms
        {
        	btn_status = btn_double_click;
        }
        else if(dclick_tmr_ct <= 10)                                            //  1000ms
        {
        	btn_status = btn_click_press;
        }
        else                                                                    //  1000ms
        {
        	btn_status = btn_click_long_press;
        }
        SM_StateChange(sm, ST_IDLE);                                            //  Change to idle state
        break;
    }
    return SM_OK;                                                               //  Status is OK
}
static SM_STATUS_T dclick_test_exit(STATE_MACHINE_T* sm, SM_EVENT_T ev)
{
    return SM_OK;                                                               //  Status is OK
}
