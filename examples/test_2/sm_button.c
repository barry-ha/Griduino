//------------------------------------------------------------------------------
//  File name: sm_button.c
//
//  Description: Button processing on resistive touch screen using a state machine
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
#include <stdint.h>                   // Standard int types
#include <stdbool.h>                  // Standard bool type
#include <string.h>                   // memcpy

#include "sm.h"
#include "sm_button.h"

//------------------------------------------------------------------------------
// pragmas
//------------------------------------------------------------------------------
#pragma GCC diagnostic ignored "-Wunused-parameter"

//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------
//efine	SAMPLE_TIME_MS   50                       // 50ms (20Hz)
#define	SAMPLE_TIME_MS   25                       // 25ms (40Hz)

#define	IDLE_TIMEOUT     (1000 / SAMPLE_TIME_MS)  // 1 second
//efine	CLICK_TIME       ( 250 / SAMPLE_TIME_MS)  // 250ms
#define	CLICK_TIME       ( 100 / SAMPLE_TIME_MS)  // 100ms
#define	LONG_PRESS_TIME  (1200 / SAMPLE_TIME_MS)  // 1.2s

#define	DCLICK_TIME      ( 400 / SAMPLE_TIME_MS)  // 400ms

//------------------------------------------------------------------------------
// Local enum
//------------------------------------------------------------------------------
enum {ST_IDLE, ST_SCLICK_TST, ST_CLICK_IDLE, ST_DCLICK_TST, ST_CT   };

//------------------------------------------------------------------------------
// Local functions
//------------------------------------------------------------------------------
static sm_state_t sm_button_init(sm_t* sm);

static sm_status_t idle_enter(sm_t* sm, sm_event_t ev);
static sm_status_t idle_process(sm_t* sm, sm_event_t ev);
static sm_status_t idle_exit(sm_t* sm, sm_event_t ev);

static sm_status_t sclick_test_enter(sm_t* sm, sm_event_t ev);
static sm_status_t sclick_test_process(sm_t* sm, sm_event_t ev);
static sm_status_t sclick_test_exit(sm_t* sm, sm_event_t ev);

static sm_status_t click_idle_enter(sm_t* sm, sm_event_t ev);
static sm_status_t click_idle_process(sm_t* sm, sm_event_t ev);
static sm_status_t click_idle_exit(sm_t* sm, sm_event_t ev);

static sm_status_t dclick_test_enter(sm_t* sm, sm_event_t ev);
static sm_status_t dclick_test_process(sm_t* sm, sm_event_t ev);
static sm_status_t dclick_test_exit(sm_t* sm, sm_event_t ev);

//------------------------------------------------------------------------------
// Local data
//------------------------------------------------------------------------------
static const sm_state_fn_t states[] =
{
    [ST_IDLE]           = { idle_enter,         idle_process,           idle_exit,			"idle"					},  //  state: idle
    [ST_SCLICK_TST]     = { sclick_test_enter,  sclick_test_process,    sclick_test_exit,   "single click test"		},  //  state: single click test
    [ST_CLICK_IDLE]     = { click_idle_enter,   click_idle_process,     click_idle_exit,    "click idle"			},  //  state: click-idle
    [ST_DCLICK_TST]     = { dclick_test_enter,  dclick_test_process,    dclick_test_exit,   "double click test"		},  //  state: double click test
};
static const uint32_t sm_state_ct = sizeof(states) / sizeof(sm_state_fn_t);

