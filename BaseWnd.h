//	This is a part of the Cyberium codename project. Part of AllMyNotes Organizer source.
//	Copyright (c) 2000-2024 Volodymyr Frytskyy. All rights reserved.
//
//	THIS SOURCE FILE IS THE PROPERTY OF VOLODYMYR FRYTSKYY AND IS NOT TO BE
//	RE-DISTRIBUTED OR USED BY ANY MEANS WHATSOEVER WITHOUT THE EXPRESSED WRITTEN
//	CONSENT OF VOLODYMYR FRYTSKYY.
//
//	CONTACT INFORMATION: https://www.vladonai.com/about-resume, www.vladonai.com, www.allmynotes.org
//	License: https://www.vladonai.com/allmynotes-organizer-license-agreement

#pragma once


class CSkinBase;
class CUILayout;
class CStatusBarInfo;
class CWndShadowEffect;
enum eWndTitleIcon;
enum eWndCaptionElement;

enum eWndSnapMode
{
	snapNone,
	//
	snapMaximize,
	snapLeft,
	snapRight,
};


/////////////////////////////////////////////////////////////////////////////
//
//	CBaseWnd window
//

class CBaseWnd : public CWnd
{
public:
	
	//Methods:
						CBaseWnd(eWndTitleIcon wndIcon, bool bShowMinMaxButtons = true, bool bShowStatusbar = false);
	virtual				~CBaseWnd();
	
	CRect				GetClientRect() const;
	void				GetClientRect(LPRECT lpRect) const;
	
	virtual void		Init();
	virtual void		Show(CWnd** ppParentalDlgPointer = NULL, bool bLoadPlacement = true); //show non-modal dialog
	virtual int			DoModal(bool bLoadPlacement = true); //show modal dialog
	BOOL				IsModalDialog() const;
	
	bool				IsWindowCanBeSnapped() const;
	eWndSnapMode		IsWindowSnapped() const;
	void				SnappWindowToLeft();
	void				SnappWindowToRight();
	void				SnappWindowMax();
	void				SnappWindowMinimize();

	void				SetWndIcon(eWndTitleIcon wndIcon, bool bInvalidate = true);
	bool				SetMinPossibleClientSize(CSize size, bool bForceResize, bool bForceSpecifiedHeightOnly = false, bool bForceSpecifiedWidthOnly = false);

	void				SetDefaultControl(CWnd* pCtl);
	CWnd*				GetDefaultControl() const { return m_pDefaultControl; }
	
	CStatusBarInfo*		GetStatusBar() { return m_pStatusBarInfo; } //may return NULL if window has no status bar
	CSkinBase*			GetSkin() { ASSERT(m_pSkin); return m_pSkin; } //return used skin

	DWORD				GetTicksCountSinceWndGotActive() const;

	static bool			IsOsSupportsWindowShadow();
	LONG				GetWindowShadowStyle();
	static LONG			GetCtrlShadowStyle(); //used for menus, tooltips, etc.

	static void			OnSkinReloadedNotification();
	static void			OnLanguageReloadedNotification();
	
	DECLARE_DYNAMIC(CBaseWnd);
	
protected:

	//Methods:
	void				DrawHeaderAndBorders(CADC& dc);
	virtual void		Redraw(CADC& dc); //called to redraw window client area by inherited classes
	virtual void		RearrangeObjects(bool bReloadSkinInfo) = 0;
	virtual bool		CanCloseWindow() { return true; }
	virtual bool		IsWindowCanBeClosedByEnterkey() { return IsModalDialog() || m_bNonModalDialogBox; }
	
	virtual BOOL		PreTranslateMessage(MSG* pMsg);

	CPoint				GetTrackWindowSize() const;
	void				SetNCCursor(UINT hitTest);
	static void			UpdateAllWindows(bool bReloadTexts, bool bReloadSkinInfo);
	bool				IsResizabe() const { return m_bShowMinMaxButtons; }
	void				UpdateShadow();
	
