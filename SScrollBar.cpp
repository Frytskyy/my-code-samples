//	This is a part of the Cyberium codename project. Part of AllMyNotes Organizer source.
//	Copyright (c) 2000-2024 Volodymyr Frytskyy. All rights reserved.
//
//	THIS SOURCE FILE IS THE PROPERTY OF VOLODYMYR FRYTSKYY AND IS NOT TO BE
//	RE-DISTRIBUTED OR USED BY ANY MEANS WHATSOEVER WITHOUT THE EXPRESSED WRITTEN
//	CONSENT OF VOLODYMYR FRYTSKYY.
//
//	CONTACT INFORMATION: https://www.vladonai.com/about-resume, www.vladonai.com, www.allmynotes.org
//	License: https://www.vladonai.com/allmynotes-organizer-license-agreement

#include "StdAfx.h"
#include "SScrollBar.h"
#include "graph.h"
#include "UIUtils.h"
#include "Messages.h"
#include "SkinBase.h"
#include "SoundThemesMgr.h"
#include <math.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define REPEAT_TIMER_EVENT_ID				1
#define SCROLL_AUTO_REPEAT_DELAY_TIME		150

#define SMOOTH_SCROLLING_TIMER_ID			8734
#define SMOOTH_SCROLLING_TIMER_INTERVAL		(1000 / 40) //in milliseconds


BEGIN_MESSAGE_MAP(CSScrollBar, CWnd)
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_TIMER()
	ON_WM_KILLFOCUS()
	ON_WM_PAINT()
	ON_WM_ENABLE()
	ON_WM_CAPTURECHANGED()
END_MESSAGE_MAP()


CSScrollBar::CSScrollBar(eType type /*= etVertical*/, eMode mode /*= emLimitScrollingToMaxPosMinusLastPage*/) :
	m_pSkin(CSkinBase::GetInstanceBase()),
	m_onePageSize(1000),
	m_oneStepSize(0),
	m_minScrollPos(0),
	m_maxScrollPos(1000),
	m_curScrollPos(0),
	m_bMouseOverCtl(false),
	m_bMouseBtnPressed(false),
	m_bDisabledDueToRange(false),
	m_bEnabled(true),
	m_RepeatTimer(0),
	m_id(0),
	m_ActiveComponent(escNone),
	m_type(type),
	m_mode(mode),
	m_pCaptureBeforeButtonDown(NULL),
#ifdef ENABLE_SMOOTH_SCROLLING_FEATURE
	m_bEnableSmoothScrolling(true),
#else
	m_bEnableSmoothScrolling(false),
#endif
	m_bSmoothScrollingInProgress(false),
	m_smoothScrollingTimerID(NULL)
{
	ASSERT(m_mode == emLimitScrollingToMaxPos || m_mode == emLimitScrollingToMaxPosMinusLastPage);
	ASSERT(m_type == etVertical || m_type == etHorizontal);
}

/*virtual*/ CSScrollBar::~CSScrollBar()
{
	if (m_hWnd &&
		::IsWindow(m_hWnd))
	{
		DestroyWindow();
		m_hWnd = NULL;
	}
}

BOOL CSScrollBar::Create(CWnd* pParent, UINT uID, bool bVisible /*= true*/)
{
	DWORD	dwStyle = WS_CHILD | (bVisible ? WS_VISIBLE : 0) | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	LPCTSTR	className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(NULL, IDC_ARROW), NULL, NULL);
	BOOL	rv = CWnd::Create(className, _T("Playing with Spy++? ;-)"), dwStyle, CRect(0, 0, 0, 0), pParent, uID);

	ASSERT(rv);
	m_id = uID;
	
	return rv;
}

