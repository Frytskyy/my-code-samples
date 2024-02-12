//	This is a part of the Cyberium codename project. Part of AllMyNotes Organizer source.
//	Copyright (c) 2000-2022 Volodymyr Frytskyy (owner of Vladonai Software). All rights reserved.
//
//	THIS SOURCE FILE IS THE PROPERTY OF VOLODYMYR FRYTSKYY AND IS NOT TO BE
//	RE-DISTRIBUTED OR USED BY ANY MEANS WHATSOEVER WITHOUT THE EXPRESSED WRITTEN
//	CONSENT OF VOLODYMYR FRYTSKYY.
//
//	CONTACT INFORMATION: https://www.vladonai.com/about-resume, www.vladonai.com, www.allmynotes.org
//	License: https://www.vladonai.com/allmynotes-organizer-license-agreement

#pragma once

//#include "AppVersionInfo.h"
#include "Strings.h"



/* _val=target variable, _bit_n=bit number to act upon 0-n */
#define BIT_SET_NEW_VALUE(_val, _bit_n, _new_bit_val) ((_val) = ((_val) & ~(1 << (_bit_n))) | ((_new_bit_val) << (_bit_n)))
#define BIT_SET(_val, _bit_n) ((_val) |= (1 << (_bit_n)))
#define BIT_CLEAR(_val, _bit_n) ((_val) &= ~(1 << (_bit_n)))
#define BIT_FLIP(_val, _bit_n) ((_val) ^= (1 << (_bit_n)))
#define BIT_CHECK(_val, _bit_n) ((_val) & (1 << (_bit_n)))


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	class CArrayEx - a bit enhanced version of CArray
//
;
template <class TYPE, class ARG_TYPE = const TYPE&>
class CArrayEx: public CArray <TYPE, ARG_TYPE>
{
public:

	//Methods:
	//		CArrayEx();
	//virtual ~CArrayEx();

	int			FindElementIndex(const TYPE& element); //return -1 if element is not found

	CArrayEx&	operator=(const CArrayEx& srcArray) { if (this != &srcArray) { Copy(srcArray); }; return *this; }
};

template <class TYPE, class ARG_TYPE> 
AFX_INLINE int CArrayEx<TYPE, ARG_TYPE>::FindElementIndex(const TYPE& element)
{
	//return -1 if element is not found
	for (int arrayElemenetN = 0; arrayElemenetN < m_nSize; arrayElemenetN ++)
		if (element == *(m_pData + arrayElemenetN))
			return arrayElemenetN; //found it!

	return -1; //not found
}


class CUILayout;
union CColor;
class CADC;
class CSToolTips;
struct SPOPThreadInfo;
class CItem;
class CNoteBase;
class CFolderBase;
class CDBNote;
class CDBFolder;
class CHugeMemory;

#include "CybDefs.h"


//universal pointer
union Ptr
{
	//Methods:
						Ptr() : pVoid(NULL) {}; //constructor
						Ptr(void* ptr) : pVoid(ptr) {}; //constructor

						operator void*()	{ return pVoid; };

	void				Inc(UInt32 val)		{ value32 += val; };
	void				Dec(UInt32 val)		{ value32 -= val; };
	void				Set(void* ptr)		{ pVoid = ptr; };
	void				Null()				{ pVoid = NULL; };
	
	//Data:
	void*				pVoid;
	CWnd*				pWnd;
	char*				pChar;
	TCHAR*				pTCHAR;
	CHAR*				pCHAR;
	WCHAR*				pWCHAR;
	byte*				pByte;
	UINT64*				pUINT64;
	WORD*				pWORD;
	DWORD*				pDWORD;
	DATE*				pDATE;
	CColor*				pColor;
	COLORREF*			pCOLORREF;
	Ptr*				pPtr;
	CUILayout*			pLayout;
	CObject*			pObject;
	Int32*				pInt32;
	Int32				value32;
	UInt32				uvalue32;
	LPUNKNOWN			pUnknown;
	LPDISPATCH			pDispatch;
	CSToolTips*			pSToolTip;
	SPOPThreadInfo*		pPOPThreadInfo;
	CItem*				pItem;
	CNoteBase*			pNoteBase;
	CFolderBase*		pFolderBase;
	CDBNote*			pDBNote;
	CDBFolder*			pDBFolder;
	DWORD				asDWORD;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	class CMemory used to make work with memory allocations more secure - it frees allocated memory automatically on destruction
//


class CMemory : public CObject
{
public:
	
	// methods:
				CMemory(const CMemory& srcArray);
				CMemory(size_t size = 0, bool bSetWithZeors = false);
	virtual		~CMemory();

