//------------------------------------------------------------------------------
//  File: sm.c
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
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "sm.h"

//------------------------------------------------------------------------------
//	Function:	sm_init
//				Initialize state machine
//
//	Parameters:
//		sm - state machine pointer
//		st - state table
//		ms - max state
//
//	Returns:
//		sm_status_t: state machine status
//------------------------------------------------------------------------------
sm_status_t sm_init(sm_t* sm, const sm_state_fn_t st[], uint32_t max_st)
{
	if (sm == NULL) {
		return SM_ERROR;														//	nothing to do
	}

	if (st == NULL) {
		sm->status = SM_ERROR;													//	...set state error
		return sm->status;														//	Returns: status
	}

	if (max_st == 0ul) {
		sm->status = SM_SC_ERROR;												//	...set state count error
		return sm->status;														//	Returns: status
	}

	sm->max_state = max_st;														//	Set state machine max state
	sm->event_ct = 1ul;															//	initialize event count
	if ((*sm->init_fn) == NULL) {
		sm->curr_state	= (sm_state_t)0;										//	Init function is NULL -- set to first state
		sm->new_state	= sm->curr_state;										//	Init new state to current state
		sm->prev_state	= sm->curr_state;										//	Init previous state to current state
	} else {
		sm->curr_state	= (*sm->init_fn)(sm);									//	Run the init function
		sm->new_state	= sm->curr_state;										//	Init new state to current state
		sm->prev_state	= sm->curr_state;										//	Init previous state to current state
	}

	if (sm->curr_state < sm->max_state) {										//	If less than max state...
		sm->status = (*st[sm->curr_state].enter)(sm, SM_INIT);					//	...enter state
	} else {																	//	If not...
		sm->status = SM_SC_ERROR;												//	...set state count error
	}
	sm->state_change = false;													//	No state change on init

	return sm->status;
}

//------------------------------------------------------------------------------
//	Function:	sm_process
//				Execute current state processing
//
//	Parameters:
//		sm - state machine pointer
//		st - state table
//		ev - event to be processed
//
//	Returns:
//		state machine status
//------------------------------------------------------------------------------
sm_status_t sm_process(sm_t* sm, const sm_state_fn_t st[], sm_event_t ev)
{
	if (sm == NULL) {
		return SM_ERROR;														//	nothing to do
	}

	if (st == NULL) {
		sm->status = SM_ERROR;													//	...set state error
		return sm->status;														//	Returns: status
	}

	if (sm->pause == true) {													//	If sm is paused...
		sm->status = SM_PAUSED;													//	...set pause status
		return sm->status;														//	Returns: status
	}

	if (sm->state_restart == true) {											//	If start restart...
		(*st[sm->curr_state].enter)(sm, SM_ENTER);								//	...reenter current state...
		sm->state_restart = false;												//	disable restart
		sm->state_change = false;												//	set state change to false (should not be run with restart)
	}

	sm->event_ct += 1U;															//	bump event count
	sm->status = (*st[sm->curr_state].process)(sm, ev);							//	Run current state processing
	if ((sm->state_change == true) && (sm->curr_state != sm->new_state)) {		//	If SM to change state, and the new state is different...
		(*st[sm->curr_state].exit)(sm, SM_EXIT);								//	...exit old state...
		sm->prev_state = sm->curr_state;										//	...set prev state...
		sm->curr_state = sm->new_state;											//	...set new state...
		(*st[sm->curr_state].enter)(sm, SM_ENTER);								//	...enter new state...
		sm->state_change = false;												//	...reset state change
		sm->status = SM_STATE_CHANGE;											//	...note state change
	}
	return sm->status;
}

//------------------------------------------------------------------------------
//	Function:	sm_state_change
//				Change state machine state. Change to a new state.
//
//	Parameters:
//		sm - state machine pointer
//		st - next state
//
//	Returns:
//		state machine status
//------------------------------------------------------------------------------
sm_status_t sm_state_change(sm_t* sm, sm_state_t st)
{
	if (sm == NULL) {
		return SM_ERROR;														//	nothing to do
	}

	if (st < sm->max_state) {													//	If less than max state...
		sm->new_state = st;														//	...set new state...
		sm->state_change = true;													//	...set state change to true...
		sm->status = SM_STATE_CHANGE;											//	Set state machine status to state changed
	} else {																	//	If new state is not in range...
		sm->status = SM_SC_ERROR;												//	...set state count error
	}
	return sm->status;
}