	bool				LoadWindowPlacement(bool bPreventMinimizedInitialPos);
	void				SaveWindowPlacement() const;
	void				PlaceWndOnParentCenter();
	void				SetInternalWindowName(CString internalName) { ASSERT(!internalName.IsEmpty()); m_explicitInternalWndName = internalName; }

	//Message map functions:
	int					OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual void		OnClose();
	void				OnPaint();
	LRESULT				OnNcHitTest(CPoint point);
	void				OnNcRButtonUp(UINT nHitTest, CPoint point);
	void				OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	void				OnSetFocus(CWnd* pOldWnd);
	void				OnSize(UINT nType, int x, int y);
	void				OnWindowPosChanged(WINDOWPOS* lpwndpos);
	void				OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	int					OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	LRESULT				OnUpdateWndPos(WPARAM, LPARAM);
	LRESULT				OnDestroyInternal(WPARAM, LPARAM);
	virtual BOOL		ProcessControlKeyDown(UINT nChar, UINT nAdvKeyInfo);
	//
	void				OnMax();
	void				OnMin();
	virtual void		OnCloseWnd(UINT nResultCode = IDCANCEL);
	//
	virtual void		OnOK();
	virtual void		OnCancel();
	DECLARE_MESSAGE_MAP();
	
protected:
	
	//Data:
	CSkinBase*				m_pSkin;
	CObjectsDestructor		m_destructor;
	CUILayout*				m_pWndLayout;
	CWnd**					m_ppParentalDlgPointer; //this pointer is used when window is non-modal dialog box, it points to this dialog box instance which is stored in the parent window instance in order to set it to NULL when this window will be closed
	CStatusBarInfo*			m_pStatusBarInfo; //different from NULL if window has status bar

	bool					m_bShowShadow;
	//CWndShadowEffect*		m_pShadowEffect;

	bool					m_bSaveWndPlacementOnClose;
	CString					m_explicitInternalWndName; //internal window name, used for saving/loading window placement. If it is not specified then used window title string

	UINT_PTR				m_checkMousePosTimerID;
	
	static DWORD			m_gLastWndGotActiveTimeTick;

	DECLARE_DOUBLE_BUFFERING();
	
private:
	
	//Data types:
	struct SNCInfo
	{
		//Methods:
							SNCInfo();
		bool				IsUsed();

		//Data used to track clicks on Min/Max/Close buttons:
		eWndCaptionElement	m_pressedCaptionElement; //ewceNone if none
		bool				m_bMouseIsOverPressedCaptionElement; //used in pair with m_pressedCaptionElement

		//Data used to track mouse moving over Min/Max/Close buttons:
		eWndCaptionElement	m_highlightedCaptionElement; //ewceNone if none

		//Data used to track window resizing:
		UINT				m_pressedBorderElement; //HTNOWHERE if nothing
		CPoint				m_lastMousePos; //used for moving window borders, only when m_pressedBorderElement != HTNOWHERE

		eWndSnapMode		m_snapMode;
		CRect				m_originalWndRect; //window rect before resizing
		CRect				m_newWndRect;

		bool				m_bNeedToMoveWnd;
	};

private:

	//Data:
	bool					m_bInitialized;
	bool					m_bShowMinMaxButtons;
	bool					m_bNonModalDialogBox;
	
	eWndTitleIcon			m_icon;
	
	CRect					m_clientRect;
	CRect					m_captionRect;
	CRect					m_normalWndRectBeforeSnapping; //used to properly support snapping (like in Win7 Aero Snap, to support it correctly aslo on all other systems)
	CSize					m_minPossibleClientSize;
		
	CWnd*					m_pDefaultControl;

	SNCInfo*				m_pNCInfo;
	UINT					m_oldWndSizeType;

	static CWndPtrsArray	m_allBaseWnds;


	/////////////////////
	//	Friends:
	friend class CBaseDialog;
};