void CSScrollBar::SetScrollInfo2(INT64 onePageSize, INT64 oneStepSize, INT64 minScrollPos, INT64 maxScrollPos, INT64 curScrollPos, BOOL bRedraw /*= TRUE*/)
{
	ASSERT(minScrollPos <= curScrollPos);
	ASSERT(maxScrollPos >= minScrollPos);

	if (m_bSmoothScrollingInProgress)
		FinishSmoothScrolling(false);
	
	m_onePageSize  = onePageSize;
	m_oneStepSize  = oneStepSize;
	m_minScrollPos = minScrollPos;
	m_maxScrollPos = maxScrollPos;
	m_curScrollPos = curScrollPos;
	ValidateNewPos();

	if (bRedraw)
		Invalidate();
}

void CSScrollBar::ValidateNewPos()
{
	ASSERT(m_minScrollPos <= m_curScrollPos);
	ASSERT(m_maxScrollPos >= m_minScrollPos);
	if ((m_mode == emLimitScrollingToMaxPosMinusLastPage && m_onePageSize > m_maxScrollPos - m_minScrollPos) ||
		(m_mode == emLimitScrollingToMaxPos && m_maxScrollPos <= m_minScrollPos))
	{
		ASSERT(m_maxScrollPos >= m_minScrollPos);
		m_onePageSize = m_maxScrollPos - m_minScrollPos;
		m_bDisabledDueToRange = true;
	} else
	{
		m_bDisabledDueToRange = false;
	}
	ASSERT(m_curScrollPos + (m_mode == emLimitScrollingToMaxPosMinusLastPage ? (m_onePageSize - 1) : 0) - m_minScrollPos <= m_maxScrollPos - m_minScrollPos);
}

void CSScrollBar::SetMaxPos(INT64 maxScrollPos)
{
	if (m_bSmoothScrollingInProgress)
		FinishSmoothScrolling(false);

	m_maxScrollPos = maxScrollPos;
	ValidateNewPos();
	
	Invalidate();
}

void CSScrollBar::SetPageSize(INT64 onePageSize)
{
	if (m_bSmoothScrollingInProgress)
		FinishSmoothScrolling(false);

	m_onePageSize = onePageSize;
	ValidateNewPos();
	
	Invalidate();
}

void CSScrollBar::SetScrollPos(INT64 curScrollPos, int recommendedSmoothScrollingTime /*= SMOOTH_SCROLLING_RECOMMENDED_TIME*/)
{
	if (m_curScrollPos != curScrollPos)
	{
		ASSERT(curScrollPos + (m_mode == emLimitScrollingToMaxPosMinusLastPage ? (m_onePageSize - 1) : 0) <= m_maxScrollPos);
		
//		if (m_bSmoothScrollingInProgress)
//			FinishSmoothScrolling(recommendedSmoothScrollingTime <= 0);

		if (recommendedSmoothScrollingTime > 0)
			StartSmoothScrolling(curScrollPos, recommendedSmoothScrollingTime);

		m_curScrollPos = curScrollPos;
		ValidateNewPos();
		
		RedrawWindow();
	}
}

void CSScrollBar::GetScrollInfo2(INT64& onePageSize, INT64& oneStepSize, INT64& minScrollPos, INT64& maxScrollPos, INT64& curScrollPos) const
{
	onePageSize  = m_onePageSize;
	oneStepSize  = m_oneStepSize;
	minScrollPos = m_minScrollPos;
	maxScrollPos = m_maxScrollPos;
	curScrollPos = m_curScrollPos;
}