	Ptr			GetPtr() const { return m_pMem; }; //WARNING: be careful with returned pointer, do not resize it, do not delete it!!
	size_t		GetSize() const;

	Ptr			Realloc(size_t size, bool bSetWithZeors = false); //WARNING: be careful with returned pointer, do not resize it, do not delete it!!
	void		CreateFromMemoryPtr(void* pSrcMemoryBlock);
	void		Free();

	void		Attach(void* pMemoryBuf, size_t size);
	Ptr			Detach();

	CString		DetectFileExtensionByBinaryFileContent();

	CMemory&	operator=(const CMemory& srcArray);
	CMemory&	operator=(CHugeMemory& srcArray);

protected:

	// data:
	Ptr			m_pMem;
	size_t		m_size;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	class CHugeMemory used to store huge non-consecutive memory blocks. For example, to store 500Mb memory block system may not find 
//	consecutive memory block, therefore it's more secure to use many smaller memory blocks instead.
//

typedef CArray<CMemory,	CMemory&>	CMemoryBlocksArray;


class CHugeMemory : public CObject
{
public:

	// methods:
						CHugeMemory(const CHugeMemory& src);
						CHugeMemory();
	virtual				~CHugeMemory();

	ULONGLONG			GetSize() const;
	static size_t		GetMaxBlockSize();

	bool				Alloc(ULONGLONG size);
	void				Free();
	
	CMemoryBlocksArray&	GetMemoryBlocks() { return m_memoryBlocks; } //WARNING: DO NOT RESIZE THESE BLOCKS, USE RETURNED ARRAY FOR READING-WRITING DATA CONTENT
	bool				AddMemoryBlock(const void* pBuf, int nBufSize);

	CString				DetectFileExtensionByBinaryFileContent();
	CString				GetMD5() const;

	bool				WriteToFile(const CString& filePath) const;
	bool				WriteToFile(CFile* pFile) const;

	CHugeMemory&		operator=(const CMemory& src);
	CHugeMemory&		operator=(const CHugeMemory& src);
	BOOL				operator==(const CHugeMemory& otherObj) const;
	BOOL				operator!=(const CHugeMemory& otherObj) const { return !operator==(otherObj); };

protected:

	// data:
	CMemoryBlocksArray	m_memoryBlocks;
};


class CHugeObjsKeeper //see also: CObjectsKeeper
{
public:

	// methods:
						CHugeObjsKeeper();
	virtual				~CHugeObjsKeeper();

	UINT64				CreateNewObj(ULONGLONG size); //return new object ID
	UINT64				AttachObject(CHugeMemory* pHugeMemObj); //return new object ID, note: after this function call the pointer deletion is fully managed by this class
	CHugeMemory*		GetObjectPtrByID(UINT64 objID);
	UINT64				GetObjID(ULONGLONG size, CString objMD5, bool bIncrRefCounter); //return 0 if not found
	void				AddAnotherObjectReferrence(UINT64 objectID);
	void				RemoveObjectReferrence(UINT64 objID); //delete the object if there is no other refferrence to it

	int					GetObjectsCount();

protected:

	// methods:
	int					GetObjectIndexByID(UINT64 objID, bool bForCreation); //return -1 if not found

protected:

	// data types:
	struct SHugeBlockRefInfo
	{
						SHugeBlockRefInfo() : m_refCounter(0), m_objectID(0), m_pHugeMem(NULL) {};

		CHugeMemory*	m_pHugeMem;
		UINT			m_refCounter;
		UINT64			m_objectID;
	};
	typedef CArray<SHugeBlockRefInfo, SHugeBlockRefInfo&> SHugeBlockRefsArray;

	// data:
	SHugeBlockRefsArray	m_memBlocks;
	UINT64				m_objectIDsCounter;

	CCriticalSection	m_criticalSection; //for muti-thread access
};

extern CHugeObjsKeeper	g_huge_mem_objects_keeper; //see also: g_big_objects_keeper


class CObjectsKeeper //see also: CHugeMemoryCollector
{
public:

	//Methods
						CObjectsKeeper();
	virtual				~CObjectsKeeper();

	UINT64				AttachObject(CObject* pObject); //return object ID, since this moment this class is responsible for object destruction
	UINT64				GetObjectIDByPtr(CObject* pObject); //return object ID, or 0 if object doesn't exists
	CObject*			GetObjectPtrByID(UINT64 objectID);
	void				AddAnotherObjectReferrence(UINT64 objectID);
	void				RemoveObjectReferrence(UINT64 objectID);

