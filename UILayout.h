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

#include "Memory.h"

class CMenuBar;
class CBaseWnd;
class CAdvPtrList;


enum eSizeType
{
	sizeFull, //occupy all available space, in this case CUIObjInfo::m_width and CUIObjInfo::m_height are expressing minimal size in pixels
	sizeRecomended, //widget may recomend its size depending on its type
	sizeCustomInPixels,
	sizeCustomInPercents //width in percents //to-do: width is percents is not everywhere respected properly (see CUIObjInfo::GetObjRecommendedSize for example, how m_width is handled there)
};


enum eObjAlignment
{
	alignTop,
	alignBottom,
	alignLeft,

	alignRight
};


class CUIObjInfo : public CObject
{
public:

	//Methods:
					CUIObjInfo();
					CUIObjInfo(CObject* pObj, eObjAlignment alignment, bool bFocusable = true, eSizeType widthType = sizeRecomended, eSizeType heightType = sizeRecomended, double width = 0, double height = 0, bool bAllowSpacingToNextObj = true);
					CUIObjInfo(const CUIObjInfo& src) { *this = src; }

	CSize			GetDesiredSize(CSize parentLayoutSize) const; //return minimal size in pixels
	eObjAlignment	GetAlignment() const { return m_alignment; }
	bool			IsSizeDependOnLayoutSize() const;
	bool			IsAllowSpacingToNextObj() const { return m_bAllowSpacingToNextObj; }

	void			SetWidth(double width) { m_width = width; }
	double			GetWidth() const { return m_width; }
	void			SetWidthType(eSizeType widthType) { m_widthType = widthType; }
	eSizeType		GetWidthType() { return m_widthType; }
	void			SetHeight(double height) { m_height = height; }
	double			GetHeight() const { return m_height; }
	void			SetHeightType(eSizeType heightType) { m_heightType = heightType; }
	eSizeType		GetHeightType() { return m_heightType; }

	void			SetObjPlacement(const CRect& rect, BOOL bRepaint = TRUE);
	CRect			GetObjPlacement();

	CObject*		GetObject() const { return m_pObject; } //may return NULL if object is "separator"
	CWnd*			GetWndObject() const; //may return NULL if object is non-window class
	bool			IsCWndCompatible() const;
	bool			IsLayoutObject() const;

	bool			CanGetFocus() const;
	void			SetFocus();

	BOOL			IsVisible();
	void			SetVisible(BOOL bVisible, bool bRecursively = true);

	bool			IsManualPlacementMode() const { return m_bManualPlacementObject; }
	void			SetManualPlacementMode(bool bManualPlacement);

	CUIObjInfo&		operator=(const CUIObjInfo& src);

protected:
	
	//Methods:
	int				GetObjSizeFromItsType(eSizeType type, double size, int layoutSize, bool bWidth) const;
	CSize			GetObjRecommendedSize(CSize parentLayoutSize) const;

protected:
	
	//Data:
	CObject*		m_pObject; //may be NULL if object is used as a "separator"
	bool			m_bFocusable;
	BOOL			m_bVisible; //used only if m_pObject is not derived from CWnd
	bool			m_bManualPlacementObject; //for this kind of obj no automatic coordinates computation will be used
	bool			m_bAllowSpacingToNextObj;

	eObjAlignment	m_alignment;

	eSizeType		m_widthType;
	double			m_width; //depends on m_widthType value, may be skipped at all in certain cases
	
	eSizeType		m_heightType;
	double			m_height; //depends on m_heightType value, may be skipped at all in certain cases

	CRect			m_placementRect; //remembers last obj placement
};


class CUILayoutBase : public CObject
{
public:

	//Methods:
							CUILayoutBase(bool bReverseRTLlayoutIfNeeded = true);
	virtual					~CUILayoutBase();

	void					SetDistances(int distBetweenObjects, int distFromCorner, bool bSkipInvisibleObjects = false);
	void					SetDistancesEx(int distBetweenObjects, int distFromTopCorner, int distFromBottomCorner, int distFromLeftCorner, int distFromRightCorner, bool bSkipInvisibleObjects = false);