BOOL CSScrollBar::GetScrollInfo(LPSCROLLINFO lpScrollInfo, UINT nMask /*= SIF_ALL*/)
{
	//for compatibility with standard scrollbar (just to make code more maintainable)
	if (nMask & SIF_RANGE)
	{
		ASSERT(m_minScrollPos >= MININT_PTR && m_minScrollPos <= MAXINT_PTR);
		ASSERT(m_maxScrollPos >= MININT_PTR && m_maxScrollPos <= MAXINT_PTR);
		lpScrollInfo->nMin = (int)m_minScrollPos;
		lpScrollInfo->nMax = (int)m_maxScrollPos;
	}

	if (nMask & SIF_PAGE)
	{
		ASSERT(m_onePageSize >= MININT_PTR && m_onePageSize <= MAXINT_PTR);
		lpScrollInfo->nPage = (int)m_onePageSize;
	}
	
	if (nMask & SIF_POS)
	{
		ASSERT(m_curScrollPos >= MININT_PTR && m_curScrollPos <= MAXINT_PTR);
		lpScrollInfo->nPos = (int)m_curScrollPos;
	}

	if (nMask & SIF_TRACKPOS)
	{
		ASSERT(m_curScrollPos >= MININT_PTR && m_curScrollPos <= MAXINT_PTR);
		lpScrollInfo->nTrackPos = (int)m_curScrollPos;
	}

	return nMask != 0;
}

INT64 CSScrollBar::GetScrollPos(/*bool bPaintingScreen /*= false*/)
{
#ifdef ENABLE_SMOOTH_SCROLLING_FEATURE
	if (m_bSmoothScrollingInProgress && m_bEnableSmoothScrolling)
	{
		return m_smoothScrollingCurScrollPos;
	} else
#endif //ENABLE_SMOOTH_SCROLLING_FEATURE
	{
		return m_curScrollPos;
	}
}

/*static*/ int CSScrollBar::GetVScrollBarWidth()
{
	return CSkinBase::GetInstanceBase()->GetVScrollBarWidth();
}

/*static*/ int CSScrollBar::GetHScrollBarHeight()
{
	return CSkinBase::GetInstanceBase()->GetHScrollBarHeight();
}

eScrollBarComponent CSScrollBar::GetComponentFromCoord(CPoint point) const
{
	CRect		scrollbarRect;
	
	GetClientRect(scrollbarRect);

	eScrollBarComponent component = m_type == etVertical ?
										m_pSkin->GetVScrollBarComponentFromCoord(scrollbarRect, point, m_minScrollPos, m_maxScrollPos + (m_mode == emLimitScrollingToMaxPosMinusLastPage ? 0 : (m_onePageSize - 1)), m_onePageSize, m_curScrollPos) :
										m_pSkin->GetHScrollBarComponentFromCoord(scrollbarRect, point, m_minScrollPos, m_maxScrollPos + (m_mode == emLimitScrollingToMaxPosMinusLastPage ? 0 : (m_onePageSize - 1)), m_onePageSize, m_curScrollPos);

	return component;
}

void CSScrollBar::OnPaint()
{
	CADC			dc(this);//it appears that this UI control is flicker-free, so no need in double-buffering:, DOUBLE_BUFFERING);
	CRect			scrollbarRect;
	BOOL			bEnabled = IsScrollBarEnabled(true);
	INT64			curScrollPos = m_bSmoothScrollingDisplayScrollerAtFinalPos ? m_curScrollPos : GetScrollPos();
	
	GetClientRect(scrollbarRect);

	if (m_type == etVertical)
	{
		m_pSkin->DrawVScrollBar(dc, scrollbarRect, bEnabled, m_ActiveComponent, m_bMouseBtnPressed, m_minScrollPos, m_maxScrollPos + (m_mode == emLimitScrollingToMaxPosMinusLastPage ? 0 : (m_onePageSize - 1)), m_onePageSize, curScrollPos);
	} else
	{
		m_pSkin->DrawHScrollBar(dc, scrollbarRect, bEnabled, m_ActiveComponent, m_bMouseBtnPressed, m_minScrollPos, m_maxScrollPos + (m_mode == emLimitScrollingToMaxPosMinusLastPage ? 0 : (m_onePageSize - 1)), m_onePageSize, curScrollPos);
	}

	if (m_bSmoothScrollingInProgress)
		PostMessage(WM_TIMER, m_smoothScrollingTimerID);
}

void CSScrollBar::OnMouseLeave()
{
	m_bMouseOverCtl = false;
	m_ActiveComponent = escNone;
	RedrawWindow();
}