//------------------------------------------------------------------------------
//	Function:	sm_state_auto
//				Change state machine state.
//				Change states based on a table of events x states.
//
//	Parameters:
//		sm - state machine pointer
//		next_state - next state
//
//	Returns:
//		sm_status_t: state machine status
//------------------------------------------------------------------------------
sm_status_t sm_state_auto(sm_t* sm, sm_state_t next_state)
{
	sm_state_t new_state;														//	used to determine next state
	if (sm == NULL) {
		return SM_ERROR;														//	nothing to do
	}

	new_state = next_state;														//	set next state
	switch (new_state) {														//	switch on new state
	case ST_IGNORE:																//	meta-state ST_IGNORE, don't do anything
		break;																	//	exit
	case ST_RESTART:															//	meta-state ST_RESTART, restart the current state
		sm->state_restart = true;												//	...set state change to true
		sm->state_change = false;												//	...set state change to false (should not be run with restart)
		sm->status = SM_STATE_RESTART;											//	Set state machine status to state changed
		break;																	//	exit
	default:																	//	normal state processing
		sm->new_state = new_state;												//	...get next state
		sm->state_restart = false;												//	...set state change to false
		sm->state_change = true;												//	...set state change to true
		sm->status = SM_STATE_CHANGE;											//	Set state machine status to state changed
		break;																	//	exit
	}

	return sm->status;
}

//------------------------------------------------------------------------------
//	Function:	sm_state_next
//				Change state machine state.
//				Change states based on a state change table.
//
//	Parameters:
//		sm - state machine pointer
//		sa - state table to select next state
//
//	Returns:
//		state machine status
//------------------------------------------------------------------------------
sm_status_t sm_state_next(sm_t* sm, sm_state_t const sa[])
{
	if (sm == NULL) {
		return SM_ERROR;														//	nothing to do
	}

	sm->new_state = sa[sm->curr_state];											//	Get next state from event table
	sm->state_change = true;													//	Set state change to true...
	sm->status = SM_STATE_CHANGE;												//	Set state machine status to state changed
	return sm->status;
}
//------------------------------------------------------------------------------
//	Function:	sm_state_prev
//		Change state machine state. Change to the previous state.
//
//	Parameters:
//		sm - state machine pointer
//
//	Returns:
//		state machine status
//------------------------------------------------------------------------------
sm_status_t sm_state_prev(sm_t* sm)
{
	if (sm == NULL) {
		return SM_ERROR;														//	nothing to do
	}

	sm->new_state = sm->prev_state;												//	Set state to previous state
	sm->state_change = true;													//	Set state change to true
	sm->status = SM_STATE_CHANGE;												//	Set state machine status to state changed
	return sm->status;
}

//------------------------------------------------------------------------------
//	Function:	sm_pause
//				Pause this state machine.
//				Will only pause an SM that has no errors.
//
//	Parameters:
//		sm - state machine pointer
//
//	Returns:
//		state machine status
//------------------------------------------------------------------------------
sm_status_t sm_pause(sm_t* sm, const sm_state_fn_t st[])
{
	if (sm == NULL) {
		return SM_ERROR;														//	nothing to do
	}

	if (sm->pause == false) {													//	If not paused, then...
		(*st[sm->curr_state].exit)(sm, SM_EXIT);								//	pause current state
		sm->pause = true;														//	Set SM as paused
		sm->status = SM_PAUSED;													//	Set state machine to paused
	}
	return sm->status;
}

//------------------------------------------------------------------------------
//	Function:	sm_resume
//				Resume state machine processing.
//
//	Parameters:
//		sm - state machine pointer
//		st - State table
//
//	Returns:
//		state machine status
//------------------------------------------------------------------------------
sm_status_t sm_resume(sm_t* sm, const sm_state_fn_t st[])
{
	if (sm == NULL) {
		return SM_ERROR;														//	nothing to do
	}

	if (sm->pause == true) {													//	If	paused, then...
		(*st[sm->curr_state].enter)(sm, SM_ENTER);								//	resume current state
		sm->pause = false;														//	Set SM as running
		sm->status = SM_OK;														//	Set state machine to OK
	}
	return sm->status;
}

//------------------------------------------------------------------------------
//	Function:	sm_user_data_get
//				Get user data.
//
//	Parameters:
//		sm - state machine pointer
//
//	Returns:
//		pointer to the user data
//------------------------------------------------------------------------------
void* sm_user_data_get(const sm_t* sm)
{
	return (sm == NULL) ? NULL : sm->user_data;
}

//------------------------------------------------------------------------------
//	Function:	sm_get_state
//				Access function to retrieve state machine current state.
//
//	Parameters:
//		sm - state machine pointer
//
//	Returns:
//		state machine status
//------------------------------------------------------------------------------
sm_state_t sm_get_state(const sm_t* sm)
{
	return (sm == NULL) ? 0U : sm->curr_state;
}

//------------------------------------------------------------------------------
//	Function:	sm_get_status
//				Access function to retrieve state machine current status.
//
//	Parameters:
//		sm - state machine pointer
//
//	Returns:
//		state machine status
//------------------------------------------------------------------------------
sm_status_t sm_get_status(const sm_t* sm)
{
	return (sm == NULL) ? SM_ERROR : sm->status;
}

//------------------------------------------------------------------------------
//	Function:	sm_get_event_ct
//				Access function to retrieve state machine current state count.
//
//	Parameters:
//		sm - state machine pointer
//
//	Returns:
//		number of events processed by this state machine
//------------------------------------------------------------------------------
uint32_t sm_get_event_ct(const sm_t* sm)
{
	return (sm == NULL) ? 0U : sm->event_ct;
}
