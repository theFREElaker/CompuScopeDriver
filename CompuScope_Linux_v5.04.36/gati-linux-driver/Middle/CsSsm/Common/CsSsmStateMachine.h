
#pragma once

#ifdef __linux__

#include <CsLinuxPort.h>
#include <CWinEventHandle.h> //for EVENT_HANDLE for now, change to other file

#endif

#include "CsTypes.h"
#include "CsSSMStateMachine_sm.h"
#include "CsSsm.h"

#include <vector>

using namespace std;

#define ERROR_INVALID_TRANSITION	0x1001


enum TRANSITIONS
{
	INIT_TRANSITION	= 1,	//!< Initial Transition
	ABORT_TRANSITION,		//!< Abort Transition
	BADMODIFY_TRANSITION,	//!< Invalid Modification Transition
	COERCE_TRANSITION,		//!< Coerce Transition
	DONE_TRANSITION,		//!< Done Transition
	FORCETRIGGER_TRANSITION,//!< Force Trigger Transition
	HWEVENT_TRANSITION,		//!< Hardware Event Transition
	INVALID_TRANSITION,		//!< Invalid (Mode) Transition
	MODIFY_TRANSITION,		//!< Modify Transition
	READ_TRANSITION,		//!< Read (Data) Transition
	START_TRANSITION,		//!< Start (Capture) Transition
	TRANSFER_TRANSITION,	//!< Transfer Transition
	VALID_TRANSITION,		//!< Valid (Mode) Transition
	COMMIT_TRANSITION,		//!< Commit Transition
	RESET_TRANSITION,		//!< Commit Transition
	DISKSTREAM_TRANSITION,  //!< DiskStream Transitition
	CALIBRATE_TRANSITION,		//!< Commit Transition
};

enum STATE_ID
{
	SSM_INIT_ID = 0,				//!< Initial State
	SSM_VALIDCONFIGURATION_ID,		//!< Valid Configuration State
	SSM_INVALIDCONFIGURATION_ID,	//!< Invalid Configuration State
	SSM_ARMING_ID,					//!< Arming State
	SSM_READY_ID,					//!< Ready State
	SSM_WAIT_ID,					//!< Wait State (Wait for Trigger)
	SSM_BUSY_ID,					//!< Busy State (Acquiring)
	SSM_DATAREADY_ID,				//!< Data ready
	SSM_VALIDMODE_ID,				//!< Valid Mode State (Memory Transfer)
	SSM_INVALIDMODE_ID,				//!< Invalid Mode State (Memory Transfer)
	SSM_MEMORYACCESS_ID,			//!< Memory Access State
	SSM_ARMINGDATAREADY_ID,			//!< Memory Access State
	SSM_DISKSTREAM_ID,				//!< DiskStream state
	SSM_ERROR_ID					//!< Error State
};


class CStateMachine
{
private:
	CStateMachineContext* _fsm;

	CSHANDLE	m_hSystem;

	EVENT_HANDLE	m_hReleaseSystemEvent;
	EVENT_HANDLE	m_hThreadEndEvent;
	EVENT_HANDLE	m_hThreadEndEventTxfer;
	EVENT_HANDLE	m_hTriggerEvent;
	EVENT_HANDLE	m_hAcquisitionEndEvent;
	EVENT_HANDLE	m_hEndTransferEvent;

#ifdef _WIN32
	EVENT_HANDLE	m_hApp_TriggerEvent;
	EVENT_HANDLE	m_hApp_AcquisitionEndEvent;
	EVENT_HANDLE	m_hAppEndTransferEvent;
#else
	int				m_hApp_TriggerEvent[2];
	int				m_hApp_AcquisitionEndEvent[2];
	int				m_hAppEndTransferEvent_read;
	int				m_hAppEndTransferEvent_write;
#endif /* _WIN32 */

	bool	m_bSMLock;
	bool	m_bAborted;

	//queue<int16 > m_transitionQueue;
	vector<int>						m_InDataIndexVector;
	vector<PIN_PARAMS_TRANSFERDATA> m_InDataStructVector;

	CRITICAL_SECTION m_Sec;

public:
	CStateMachine(CSHANDLE hSystem);   //!< default constructor should not be called
	~CStateMachine();

	int32 RetrieveStateID(void);


	int32 RetrieveStateName(LPTSTR szBuffer, int16 nMaxCount);


	bool PerformTransition(int16 nTransition);


	int32 RegisterDriverEvents(CSHANDLE hSystem);


	void SignalEvent(int16 EventType);


	void ClearEvent(int16 EventType);


	// Get Events
	EVENT_HANDLE GetSystemReleaseEvent(void) {return (m_hReleaseSystemEvent);};
	EVENT_HANDLE GetThreadEndEvent(void) {return (m_hThreadEndEvent);};
	EVENT_HANDLE GetThreadEndEventTxfer(void) {return (m_hThreadEndEventTxfer);};
	EVENT_HANDLE GetTriggerEvent(void) {return (m_hTriggerEvent);};
	EVENT_HANDLE GetAcquisitionEndEvent(void) {return (m_hAcquisitionEndEvent);};

#ifdef _WIN32
	EVENT_HANDLE GetAppTriggerEvent(void) {return (m_hApp_TriggerEvent);};
	EVENT_HANDLE GetAppAcquisitionEndEvent(void) {return (m_hApp_AcquisitionEndEvent);};
#else
	int GetAppTriggerEvent(void) {return (m_hApp_TriggerEvent[0]);};
	int GetAppAcquisitionEndEvent(void) {return (m_hApp_AcquisitionEndEvent[0]);};
#endif /* _WIN32 */

	EVENT_HANDLE* GetTransferEndEvent(void) {return (&m_hEndTransferEvent);};

#ifdef _WIN32
	void   SetAppEndTransferEvent(EVENT_HANDLE hEvent) {m_hAppEndTransferEvent = hEvent;};
#else
	void   SetAppEndTransferEvent(int hEvent_read, int hEvent_write) {
		m_hAppEndTransferEvent_read = hEvent_read;
		m_hAppEndTransferEvent_write = hEvent_write;
	};
#endif /* _WIN32 */

	CSHANDLE GetSystemHandle(void) {return (m_hSystem);};

	void LockSM(bool bEnable);
	bool IsSMLock(void);

	void Abort(bool bAbort);
	bool IsAborted(void);

	// AS Transfer helper routine
	PIN_PARAMS_TRANSFERDATA AllocateInStruct(void);
	void ReleaseInStruct(PIN_PARAMS_TRANSFERDATA);

	// State machine support function
	void SetFlag(int32);
	void ResetFlags(void);

	void RaiseError(int);

	// Debug function
	void Set_Debug_Trace(int8, ostream&);
	LPCTSTR Dump(void);
};