void CSScrollBar::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bMouseBtnPressed)
	{
		eScrollBarComponent componentUnderMouse = GetComponentFromCoord(point);

		if (m_ActiveComponent != componentUnderMouse)
		{
			if (componentUnderMouse != escUpperFreeSpace &&
				componentUnderMouse != escBottomFreeSpace &&
				componentUnderMouse != escNone)
				PlaySoundForEvent(SND_NAME_MOUSE_HOOVER, __FUNCTION__);

			m_ActiveComponent = componentUnderMouse;
			RedrawWindow();
		}
	}

	if (!m_bMouseOverCtl)
	{
		m_bMouseOverCtl = true;
		EnableMouseLeaveWndNotification(m_hWnd);
		RedrawWindow();
	} else
	if (m_bMouseBtnPressed)
	{
		if (m_ActiveComponent == escScroller)
		{
			CRect		scrollbarRect;
			int			scrollableAreaLen;
			int			scrollerLen;
			
			GetClientRect(scrollbarRect);
			if (m_type == etVertical)
				m_pSkin->GetVScrollBarMetrices(scrollbarRect.Height() + 1, m_minScrollPos, m_maxScrollPos + (m_mode == emLimitScrollingToMaxPosMinusLastPage ? 0 : (m_onePageSize - 1)), m_onePageSize, m_curScrollPos, &scrollableAreaLen, &scrollerLen, NULL);
			else
				m_pSkin->GetHScrollBarMetrices(scrollbarRect.Width() + 1, m_minScrollPos, m_maxScrollPos + (m_mode == emLimitScrollingToMaxPosMinusLastPage ? 0 : (m_onePageSize - 1)), m_onePageSize, m_curScrollPos, &scrollableAreaLen, &scrollerLen, NULL);

			//
			INT64	oldScrollPos = GetScrollPos();
			INT64	sumCrollerPlusAreaLen = scrollableAreaLen + scrollerLen;
			double	convProportion = sumCrollerPlusAreaLen != 0 ? (double(m_maxScrollPos + (m_mode == emLimitScrollingToMaxPosMinusLastPage ? 0 : (m_onePageSize - 1)) - m_minScrollPos) / sumCrollerPlusAreaLen) : 0;
			INT64	newScrollPos = NormalizeScrollerPos(m_dragOriginalPos + INT64(convProportion * (((m_type == etVertical) ? point.y : point.x ) - m_dragStartCoord)));

			if (newScrollPos != oldScrollPos)
			{
				SetScrollPos(newScrollPos, SMOOTH_SCROLLING_RECOMMENDED_TIME / 3);

				SendNotfication();
			}
		}
	}
}

void CSScrollBar::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (IsScrollBarEnabled(true))
	{
		m_ActiveComponent = GetComponentFromCoord(point);

		if (m_ActiveComponent == escNone)
			return;
		
		m_bMouseBtnPressed = true;
		m_pCaptureBeforeButtonDown = SetCapture();
		if (m_ActiveComponent != escScroller)
		{
			m_RepeatTimer = SetTimer(REPEAT_TIMER_EVENT_ID, SCROLL_AUTO_REPEAT_DELAY_TIME, NULL);
			PlaySoundForEvent(SND_NAME_MOUSE_HOOVER, __FUNCTION__);
			ProcessAction();
		} else
		{
			m_dragStartCoord = m_type == etVertical ? point.y : point.x;
			m_dragOriginalPos = m_curScrollPos;
		}
		RedrawWindow();
	}

	CWnd* pOwner = GetOwner();
	
	if (pOwner)
		pOwner->SetFocus();
}

void CSScrollBar::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_RepeatTimer)
	{
		KillTimer(m_RepeatTimer);
		m_RepeatTimer = NULL;
	}
	if (m_bMouseBtnPressed)
	{
		m_ActiveComponent = GetComponentFromCoord(point);
		EndMouseDownState();
	}
}

