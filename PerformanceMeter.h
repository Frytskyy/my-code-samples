//	This is a part of the Cyberium codename project. Part of AllMyNotes Organizer source.
//	Copyright (c) 2000-2022 Volodymyr Frytskyy (owner of Vladonai Software). All rights reserved.
//
//	THIS SOURCE FILE IS THE PROPERTY OF VOLODYMYR FRYTSKYY AND IS NOT TO BE
//	RE-DISTRIBUTED OR USED BY ANY MEANS WHATSOEVER WITHOUT THE EXPRESSED WRITTEN
//	CONSENT OF VOLODYMYR FRYTSKYY.
//
//	CONTACT INFORMATION: support@vladonai.com, www.vladonai.com, www.allmynotes.org
//	License: https://www.vladonai.com/allmynotes-organizer-license-agreement

#pragma once


///////////////////////////////////////////////////////////////////////
//
//	  Class CAppPerformanceMeter is used to measure performance of 
// certain code and log it to the log file.
//


class CAppPerformanceMeter
{
public:
	
	//Methods:
							CAppPerformanceMeter(LPCTSTR lpCodeName = _T(""), bool bStartTimer = true, DWORD allowedExecutionTimeMsec = 500);
	virtual					~CAppPerformanceMeter();
	
	void					Start();
	DWORD					GetExecutionTimeMsec() const;
	void					LogExecutionPoint(LPCTSTR lpPointName, bool bLogFullExecutionTime = false, bool bLogTimeEvenIfShort = false);

protected:
	
	//Data:
	DWORD					m_executionStartTime;
	DWORD					m_allowedExecutionTimeMsec; //if exceeds, there will be waring in the log
	DWORD					m_lastLogPointTime;
	CString					m_codeName;
};

#define MEASURE_CODE_PERFORMANCE_REL(___lpCodeName___)						CAppPerformanceMeter	__performance_log__rel__(___lpCodeName___);
#define MEASURE_CODE_PERFORMANCE_LOG_POINT_REL(___lpPointName___)			__performance_log__rel__.LogExecutionPoint(___lpPointName___);
#define MEASURE_CODE_PERFORMANCE_LOG_POINT_REL_FORCE(___lpPointName___)		__performance_log__rel__.LogExecutionPoint(___lpPointName___, false, true);

#ifdef _DEBUG
	#define MEASURE_CODE_PERFORMANCE_DBG(___lpCodeName___)					CAppPerformanceMeter	__performance_log__dbg__(___lpCodeName___);
	#define MEASURE_CODE_PERFORMANCE_LOG_POINT_DBG(___lpPointName___)		__performance_log__dbg__.LogExecutionPoint(___lpPointName___);
	#define MEASURE_CODE_PERFORMANCE_LOG_POINT_DBG_FORCE(___lpPointName___)	__performance_log__rel__.LogExecutionPoint(___lpPointName___, false, true);
#else
	#define MEASURE_CODE_PERFORMANCE_DBG(___lpCodeName___)					(void(0));
	#define MEASURE_CODE_PERFORMANCE_LOG_POINT_DBG(___lpPointName___)		(void(0));
	#define MEASURE_CODE_PERFORMANCE_LOG_POINT_DBG_FORCE(___lpPointName___)	(void(0));
#endif