	int					GetObjectsCount();

protected:

	//Methods:
	int					GetObjectIndexByID(UINT64 objectID, bool bForCreation); //return -1 if object not found in m_objectIDsCounter

protected:

	//Data types:
	struct SSingleObjInfo
	{
		CObject*		m_pObject;
		UINT64			m_objectID;
		UINT			m_refCounter; //when it gets to 0 then delete the object as it's not used anymore
	};
	typedef CArray<SSingleObjInfo,	SSingleObjInfo&>	CObjectsInfoArray;

protected:

	//Data
	UINT64				m_objectIDsCounter; //ID start from 1, while 0 is invalid value, we use it to identify that ID not found
	CObjectsInfoArray	m_objects;

	CCriticalSection	m_criticalSection; //for muti-thread access
};

extern CObjectsKeeper	g_big_objects_keeper;


class CAdvPtrList : public CObject //advanced pointers list
{
public:

	//Data types:
	struct sNode
	{
		sNode*			pNext; //if == NULL then it's the last node
		sNode*			pPrev; //if == NULL then it's the first node
		Ptr				ptr;
	};
	typedef sNode* LPNODE;

public:
	
			CAdvPtrList();
	virtual ~CAdvPtrList();
	
	void	Add(Ptr newPtr);
	void	Insert(int index, Ptr ptr);
	bool	Remove(Ptr ptr);
	Ptr		Remove(int index);
	Ptr		Remove(LPNODE pNode);
	Ptr		RemoveFirst() { return Remove(m_pFirstNode); }
	int		GetCount() const { return m_NodesCount; }; //return stored pointers count
	
	Ptr		GetFirst() { return(m_pFirstNode ? m_pFirstNode->ptr : NULL); }
	Ptr		GetNext(LPNODE& pNode); // set pNode to the next position and returns value from passed initially
	Ptr		GetPrev(LPNODE& pNode);// set pNode to the previous position and returns value from passed initially
	
	Ptr		Get(LPNODE pNode) const { ASSERT(pNode); return ((sNode*)pNode)->ptr; }
	Ptr		Get(int index);
	void	Set(int index, Ptr value);
	void	Set(LPNODE pNode, Ptr value) const { ASSERT(pNode); ((sNode*)pNode)->ptr = value; }

	LPNODE	Find(Ptr value) const;
	
	void	Clear();
	
protected:
	
	//Methods:
						CAdvPtrList(CAdvPtrList&)		{ ASSERT(false); };	// no copy constructor
	void				operator=(const CAdvPtrList&)	{ ASSERT(false); }; // no operator =
	
	sNode*				GetNodeByIndex(int index);

	Ptr					RawRemove(LPNODE pNode);

protected:

	//Data:
	int					m_NodesCount;
	sNode*				m_pFirstNode;

	int					m_LastAccesedIndex;
	sNode*				m_pLastAccesedNode;
};

typedef CAdvPtrList::LPNODE LPNODE;


class CAdvPtrsArray : public CObject
{
public:

	//Methods:
						CAdvPtrsArray();
	virtual				~CAdvPtrsArray();

	void				AddPtr(Ptr pItem);
	void				SetPtr(int index, Ptr pItem);
	Ptr				 	GetPtr(int index);
	void				RemovePtr(int index);
	bool				RemovePtr(Ptr ptr); //return true if pointer was found
	int					FindPtrIndex(Ptr ptr); //may return -1 if nothing found
	int					GetCount() const { return m_ItemsCount; }
	void				Clear();

	void				InvertItemsOrder();

	CAdvPtrsArray&		operator=(const CAdvPtrsArray& anotherContainer);

protected:

	//Data
	int					m_ItemsCount;
	int					m_AllocatedCount;
	CMemory				m_ptrsArray; //an array of Ptr instances
};


class CAMemFile : public CMemFile
{
public:

	//Methods
						CAMemFile(UINT nGrowBytes = 1024) : CMemFile(nGrowBytes) {}
						CAMemFile(BYTE* lpBuffer, UINT nBufferSize, UINT nGrowBytes = 0) : CMemFile(lpBuffer, nBufferSize, nGrowBytes) {}

	//operations
/*	UINT				Read(const ULONG* pul)			{ return Read(pul, sizeof(*pul));		}
	UINT				Read(const LONG* pl)			{ return Read(pl, sizeof(*pl));			}
	UINT				Read(const USHORT* pus)			{ return Read(pus, sizeof(*pus));		}
	UINT				Read(const SHORT* ps)			{ return Read(pus, sizeof(*ps));		}

	void				Write(ULONG ul)					{ Write(&ul, sizeof(ul));				} 
	void				Write(LONG l)					{ Write(&l, sizeof(l));					}
	void				Write(USHORT us)				{ Write(&us, sizeof(us));				}
	void				Write(SHORT s)					{ Write(&s, sizeof(s));					}*/