void CSScrollBar::EndMouseDownState()
{
	if (m_bMouseBtnPressed)
	{
		m_bMouseBtnPressed = false;
		ReleaseCapture();
		if (m_pCaptureBeforeButtonDown)
			m_pCaptureBeforeButtonDown->SetCapture();
		RedrawWindow();
	}
}

void CSScrollBar::OnCaptureChanged(CWnd* pWnd)
{
	if (m_bMouseBtnPressed)
	{
		m_ActiveComponent = escNone;
		OnLButtonUp(0, CPoint(0, 0));
	}
}

void CSScrollBar::OnTimer(UINT_PTR nIDEvent)
{
	try
	{
		if (m_RepeatTimer &&
			nIDEvent == m_RepeatTimer &&
			IsScrollBarEnabled(true))
		{
			KillTimer(m_RepeatTimer);
			m_RepeatTimer = SetTimer(REPEAT_TIMER_EVENT_ID, AUTO_REPEAT_INTERVAL, NULL);
			ProcessAction();
		} else
#ifdef ENABLE_SMOOTH_SCROLLING_FEATURE
		if (m_smoothScrollingTimerID &&
			nIDEvent == m_smoothScrollingTimerID)
		{
			if (m_bSmoothScrollingInProgress &&
				m_smoothScrollingScreenPaintingStats.IsOkToUseSmoothScrolling())
			{
				DWORD			curTickTime = GetTickCount();
				int				smoothScrollingTime = curTickTime - m_smoothScrollingStartTime;
				double			transitionValueLinear = (double(curTickTime - m_smoothScrollingStartTime) / (m_smoothScrollingEndTime - m_smoothScrollingStartTime)); // 0..1 value, linearly scaled by time
				//double		transitionValueLog = (m_bMouseBtnPressed && (m_ActiveComponent == escDownArrow || m_ActiveComponent == escUpArrow)) ? transitionValueLinear : pow(transitionValueLinear, 1.6); // 0..1 value
				bool			bUseLinearScrolling = m_bMouseBtnPressed && (m_ActiveComponent == escDownArrow || m_ActiveComponent == escUpArrow || m_ActiveComponent == escUpperFreeSpace || m_ActiveComponent == escBottomFreeSpace);
				//double		transitionValueCorrected = max(0, min(1, bUseLinearScrolling ? transitionValueLinear : sqrt(transitionValueLinear))); // 0..1 value
				double			transitionValueCorrected = max(0, min(1, transitionValueLinear)); // 0..1 value

				if (transitionValueCorrected >= 0 && transitionValueCorrected < 1)
				{
					INT64		oldSmoothScrollingCurScrollPos = m_smoothScrollingCurScrollPos;

					m_smoothScrollingCurScrollPos = m_smoothScrollingStartScrollPos + (INT64)((m_curScrollPos - m_smoothScrollingStartScrollPos) * transitionValueCorrected);
					ASSERT(m_smoothScrollingCurScrollPos >= min(m_smoothScrollingStartScrollPos, m_curScrollPos) && m_smoothScrollingCurScrollPos <= max(m_smoothScrollingStartScrollPos, m_curScrollPos));
// CString msg;
// msg.Format(_T("Scroll Pos: %d   ---  %f  ---   %d ms \r\n"), (int)m_smoothScrollingCurScrollPos, transitionValueLinear, curTickTime - m_smoothScrollingStartTime);
// TRACE(msg);
					if (oldSmoothScrollingCurScrollPos != m_smoothScrollingCurScrollPos)
					{
						CWnd*			pOwner = GetOwner();
						const DWORD		startTick = GetTickCount();

						if (!m_bSmoothScrollingDisplayScrollerAtFinalPos)
							RedrawWindow();
						if (pOwner)
							pOwner->RedrawWindow();
						else
							SendNotfication();
						m_smoothScrollingScreenPaintingStats.AddPaintingTime(GetTickCount() - startTick);
					}

//					if (m_bSmoothScrollingInProgress)
//						PostMessage(WM_TIMER, m_smoothScrollingTimerID);
				} else
				{
					FinishSmoothScrolling();
				}
			} else
			{
				FinishSmoothScrolling();
			}
		}
#endif //ENABLE_SMOOTH_SCROLLING_FEATURE
		
		CWnd::OnTimer(nIDEvent);
	}
	catch (...)
	{
		ASSERT(false); //must never happen
	}
}