	void					SetParent(CUILayoutBase* pParentLayout) { ASSERT(!m_pParentLayout); m_pParentLayout = pParentLayout; }
	CUILayoutBase*			GetParent() const { return m_pParentLayout; } //return NULL for a top-most layout

	virtual CSize			GetDesiredSize(CSize initialLayoutSize) = 0;
	virtual void			SetPlacement(const CRect& layoutRect);
	CRect					GetClientRect() const { return m_layoutRect; }

	void					SetUIObjectWidth(CObject* pObj, double width);
	void					SetUIObjectWidthType(CObject* pObj, eSizeType widthType);
	void					SetUIObjectHeight(CObject* pObj, double height);
	void					SetUIObjectHeightType(CObject* pObj, eSizeType heightType);
	double					GetUIObjectWidth(CObject* pObj);
	double					GetUIObjectHeight(CObject* pObj);
	CRect					GetObjPlacement(CObject* pObj);

	virtual void			AddUIObject(CUIObjInfo& objInfo);
	virtual void			AddUISeparatorObject(eObjAlignment alignment);
	virtual void			RemoveUIObject(CObject* pObj);
	virtual void			RemoveAllUISeparators();
	virtual void			RemoveAllUIObjects();

	bool					SetFocus(bool bSetToFirstCtlIfNeeded); //set focust to the last focused control in the layout
	CWnd*					GetFocus() const; //return CWnd* of focused window if it belongs to the layout

	void					ShowAllUIControls(BOOL bShow, bool bRecursively = true);
	bool					IsVisible(); //return true if at least one control in the layout is visible

	void					InitTopmostLayout(CWnd* pParent);
	bool					IsTopMostLayout() const { return m_pParentWnd != NULL; }

	virtual CMenuBar*		GetMenuBar() const { return NULL; }

	bool					IsControlBelongsToTheGroup(CObject* pUICtl);
	static CUILayoutBase*	GetLayoutFromCtlPtr(CObject** ppUICtl, bool* pbCtlISParentWhd = NULL);
	CUIObjInfo*				GetAssociatedObjInfo(CObject* pObj);

	//Extrenally accesible event handler
	static BOOL				GlobalPreTranslateMessage(MSG* pMsg);

	DECLARE_DYNAMIC(CUILayoutBase);

protected:

	//Methods:
	int						GetObjIndex(CObject* pUICtl) const; //return -1 if not found
	
	bool					PassFocusToNextCtl(CObject* pObj, bool bGoBackward = false, bool bCycleWithinLayout = false);
	bool					PassFocusToFirstLastCtl(bool bToTheLast);
	void					FocusChangeNotify(CObject* pFocusedObject);

	CWnd* 					GetFirstFocusableCtrl() const;
	CWnd* 					GetLastFocusableCtrl() const;
	CWnd* 					GetNextFocusableCtrl(CObject* pObj, bool bAllowCycling = false) const;
	CWnd* 					GetPrevFocusableCtrl(CObject* pObj, bool bAllowCycling = false) const;

	static BOOL				IsFocusableWnd(CObject* pUICtl);
	static LRESULT CALLBACK	HookCallWndProc(int nCode, WPARAM wParam, LPARAM lParam);
	
protected:

	//Data:
	CRect					m_layoutRect;
	int						m_distBetweenObjects;
	int						m_distFromTopCorner;
	int						m_distFromBottomCorner;
	int						m_distFromLeftCorner;
	int						m_distFromRightCorner;
	bool					m_bSkipInvisibleObjects;
	bool					m_bReverseRTLlayoutIfNeeded; //support right-to-left language layouts, like Hebrew and Arabic

	CUILayoutBase*			m_pParentLayout; //NULL if it is a top-most layout
	CWnd*					m_pParentWnd; //used only for top-most windows

	CObject*				m_pLastFocusedCtl;

	CArray<CUIObjInfo, CUIObjInfo&>	m_objects;

	//
	static CAdvPtrList		m_gInstances;
};


class CUILayout : public CUILayoutBase
{
public:

	//Methods:
							CUILayout(bool bMultiPageMode = false, bool bReverseRTLlayoutIfNeeded = true);
	virtual					~CUILayout();

	void					AddMenuBar(CMenuBar* pMenuBar);

	void					SetMultiPagePlacement(const CRect& layoutRect);

	virtual CSize			GetDesiredSize(CSize initialLayoutSize);
	virtual void			SetPlacement(const CRect& layoutRect);

	virtual CMenuBar*		GetMenuBar() const;
	DECLARE_DYNAMIC(CUILayout);

protected:
	
	//Methods:
	void					GetFullSizeObjFollowingObjsSize(bool bCalcHeight, bool bCalcWidth, int objN, 
															CSize curObjSize, CRect curLayoutRect, 
															int* pFollowingObjsHeight, int* pFollowingObjsWidth);

protected:
	
	//Data:
	bool					m_bMultiPageMode; //used to put more then one page in a single layout on a same place, useful for having tabbed dialogs

	CMenuBar*				m_pMenuBar; //Used only for focusing. Each window has not more then one menu bar
};

#define CREATE_LAYOUT(_pLayout_)		\
{										\
	ASSERT(!_pLayout_);					\
	_pLayout_ = new CUILayout();		\
	m_destructor.Add(_pLayout_);		\
}

#define CREATE_MULTIPAGE_LAYOUT(_pLayout_)	\
{											\
	ASSERT(!_pLayout_);						\
	_pLayout_ = new CUILayout(true);		\
	m_destructor.Add(_pLayout_);			\
}


class CUIGridLayout : public CUILayoutBase
{
public:

	//Data types:
	struct SCellInfo
	{
		// methods:
					SCellInfo() : m_pUICtl(NULL) {;}

		// data:
		CObject*	m_pUICtl;
	};
	typedef CArray<SCellInfo, SCellInfo&>	CCellsArray;

public:

	//Methods:
					CUIGridLayout(int x, int y, bool bReverseRTLlayoutIfNeeded = true);
	virtual			~CUIGridLayout();

	void			SetCellUIObject(int x, int y, CUIObjInfo& objInfo);
	virtual void	AddUIObject(CUIObjInfo& objInfo) { ASSERT(false); } //call SetCellUIObject method instead
	virtual void	AddUISeparatorObject(eObjAlignment alignment) { ASSERT(false); } //not supported method
	virtual void	RemoveUIObject(CObject* pObj);
	virtual void	RemoveAllUISeparators();
	virtual void	RemoveAllUIObjects();

	void			SetFullWidthColumn(int x); //this column will be used to fill all space that can be occupied
	void			SetFullHeightRow(int y); //this row will be used to fill all space that can be occupied

	virtual CSize	GetDesiredSize(CSize initialLayoutSize);
	virtual void	SetPlacement(const CRect& layoutRect);

	DECLARE_DYNAMIC(CUIGridLayout);

protected:

	//Data types:
	class CDimensionsCache
	{
	public:

		// methods:
					CDimensionsCache(CUIGridLayout* pParent);

		void		ClearCache();

		CSize		GetMinimalObjSize(int x, int y);

	protected:

		// data:
		CUIGridLayout*			m_pParent;
		CArray<CSize, CSize&>	m_sizes; //if cx value of size is equal to -1 then it means that it is not initialized
	};

protected:

	//Methods:
	int				GetCellIndex(int x, int y) const;
	int				GetObjCellIndex(CObject* pObj) const; //function may return -1 if not found

	int				GetMinColumnWidth(int x, CDimensionsCache* pCache);
	int				GetMinRowHeight(int y, CDimensionsCache* pCache);

protected:

	//Data:
	CCellsArray		m_cells;
	int				m_xSize;
	int				m_ySize;
	int				m_fullWidthColN;
	int				m_fullHeightRowN;

	//
	// friends:
	friend class CDimensionsCache;
};

#define CREATE_GRID_LAYOUT(_pLayout_, _x_, _y_)	\
{												\
	ASSERT(!_pLayout_);							\
	_pLayout_ = new CUIGridLayout(_x_, _y_);	\
	m_destructor.Add(_pLayout_);				\
}