	//operators
	CFile&				operator << (ULONG ul)			{ Write(&ul, sizeof(ul)); return *this;	}
	CFile&				operator << (LONG l)			{ Write(&l, sizeof(l));	return *this;	}
	CFile&				operator << (int i)				{ Write(&i, sizeof(i));	return *this;	}
	CFile&				operator << (USHORT us)			{ Write(&us, sizeof(us)); return *this;	}
	CFile&				operator << (SHORT s)			{ Write(&s, sizeof(s));	return *this;	}
	CFile&				operator << (double d)			{ Write(&d, sizeof(d));	return *this;	}
	CFile&				operator >> (ULONG& ul)			{ Read(&ul, sizeof(ul)); return *this;	}
	CFile&				operator >> (LONG& l)			{ Read(&l, sizeof(l));	return *this;	}
	CFile&				operator >> (USHORT& us)		{ Read(&us, sizeof(us)); return *this;	}
	CFile&				operator >> (SHORT& s)			{ Read(&s, sizeof(s));	return *this;	}
	CFile&				operator >> (double& d)			{ Read(&d, sizeof(d));	return *this;	}

	//attributes
	LPBYTE				GetData() const					{ return m_lpBuffer;					}
	SIZE_T				GetGrowByValue() const			{ return m_nGrowBytes;					}
	void				Attach(BYTE* lpBuffer, UINT nBufferSize, UINT nGrowBytes); //note: original function CMemFile::Attach has a serious issue, it simply doesn't works as of 4/2018, as it sets zero file size, not matter what params where provided... (VMF)
};


//bsearch equivalent
class CBinarySearch
{
public:

	//Methods:
			CBinarySearch(int entriesCount);

	bool	GetElementIndexToTry(int* pElementIndexToTry);
	void	NextStep(bool bGoDown);

protected:

	//Data:
	int		m_entriesCount; //input array size
	int		m_minIndex;
	int		m_maxIndex;
	int		m_curIndex;
	int		m_oldIndex;
};



struct SExternalFileMonitoringInfo
{
	CString			m_filePathToMonitor;
	CString			m_originalFileName;
	COleDateTime	m_fileModDateTime; //if changed - the file got modified outside!
	
	CString			m_ddbDataFileName; //.ddb database file to which this file attachment was saved
	CString			m_treeItemIDhexStr; //tree item ID in which  this file attachment was saved
	int				m_attachemntObjectLineInDoc;
	int				m_attachemntObjectPosInLine; //used in conjunction with m_attachemntObjectLineInDoc
	CString			m_attachmentOldFileMD5; //used when object cannot be found using m_attachemntObjectLineInDoc and m_attachemntObjectPosInLine
	UINT64			m_attachmentOldFileSize; //to (1) track changes, and to (2) find item in the document (skip all files with different size, beofre doing MD5, which may be slow)
};

typedef CArray<SExternalFileMonitoringInfo,	SExternalFileMonitoringInfo&>	CExtFilesMonitoringArray;
class CExternalFilesMonitorBackgroundThread;

#define ID_COPYDATA_EXT_ATTACHMENT_MODIFICATION	3447


class CExternalFilesMonitor
{
public:

	//Methods:
						CExternalFilesMonitor();
	virtual				~CExternalFilesMonitor();

	void				AddFileToBeDeletedLater(const CString& filePath, bool bWipeFileContentAlso = false);
	void				DeleeteAllQueuedFiles();

	void				AddFileToMonitorExternalModification(const SExternalFileMonitoringInfo& fileToMonitorInfo);
	void				UpdateFileInfoToMonitorExternalModification(const CString& filePath, UINT64 newFileSize, const CString& newMD5, COleDateTime newFileModficationDate);
	void				ClearAllExtFilesModificationWatching();

private:

	//Data
	CAStringArray		m_listOfFileNamesToDelete;
	CAStringArray		m_listOfFileNamesToDeleteAndEraseContent;

	CExtFilesMonitoringArray				m_modificationTrackingInfo;
	CExternalFilesMonitorBackgroundThread*	m_pFileModWatcherThread;

	//Friends :)
	friend class CExternalFilesMonitorBackgroundThread;
};

extern CExternalFilesMonitor g_external_files_monitor;