void CSScrollBar::OnKillFocus(CWnd* pNewWnd) 
{
	RedrawWindow();
	CWnd::OnKillFocus(pNewWnd);	
}

/*virtual*/ BOOL CSScrollBar::PreTranslateMessage(MSG* pMsg)
{
	BOOL	bProcessed = FALSE;

	if (!bProcessed)
		bProcessed = CWnd::PreTranslateMessage(pMsg);

	return bProcessed;
}

void CSScrollBar::EnableSmoothScrolling(bool bEnableSmoothScrolling)
{
#ifdef ENABLE_SMOOTH_SCROLLING_FEATURE
	if (m_bEnableSmoothScrolling != bEnableSmoothScrolling)
	{
		m_bEnableSmoothScrolling = bEnableSmoothScrolling;

		if (m_bSmoothScrollingInProgress)
			FinishSmoothScrolling();
	}
#endif //ENABLE_SMOOTH_SCROLLING_FEATURE
}

void CSScrollBar::StartPaintingTimeMeasure()
{
	//needed for smooth scrolling (to auto-disable it for slow CPUs)
	m_smoothScrollingScreenPaintingStats.StartPaintingTimeMeasure();
}

void CSScrollBar::FinishPaintingTimeMeasure()
{
	//needed for smooth scrolling (to auto-disable it for slow CPUs)
	m_smoothScrollingScreenPaintingStats.FinishPaintingTimeMeasure();
}

bool CSScrollBar::IsUIPaintingTimeFastEnoughForAnimation() const
{
	return m_smoothScrollingScreenPaintingStats.IsOkToUseSmoothScrolling();
}

void CSScrollBar::StartSmoothScrolling(INT64 newScrollPos, int recommendedSmoothScrollingTime /*= SMOOTH_SCROLLING_RECOMMENDED_TIME*/)
{
	ASSERT(recommendedSmoothScrollingTime >= 0);

#ifdef ENABLE_SMOOTH_SCROLLING_FEATURE
	if (m_bEnableSmoothScrolling && 
		recommendedSmoothScrollingTime > 0 &&
		m_smoothScrollingScreenPaintingStats.IsOkToUseSmoothScrolling())
	{
		INT64	oldScrollPos = m_bSmoothScrollingInProgress ? GetScrollPos() : m_curScrollPos;

		if (!m_smoothScrollingTimerID)
			m_smoothScrollingTimerID = SetTimer(SMOOTH_SCROLLING_TIMER_ID, SMOOTH_SCROLLING_TIMER_INTERVAL, NULL);
		ASSERT(m_smoothScrollingTimerID);

		m_smoothScrollingStartTime = GetTickCount();
		m_smoothScrollingEndTime = m_smoothScrollingStartTime + recommendedSmoothScrollingTime;
		m_smoothScrollingStartScrollPos = oldScrollPos;
		m_smoothScrollingCurScrollPos = oldScrollPos;
		m_curScrollPos = newScrollPos;
		m_bSmoothScrollingDisplayScrollerAtFinalPos = m_bMouseBtnPressed && m_ActiveComponent == escScroller;
		m_bSmoothScrollingInProgress = true;
		ValidateNewPos();

		//OnTimer(m_smoothScrollingTimerID);
	} else
#endif //ENABLE_SMOOTH_SCROLLING_FEATURE
	{
		if (m_bSmoothScrollingInProgress)
		{
			//likely we we are on slow CPU, so need to finish previous smooth scrolling
			FinishSmoothScrolling(false);
		}
		ASSERT(!m_bSmoothScrollingInProgress);
		ASSERT(!m_smoothScrollingTimerID);
	}
}

