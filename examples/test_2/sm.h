//------------------------------------------------------------------------------
//  File: sm.h
//  Purpose: generalized state machine
//
//	COPYRIGHT 2018 TRILINEAR TECHNOLOGIES, INC.
//	CONFIDENTIAL AND PROPRIETARY
//
//	THE SOURCE CODE CONTAINED HEREIN IS PROVIDED ON AN "AS IS" BASIS.
//	TRILINEAR TECHNOLOGIES, INC. DISCLAIMS ANY AND ALL WARRANTIES,
//	WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING ANY IMPLIED
//	WARRANTIES OF MERCHANTABILITY OR OF FITNESS FOR A PARTICULAR PURPOSE.
//	IN NO EVENT SHALL TRILINEAR TECHNOLOGIES, INC. BE LIABLE FOR ANY
//	INCIDENTAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES OF ANY KIND WHATSOEVER
//	ARISING FROM THE USE OF THIS SOURCE CODE.
//
//	THIS DISCLAIMER OF WARRANTY EXTENDS TO THE USER OF THIS SOURCE CODE
//	AND USER'S CUSTOMERS, EMPLOYEES, AGENTS, TRANSFEREES, SUCCESSORS,
//	AND ASSIGNS.
//
//	THIS IS NOT A GRANT OF PATENT RIGHTS
//------------------------------------------------------------------------------
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//	Typedef variables for state machine
//------------------------------------------------------------------------------
typedef uint32_t	sm_event_t;													//	Events passed to state machine
typedef uint32_t	sm_state_t;													//	Current state
typedef enum {
	SM_OK,																		//	State machine is running well
	SM_PAUSED,																	//	State machine is paused
	SM_NO_STATE_CHANGE,															//	State machine did not change states
	SM_STATE_CHANGE,															//	State machine changed states
	SM_STATE_RESTART,															//	State machine state restart
	SM_ERROR,																	//	State machine error
	SM_SC_ERROR,																//	State machine state count error
	SM_EC_ERROR,																//	State machine event count error
	SM_SIG_ERROR,																//	State machine signature error
} sm_status_t;																	//	Current status of the state machine

//------------------------------------------------------------------------------
//	Define SM control events
//------------------------------------------------------------------------------
enum {
	SM_INIT,
	SM_ENTER,
	SM_EXIT,
};

//------------------------------------------------------------------------------
//	Auto state change tags
//------------------------------------------------------------------------------
#define	ST_IGNORE	(sm_state_t)0xF000
#define	ST_RESTART	(sm_state_t)0xF001
#define	ST_UNKNOWN	(sm_state_t)0xF002

//------------------------------------------------------------------------------
//	Forward reference so that STATE_FN can be declared
//------------------------------------------------------------------------------
typedef struct sm_tag sm_t;														//	Forward reference

//------------------------------------------------------------------------------
//	Typedef functions for state machine
//------------------------------------------------------------------------------
typedef sm_status_t	(*state_fn_t)(sm_t* sm, sm_event_t ev);						//	State processing function
typedef sm_state_t	(*sm_init_t)(sm_t* sm);										//	Init function

//------------------------------------------------------------------------------
//	Typedef structures for state machine
//------------------------------------------------------------------------------
typedef struct {
	state_fn_t		enter;														//	Enter function
	state_fn_t		process;													//	Processing function
	state_fn_t		exit;														//	Exit function
	const char*		name;														//	State name
} sm_state_fn_t;

struct sm_tag {
	void*			user_data;													//	user data
	uint32_t		event_ct;													//	count events
	uint32_t		signature;													//	state machine signature
	sm_state_t		curr_state;													//	SM state
	sm_state_t		new_state;													//	SM next state
	sm_state_t		prev_state;													//	SM last state
	sm_status_t		status;														//	SM status
	sm_init_t		init_fn;													//	SM initial function
	bool			state_change;												//	Change state
	bool			state_restart;												//	Restart the state
	bool			pause;														//	Pause state
	uint32_t		max_state;													//	Max size of state array: so we don't go off the end!
};

//------------------------------------------------------------------------------
//	Function declaration
//------------------------------------------------------------------------------
sm_status_t		sm_init(sm_t* sm, const sm_state_fn_t st[], uint32_t max_st);
sm_status_t		sm_process(sm_t* sm, const sm_state_fn_t st[], sm_event_t ev);
sm_status_t		sm_state_change(sm_t* sm, sm_state_t st);
sm_status_t		sm_state_auto(sm_t* sm, sm_state_t next_state);
sm_status_t		sm_state_next(sm_t* sm, sm_state_t const sa[]);
sm_status_t		sm_state_prev(sm_t* sm);

sm_status_t		sm_pause(sm_t* sm, const sm_state_fn_t st[]);
sm_status_t		sm_resume(sm_t* sm, const sm_state_fn_t st[]);

void*			sm_user_data_get(const sm_t* sm);

sm_state_t		sm_get_state(const sm_t* sm);
sm_status_t		sm_get_status(const sm_t* sm);
uint32_t		sm_get_event_ct(const sm_t* sm);

#ifdef __cplusplus
}
#endif