static sm_t			sm;
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
  memset(&sm, 0, sizeof(sm));         // clear state machine
  sm.init_fn = sm_button_init;        // set init function
  sm_init(&sm, states, sm_state_ct);  // init
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
void btn_process(btn_event_t ev)
{
  sm_process(&sm, states, (sm_event_t)ev);  //  Process current state
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
static sm_state_t sm_button_init(sm_t* sm)
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
static sm_status_t idle_enter(sm_t* sm, sm_event_t ev)
{
    return SM_OK;                       //  Status is OK
}
static sm_status_t idle_process(sm_t* sm, sm_event_t ev)
{
  switch(ev)                            //  process button events
  {
  case EVENT_TIMER:                     //  100ms timer event
    break;
  case EVENT_BUTTON_DN:                 //  button down
    sm_state_change(sm, ST_SCLICK_TST); //  Change to click test
    break;
  case EVENT_BUTTON_UP:                 //  button up
    break;
  }
  return SM_OK;                         //  Status is OK
}
static sm_status_t idle_exit(sm_t* sm, sm_event_t ev)
{
  return SM_OK;                         //  Status is OK
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
static sm_status_t sclick_test_enter(sm_t* sm, sm_event_t ev)
{
    sclick_tmr_ct = 0;                                                          //  reset the timer count
    return SM_OK;                                                               //  Status is OK
}
static sm_status_t sclick_test_process(sm_t* sm, sm_event_t ev)
{
    switch(ev)                                                                  //  process button events
    {
    case EVENT_TIMER:                                                           //  timer event
        sclick_tmr_ct += 1;                                                     //  bump timer
        if (sclick_tmr_ct == LONG_PRESS_TIME)                              		//  go to idle after timeout
        {
        	btn_status = btn_long_press;
        	sm_state_change(sm, ST_IDLE);                                       //  Change to idle state
        }
        break;
    case EVENT_BUTTON_DN:                                                       //  button down
        break;
    case EVENT_BUTTON_UP:                                                       //  button up
        if (sclick_tmr_ct < CLICK_TIME)
        {
        	sm_state_change(sm, ST_CLICK_IDLE);                                 //  Look for double click
        }
        else
        {
        	btn_status = btn_press;
        	sm_state_change(sm, ST_IDLE);                                       //  Change to idle state
        }
        break;
    }
    return SM_OK;                                                               //  Status is OK
}
static sm_status_t sclick_test_exit(sm_t* sm, sm_event_t ev)
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
static sm_status_t click_idle_enter(sm_t* sm, sm_event_t ev)
{
    cidle_tmr_ct = 0;                                                           //  reset the click idle timer
    return SM_OK;                                                               //  Status is OK
}
static sm_status_t click_idle_process(sm_t* sm, sm_event_t ev)
{
    switch(ev)                                                                  //  process button events
    {
    case EVENT_TIMER:                                                           //  timer event
        cidle_tmr_ct += 1;                                                      //  bump click idle timer count
        if (cidle_tmr_ct == DCLICK_TIME)                                  		//  go to idle after timeout
        {
        	btn_status = btn_click;
        	sm_state_change(sm, ST_IDLE);                                       //  Change to idle state
        }
        break;
    case EVENT_BUTTON_DN:                                                       //  button down
        if (cidle_tmr_ct < DCLICK_TIME)
        {
        	sm_state_change(sm, ST_DCLICK_TST);                                 //  Change to double click test state
        }
        else
        {
        	sm_state_change(sm, ST_IDLE);                                       //  Change to idle state
        }
        break;
    case EVENT_BUTTON_UP:                                                       //  button up
        break;
    }
    return SM_OK;                                                               //  Status is OK
}
static sm_status_t click_idle_exit(sm_t* sm, sm_event_t ev)
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
static sm_status_t dclick_test_enter(sm_t* sm, sm_event_t ev)
{
    dclick_tmr_ct = 0;
    return SM_OK;                                                               //  Status is OK
}
static sm_status_t dclick_test_process(sm_t* sm, sm_event_t ev)
{
    switch(ev)                                                                  //  process button events
    {
    case EVENT_TIMER:                                                           //  timer event
        dclick_tmr_ct += 1;                                                     //  bump timer
        if (dclick_tmr_ct == LONG_PRESS_TIME)                              		//  go to idle after timeout
        {
        	btn_status = btn_click_long_press;
        	sm_state_change(sm, ST_IDLE);                                       //  Change to idle state
        }
        break;
    case EVENT_BUTTON_DN:                                                       //  button down
        break;
    case EVENT_BUTTON_UP:                                                       //  button up
        if (dclick_tmr_ct < CLICK_TIME)
        {
        	btn_status = btn_double_click;
        }
        else
        {
        	btn_status = btn_click_press;
        }
        sm_state_change(sm, ST_IDLE);                                           //  Change to idle state
        break;
    }
    return SM_OK;                                                               //  Status is OK
}
static sm_status_t dclick_test_exit(sm_t* sm, sm_event_t ev)
{
    return SM_OK;                                                               //  Status is OK
}