void CSScrollBar::FinishSmoothScrolling(bool bNotifyParent /*= true*/)
{
	if (m_bSmoothScrollingInProgress)
	{
		ASSERT(m_smoothScrollingTimerID && m_bEnableSmoothScrolling);
		m_bSmoothScrollingInProgress = false;
		if (m_smoothScrollingTimerID)
			KillTimer(m_smoothScrollingTimerID);
		m_smoothScrollingTimerID = NULL;

		if (bNotifyParent)
		{
			RedrawWindow();
			SendNotfication();
		}
	} else
	{
		ASSERT(!m_smoothScrollingTimerID);
	}
}

void CSScrollBar::EnableScrollBar(BOOL bEnable)
{
	//note: use this method instead of EnableWindow
	m_bEnabled = bEnable;
	RedrawWindow();
}

BOOL CSScrollBar::IsScrollBarEnabled(bool bTakeInToAccountScrollPos /*= false*/) const
{
	if (bTakeInToAccountScrollPos)
		return m_bEnabled && !m_bDisabledDueToRange;
	else
		return m_bEnabled;
}

void CSScrollBar::OnEnable(BOOL bEnable)
{
	//Warning: in order to enable or disable window please use EnableScrollBar method instead of EnableWindow!!!
	CWnd::OnEnable(bEnable);

	if (!bEnable)
		ModifyStyle(WS_DISABLED, 0, 0); // this is needed in order to display proper cursor if scroll bar
										// is placed under other view which has non-standard cursor, 
										// as disabled window is transparent for cursor by Windows design
}

BOOL CSScrollBar::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if (!IsScrollBarEnabled(true))
		return FALSE;

	int		mouseScrollLines = GetMouseScrollLines();
	INT64	newScrollPos = m_curScrollPos;
	
	if (mouseScrollLines == -1)
	{
		int nPagesScrolled = zDelta / WHEEL_DELTA;
		
		newScrollPos -= nPagesScrolled * m_onePageSize;
	} else
	{
		int nRowsScrolled = (mouseScrollLines * zDelta) / WHEEL_DELTA;
		
		newScrollPos -= m_oneStepSize * nRowsScrolled;
	};
	
	if (newScrollPos < m_minScrollPos)
		newScrollPos = m_minScrollPos;

	if (newScrollPos + (m_mode == emLimitScrollingToMaxPosMinusLastPage ? m_onePageSize : 0) > m_maxScrollPos)
		newScrollPos = m_maxScrollPos - (m_mode == emLimitScrollingToMaxPosMinusLastPage ? (m_onePageSize + 1) : 0);
	
	if (newScrollPos != m_curScrollPos)
	{
		SetScrollPos(newScrollPos, SMOOTH_SCROLLING_RECOMMENDED_TIME * 2);

		RedrawWindow();
		SendNotfication();
	}
	
	return TRUE;
}

INT64 CSScrollBar::NormalizeScrollerPos(INT64 suggestedScrollerPos)
{
	ASSERT(m_mode == emLimitScrollingToMaxPos || m_maxScrollPos - m_minScrollPos + 1 > m_onePageSize);
	if (suggestedScrollerPos < m_minScrollPos)
		suggestedScrollerPos = m_minScrollPos;
	if (suggestedScrollerPos + (m_mode == emLimitScrollingToMaxPosMinusLastPage ? (m_onePageSize - 1) : 0) > m_maxScrollPos)
		suggestedScrollerPos = m_maxScrollPos - (m_mode == emLimitScrollingToMaxPosMinusLastPage ? (m_onePageSize - 1) : 0);

	return suggestedScrollerPos;
}

void CSScrollBar::SendNotfication()
{
	// Note: for we have to send this message to the window owner, 
	// not to the parent, because of in some places (such as CAEdit control scrollbar)
	// parent and owners ma be different, and only owners should get notification
	// message.
	//PlaySoundForEvent(SND_NAME_MOUSE_HOOVER, __FUNCTION__, SND_NO_INTERVAL_CHECK);
	GetOwner()->SendMessage(WM_SSCROLLBAR_NOTIFICATION, m_id);
}

void CSScrollBar::ProcessAction()
{
	if (!m_bMouseBtnPressed ||
		m_ActiveComponent == escNone ||
		m_ActiveComponent == escScroller)
		return;
	
	INT64 oldScrollPos = m_curScrollPos;
	INT64 newScrollPos = oldScrollPos;

	switch (m_ActiveComponent)
	{
	case escUpArrow:
		newScrollPos -= m_oneStepSize;
		break;

	case escDownArrow:
		newScrollPos += m_oneStepSize;
		break;

	case escUpperFreeSpace:
		newScrollPos -= m_onePageSize;
		break;

	case escBottomFreeSpace:
		newScrollPos += m_onePageSize;
		break;
	}
	
	newScrollPos = NormalizeScrollerPos(newScrollPos);
	if (newScrollPos != oldScrollPos)
	{
		SetScrollPos(newScrollPos, SCROLL_AUTO_REPEAT_DELAY_TIME);

		SendNotfication();
	}
}


//////////////////////////////////////////////////
//
//	struct CSScrollBar::SPaintingStats
//

CSScrollBar::SPaintingStats::SPaintingStats() :
	m_bOkToUseSmoothScrolling(true)
{
}

bool CSScrollBar::SPaintingStats::IsOkToUseSmoothScrolling() const
{
	return m_bOkToUseSmoothScrolling;
}

void CSScrollBar::SPaintingStats::StartPaintingTimeMeasure()
{
	m_curPaintingStartTime = GetTickCount();
}

void CSScrollBar::SPaintingStats::FinishPaintingTimeMeasure()
{
	AddPaintingTime(GetTickCount() - m_curPaintingStartTime);
}

void CSScrollBar::SPaintingStats::AddPaintingTime(DWORD paintingTimeMilliseconds)
{
	ASSERT(paintingTimeMilliseconds >= 0);
	m_recentPaintingTimings.Add(paintingTimeMilliseconds);
	if (m_recentPaintingTimings.GetSize() > MAX_ENTRIES_COUNT_TO_KEEP_STATS_FOR)
		m_recentPaintingTimings.RemoveAt(0); //remove old entries
	ASSERT(m_recentPaintingTimings.GetSize() <= MAX_ENTRIES_COUNT_TO_KEEP_STATS_FOR);

	//let's update the m_bOkToUseSmoothScrolling variable
	DWORD	maxPaintTimeMilliseconds = 0;
	DWORD	averagePaintTimeMilliseconds = 0;
	int		entriesCount = (int)m_recentPaintingTimings.GetSize();

	for (int entryN = 0; entryN < entriesCount; entryN ++)
	{
		DWORD	curEntryPaintingTime = m_recentPaintingTimings[entryN];

		maxPaintTimeMilliseconds = max(maxPaintTimeMilliseconds, curEntryPaintingTime);
		averagePaintTimeMilliseconds += curEntryPaintingTime;
	}
	ASSERT(entriesCount > 0); //avoid div by 0
	averagePaintTimeMilliseconds /= entriesCount;
	ASSERT(m_recentPaintingTimings.GetSize() <= MAX_ENTRIES_COUNT_TO_KEEP_STATS_FOR);
	
	m_bOkToUseSmoothScrolling = maxPaintTimeMilliseconds <= MAX_PAINTING_TIME_MS_TO_ALLOW_SMOOTH_SCROLLING && averagePaintTimeMilliseconds < MAX_PAINTING_TIME_MS_TO_ALLOW_SMOOTH_SCROLLING;
}
