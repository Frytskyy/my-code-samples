//	This is a part of the Cyberium codename project. Part of AllMyNotes Organizer source.
//	Copyright (c) 2000-2024 Volodymyr Frytskyy. All rights reserved.
//
//	THIS SOURCE FILE IS THE PROPERTY OF VOLODYMYR FRYTSKYY AND IS NOT TO BE
//	RE-DISTRIBUTED OR USED BY ANY MEANS WHATSOEVER WITHOUT THE EXPRESSED WRITTEN
//	CONSENT OF VOLODYMYR FRYTSKYY.
//
//	CONTACT INFORMATION: support@vladonai.com, www.vladonai.com, www.allmynotes.org
//	License: https://www.vladonai.com/allmynotes-organizer-license-agreement

#include <StdAfx.h>
#include <malloc.h>
#include "Memory.h"
#include "Log.h"
#include "FileUtils.h"
#ifdef  APP_ForEndUser
	#include "Strings.h"
	#include "MsgBoxes.h"
	#include "UIUtils.h"
	#include "StrRes.h"
#endif
#include "md5.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////////////////////////////////////////////
//
//	CMemory class members
//

CMemory::CMemory(const CMemory& srcArray) :
	m_size(0)
{
	m_pMem = NULL;
	*this = srcArray;
}

CMemory::CMemory(size_t size /*= 0*/, bool bSetWithZeors /*= false*/) :
	m_size(0)
{
	Realloc(size, bSetWithZeors);
}

/*virtual*/ CMemory::~CMemory()
{
	Free();
}

Ptr CMemory::Realloc(size_t size, bool bSetWithZeors /*= false*/)
{
	//WARNING: be careful with returned pointer, do not resize it, do not delete it!!
	try
	{
		if (m_size != size)
		{
			if (size)
			{
				if (m_pMem)
					m_pMem = realloc(m_pMem, size);
				else
					m_pMem = malloc(size);
				if (NULL == m_pMem.pVoid)
				{
					ASSERT(false);
					m_pMem.Null();
					m_size = 0;

					return NULL;
				}

				if (bSetWithZeors)
					memset(m_pMem, 0, size);
			} else
			if (m_pMem)
			{
				free(m_pMem);
				m_pMem.Null();
			}

			m_size = size;
		}

		return m_pMem;
	}
	catch (...)
	{
		ASSERT(false);
		if (m_pMem.pVoid)
			Free();

		return NULL;
	}
}

void CMemory::CreateFromMemoryPtr(void* pSrcMemoryBlock)
{
	try
	{
		size_t	srcBlockSize = pSrcMemoryBlock ? _msize(pSrcMemoryBlock) : 0;

		if (srcBlockSize)
		{
			Ptr	ptr = Realloc(srcBlockSize);

			if (ptr.pVoid != NULL)
				memcpy(ptr.pVoid, pSrcMemoryBlock, srcBlockSize);
			else
				ASSERT(false); //must never happen
		} else
		{
			Free();
		}
	}
	catch (...)
	{
		ASSERT(false); //must never happen
		Free();
	}
}

void CMemory::Free()
{
	try
	{
		if (m_pMem)
		{
			free(m_pMem);
			m_pMem.Null();
			m_size = 0;
		}
	}
	catch (...)
	{
		m_pMem.Null();
		m_size = 0;
		ASSERT(false);
	}
}

void CMemory::Attach(void* pMemoryBuf, size_t size)
{
	Free();

	size_t	realSize = _msize(pMemoryBuf);

	ASSERT(size <= realSize);
	if (realSize > size)
	{
		pMemoryBuf = realloc(pMemoryBuf, size);
		ASSERT(pMemoryBuf);
	}
	m_pMem = pMemoryBuf;
	m_size = pMemoryBuf ? size : 0;
}

Ptr CMemory::Detach()
{
	Ptr	pDetachedData = m_pMem;

	if (m_pMem)
	{
		m_pMem.Null();
		m_size = 0;
	}

	return pDetachedData;
}

size_t CMemory::GetSize() const
{
	if (m_pMem.pVoid)
	{
		ASSERT(m_size == _msize(m_pMem.pVoid));

		return m_size;
	} else
	{
		return 0;
	}
}

CString CMemory::DetectFileExtensionByBinaryFileContent()
{
	try
	{
		int		dataBlockSize = (int)GetSize();

		if (dataBlockSize < 10)
			return CString(); //unknown format

		//
		LPBYTE	pDataBytes = GetPtr().pByte;

		//detect GIF
		if (dataBlockSize > 6 &&
			pDataBytes[0] == 'G' && pDataBytes[1] == 'I' && pDataBytes[2] == 'F' && pDataBytes[3] == '8' && 
			(pDataBytes[4] == '7' || pDataBytes[4] == '9') &&
			pDataBytes[5] == 'a')
		{
			//47 49 46 38 37 61	 	GIF87a
			// or
			//47 49 46 38 39 61	 	GIF89a
			return _T("gif");
		}

		//detect PNG
		if (dataBlockSize > 8 &&
			pDataBytes[0] == 0x89 && pDataBytes[1] == 'P' && pDataBytes[2] == 'N' && pDataBytes[3] == 'G' && 
			pDataBytes[4] == 0x0D && pDataBytes[5] == 0x0A && pDataBytes[6] == 0x1A && pDataBytes[7] == 0x0A)
		{
			//89 50 4E 47 0D 0A 1A 0A	 	‰PNG....
			return _T("png");
		}

		//detect JPEG
		if (dataBlockSize > 12 &&
			pDataBytes[0] == 0xFF && (pDataBytes[1] == 0xD8 || pDataBytes[1] == 0xD9) && pDataBytes[2] == 0xFF)
		{
			//FF D8 FF E0 xx xx 4A 46 49 46 00	 	ÿØÿà..JFIF. - JFIF, JPE, JPEG, JPG	 	JPEG/JFIF graphics file. Trailer: FF D9 (ÿÙ)
			//  or
			//FF D8 FF E1 xx xx 45 78 69 66 00	 	ÿØÿá..Exif. - JPG Digital camera JPG using Exchangeable Image File Format (EXIF). Trailer: FF D9 (ÿÙ)
			//or 
			//FF D8 FF E8 xx xx 53 50 49 46 46 00	ÿØÿè..SPIFF. - JPG Still Picture Interchange File Format (SPIFF). Trailer: FF D9 (ÿÙ)
			//
			//NOTES on JPEG file headers: It appears that one can safely say that all JPEG files start with the three hex digits 0xFF-D8-FF.
			//The fourth digit is also indicative of JPEG content. Various options include:
			//
			//0xFF-D8-FF-DB — Samsung D807 JPEG file.
			//0xFF-D8-FF-E0 — Shown above. Standard JPEG/JFIF file.
			//0xFF-D8-FF-E1 — Shown above. Standard JPEG/Exif file.
			//0xFF-D8-FF-E2 — Canon EOS-1D JPEG file.
			//0xFF-D8-FF-E3 — Samsung D500 JPEG file.
			//0xFF-D8-FF-E8 — Shown above. Still Picture Interchange File Format (SPIFF).
			return _T("jpg");
		}

		//detect BMP
		if (dataBlockSize > 3 &&
			pDataBytes[0] == 'B' && pDataBytes[1] == 'M')
		{
			//42 4D	 	BM - BMP, DIB	 	Windows (or device-independent) bitmap image
			//NOTE: Bytes 2-5 contain the file length in little-endian order.
			return _T("bmp");
		}

		//detect WMF
		if (dataBlockSize > 6 &&
			((pDataBytes[0] == 0x01 && pDataBytes[1] == 0x00 && pDataBytes[2] == 0x09 && pDataBytes[3] == 0x00 && pDataBytes[4] == 0x00 && pDataBytes[5] == 0x03) ||
			 (pDataBytes[0] == 0xD7 && pDataBytes[1] == 0xCD && pDataBytes[2] == 0xC6 && pDataBytes[3] == 0x9A)))
		{
			//01 00 09 00 00 03	 	WMF	 	Windows Metadata file (Win 3.x format)
			//D7 CD C6 9A	 		WMF	 	Windows graphics metafile
			return _T("wmf");
		}

		//detect EMF
		if (dataBlockSize > 5 &&
			(pDataBytes[0] == 0x01 && pDataBytes[1] == 0x00 && pDataBytes[2] == 0x00 && pDataBytes[3] == 0x00))
		{
			//01 00 00 00	 	EMF	 	Extended (Enhanced) Windows Metafile Format, printer spool file
			//	(0x18-17 & 0xC4-36 is Win2K/NT; 0x5C0-1 is WinXP)
			return _T("emf");
		}
	}
	catch (...)
	{
		ASSERT(false);
	}

	return CString(); //unknown format
}

CMemory& CMemory::operator=(const CMemory& srcArray)
{
	try
	{
		if (this != &srcArray)
		{
			size_t	arraySize = srcArray.GetSize();

			Realloc(arraySize);
			if (arraySize > 0 && m_pMem.pVoid)
				memcpy(GetPtr(), srcArray.GetPtr(), arraySize);
		}
	}
	catch (...)
	{
		ASSERT(false); //out of memory?
	}

	return *this;
}

CMemory& CMemory::operator=(CHugeMemory& srcArray)
{
	try
	{
		ULONGLONG			dataSize = srcArray.GetSize();
		CMemoryBlocksArray&	srcMemoryBlocks = srcArray.GetMemoryBlocks();
		INT_PTR				srcBlocksCount = srcMemoryBlocks.GetSize();
		byte*				pDstData;

		ASSERT(dataSize <= UINT32_MAX); //to-do:...
		Realloc((size_t)dataSize);
		if (dataSize > 0 && m_pMem.pVoid)
		{
			pDstData = m_pMem.pByte;
			for (INT_PTR blockN = 0; blockN < srcBlocksCount; blockN ++)
			{
				INT_PTR		curBlockSize = srcMemoryBlocks[blockN].GetSize();

				if (curBlockSize > 0)
					memcpy(pDstData, srcMemoryBlocks[blockN].GetPtr().pByte, curBlockSize);
				pDstData += curBlockSize;
			}
		}
	}
	catch (...)
	{
		ASSERT(false); //out of memory?
	}

	return *this;
}

//////////////////////////////////////////////////////////////////////
//
//	CHugeMemory class members
//

#define HUGE_MEMORY_SINGLE_BLOCK_SIZE	(0x2000) //size in bytes - 8 Kb (smaller blocks can be backed better in highly fragmended ram)
//#define HUGE_MEMORY_SINGLE_BLOCK_SIZE	(0xFFFF) //size in bytes - 64 Kb


CHugeMemory::CHugeMemory()
{
}

CHugeMemory::CHugeMemory(const CHugeMemory& src)
{
	*this = src;
}

/*virtual*/ CHugeMemory::~CHugeMemory()
{
	Free();
}

bool CHugeMemory::Alloc(ULONGLONG size)
{
	Free();

	if (size > 0)
	{
		int		lastBlockSize = size % HUGE_MEMORY_SINGLE_BLOCK_SIZE;
		bool	bAllocExtraBlockInTheEnd = ((lastBlockSize < HUGE_MEMORY_SINGLE_BLOCK_SIZE) && (lastBlockSize > 0));
		int		blocksCount = int(size / HUGE_MEMORY_SINGLE_BLOCK_SIZE) + bAllocExtraBlockInTheEnd;

		if (!bAllocExtraBlockInTheEnd)
		{
			ASSERT(blocksCount > 0 && lastBlockSize == 0);
			lastBlockSize = HUGE_MEMORY_SINGLE_BLOCK_SIZE;
		}

		m_memoryBlocks.SetSize(blocksCount, 0);
		for (INT_PTR memoryBlockN = 0; memoryBlockN < blocksCount; memoryBlockN ++)
		{
			Ptr	memBlockPtr = m_memoryBlocks[memoryBlockN].Realloc((memoryBlockN + 1 < blocksCount) ? HUGE_MEMORY_SINGLE_BLOCK_SIZE : lastBlockSize, false);
			
			if (!memBlockPtr)
			{
				//out of memory!
				ASSERT(false);
				Free();

				return false; //error
			}
		}
		ASSERT(GetSize() == size);

		return true; //success
	} else
	{
		return true; //success
	}
}

void CHugeMemory::Free()
{
	m_memoryBlocks.RemoveAll();
	ASSERT(GetSize() == 0);
}

bool CHugeMemory::AddMemoryBlock(const void* pBuf, int nBufSize)
{
	if (pBuf && nBufSize)
	{
		int			newBlockIndex = (int)m_memoryBlocks.Add(CMemory());
		CMemory&	newBlock = m_memoryBlocks[newBlockIndex];
		
		newBlock.Realloc(nBufSize);
		if (newBlock.GetPtr().pVoid)
		{
			memcpy(newBlock.GetPtr().pVoid, pBuf, nBufSize);
			return true;
		}
	}

	return false;
}

ULONGLONG CHugeMemory::GetSize() const
{
	ULONGLONG	size = 0; //result
	INT_PTR		blocksCount = m_memoryBlocks.GetSize();

	for (INT_PTR memoryBlockN = 0; memoryBlockN < blocksCount; memoryBlockN ++)
	{
		size += m_memoryBlocks[memoryBlockN].GetSize();
	}

	return size;
}

/*static*/ size_t CHugeMemory::GetMaxBlockSize()
{
	return HUGE_MEMORY_SINGLE_BLOCK_SIZE;
}

CString CHugeMemory::DetectFileExtensionByBinaryFileContent()
{
	try
	{
		if (m_memoryBlocks.GetSize() < 1)
			return CString(); //unknown format - no data inside
		
		return m_memoryBlocks[0].DetectFileExtensionByBinaryFileContent();
	}
	catch (...)
	{
		ASSERT(false);
	}

	return CString(); //unknown format
}

CString CHugeMemory::GetMD5() const
{
	return md5((CHugeMemory*)this);
}

bool CHugeMemory::WriteToFile(const CString& filePath) const
{
	try
	{
		CFile	outputFile(filePath, CFile::modeCreate | CFile::modeWrite);
		bool	bSuccess = WriteToFile(&outputFile);

		return bSuccess;
	}
	catch (...)
	{
		ASSERT(false);

		return false; //error
	}
}

bool CHugeMemory::WriteToFile(CFile* pFile) const
{
	try
	{
		ULONGLONG	fileSize = GetSize();
		UINT		memBlocksCount = (UINT)m_memoryBlocks.GetSize();

		pFile->SetLength(fileSize);
		pFile->Seek(0, CFile::begin);
		for (UINT memBlockN = 0; memBlockN < memBlocksCount; memBlockN ++)
		{
			size_t	curBlockSize = m_memoryBlocks[memBlockN].GetSize();

			ASSERT(curBlockSize <= UINT32_MAX);
			pFile->Write(m_memoryBlocks[memBlockN].GetPtr().pByte, (UINT)curBlockSize);
		}

		return true; //success
	}
	catch (...)
	{
		ASSERT(false);

		return false; //error
	}
}

CHugeMemory& CHugeMemory::operator=(const CMemory& src)
{
	size_t			summarySize = src.GetSize();

	Free();
	if (summarySize > 0)
	{
		INT_PTR		memBlocksCount;
		LPBYTE		pSrcData = src.GetPtr().pByte;
		UINT_PTR	srcDataOffset = 0;

		Alloc(summarySize);
		memBlocksCount = m_memoryBlocks.GetSize();

		for (INT_PTR memBlockN = 0; memBlockN < memBlocksCount; memBlockN ++)
		{
			size_t	curBlockSize = m_memoryBlocks[memBlockN].GetSize();

			ASSERT(curBlockSize <= UINT32_MAX);
			memcpy(m_memoryBlocks[memBlockN].GetPtr().pByte, &pSrcData[srcDataOffset], (UINT)curBlockSize);
			srcDataOffset += curBlockSize;
		}
	}

	return *this;
}

CHugeMemory& CHugeMemory::operator=(const CHugeMemory& src)
{
	Free();

	if (src.m_memoryBlocks.GetSize() > 0)
	{
		m_memoryBlocks.Copy(src.m_memoryBlocks);
		ASSERT(GetSize() == src.GetSize());
	}

	return *this;
}

BOOL CHugeMemory::operator==(const CHugeMemory& otherObj) const
{
	try
	{
		int			blocksCount = (int)m_memoryBlocks.GetSize();
		ULONGLONG	memSize = GetSize();

		if (memSize != otherObj.GetSize())
			return FALSE;

		if (memSize == 0)
			return TRUE; //both arrays are empty :)

		if (blocksCount != otherObj.m_memoryBlocks.GetSize())
		{
			ASSERT(false); //incorrect blocks count - should not happen!
			return FALSE; //return false because it's unsupported case, it should not happen!
		}

		for (INT_PTR memoryBlockN = 0; memoryBlockN < blocksCount; memoryBlockN ++)
		{
			CMemory&	curMemBlock = (CMemory&)m_memoryBlocks[memoryBlockN];
			CMemory&	curOtherObjMemBlock = (CMemory&)otherObj.m_memoryBlocks[memoryBlockN];
			int			blockSize = (int)curMemBlock.GetSize(); //size in bytes

			if (blockSize != curOtherObjMemBlock.GetSize())
			{
				ASSERT(false); //incorrect block content size, it must be equal for both blocks!
				return FALSE; //return false because it's unsupported case, it should not happen!
			}

			if (memcmp(curMemBlock.GetPtr().pVoid, curOtherObjMemBlock.GetPtr().pVoid, blockSize) != 0)
				return FALSE; //memory content is different
		}

		//objects are exactly same
		return TRUE;
	} catch (...)
	{
		ASSERT(false); //must never happen

		return FALSE;
	}
}


//////////////////////////////////////////////////////////////////////
//
// CHugeMemoryCollector class members
//


CHugeObjsKeeper::CHugeObjsKeeper() :
	m_objectIDsCounter(1)
{
}

/*virtual*/ CHugeObjsKeeper::~CHugeObjsKeeper()
{
	m_criticalSection.Lock(INFINITE);
	ASSERT(m_memBlocks.GetCount() == 0);
#ifdef APP_ForEndUser
	LOGVERIFY(m_memBlocks.GetCount() == 0, _T("Warning: possible memory leak detected, number of unreleased memory blocks is greater than 0."));
#endif //APP_ForEndUser
	m_criticalSection.Unlock();
}

int CHugeObjsKeeper::GetObjectsCount()
{
	int	objCount;

	m_criticalSection.Lock(INFINITE);
	objCount = (int)m_memBlocks.GetSize();
	m_criticalSection.Unlock();

	return objCount;
}

UINT64 CHugeObjsKeeper::CreateNewObj(ULONGLONG size)
{
	//return new object ID
	SHugeBlockRefInfo	objInfo;

	m_criticalSection.Lock(INFINITE);

	objInfo.m_objectID = m_objectIDsCounter ++;
	objInfo.m_pHugeMem = new CHugeMemory();
	if (size > 0)
		objInfo.m_pHugeMem->Alloc(size);
	objInfo.m_refCounter = 1;
	m_memBlocks.Add(objInfo);

	m_criticalSection.Unlock();

	return objInfo.m_objectID;
}

UINT64 CHugeObjsKeeper::AttachObject(CHugeMemory* pHugeMemObj)
{
	//return new object ID, note: after this function call the pointer deletion is fully managed by this class, including DELETION
	UINT64	objID; //result

	if (pHugeMemObj == NULL)
	{
		ASSERT(false); //it makes no sense...
		objID = 0;
	} else
	{
		objID = GetObjID(pHugeMemObj->GetSize(), pHugeMemObj->GetMD5(), true);

		if (objID == 0)
		{
			//new object, attach it
			SHugeBlockRefInfo	objInfo;

			m_criticalSection.Lock(INFINITE);

			objInfo.m_objectID = m_objectIDsCounter ++;
			objInfo.m_pHugeMem = pHugeMemObj; //from now on this class is responsible for object DELETION
			objInfo.m_refCounter = 1;
			m_memBlocks.Add(objInfo);

			m_criticalSection.Unlock();

			objID = objInfo.m_objectID;
		} else
		{
			if (GetObjectPtrByID(objID) != pHugeMemObj)
				delete pHugeMemObj;
		}
	}

	return objID;
}

CHugeMemory* CHugeObjsKeeper::GetObjectPtrByID(UINT64 objID/*, bool bIncrRefCounter*/)
{
	ASSERT(objID != 0);
	m_criticalSection.Lock(INFINITE);

	//
	CHugeMemory*	pResultMemObj = NULL;
	int				objN = GetObjectIndexByID(objID, false);

	if (objN >= 0)
	{
		ASSERT(m_memBlocks[objN].m_objectID == objID);
		ASSERT(m_memBlocks[objN].m_refCounter >= 1);
		pResultMemObj = m_memBlocks[objN].m_pHugeMem;
		ASSERT(pResultMemObj);
		//if (bIncrRefCounter)
		//	m_memBlocks[objN].m_refCount ++;
	} else
	{
		ASSERT(false); //should not happen
	}

	m_criticalSection.Unlock();

	return pResultMemObj;
}

int CHugeObjsKeeper::GetObjectIndexByID(UINT64 objID, bool bForCreation)
{
	//return -1 if not found
	ASSERT(objID != 0);
	m_criticalSection.Lock(INFINITE);

	//
	int	resultObjIndex; //result
	int	itemsCount = (int)m_memBlocks.GetSize();

	if (!itemsCount)
	{
		resultObjIndex = bForCreation ? 0 : -1;
	} else
	{
		CBinarySearch	binarySeeker(itemsCount);
		int				curIndex;

		while (binarySeeker.GetElementIndexToTry(&curIndex))
		{
			ASSERT(curIndex < itemsCount);
			if (m_memBlocks[curIndex].m_objectID > objID)
			{
				binarySeeker.NextStep(true);
			} else
				if (m_memBlocks[curIndex].m_objectID < objID)
				{
					binarySeeker.NextStep(false);
				} else
				{
					ASSERT(m_memBlocks[curIndex].m_objectID == objID);
					if (bForCreation)
					{
						curIndex ++;
						ASSERT(curIndex <= itemsCount);
					}
					resultObjIndex = curIndex;

					goto foundExitPoint;
				}
		}

		if (bForCreation)
		{
			ASSERT(curIndex >= 0 && curIndex <= itemsCount);
			if (curIndex < itemsCount &&
				m_memBlocks[curIndex].m_objectID <= objID)
			{
				ASSERT(curIndex < itemsCount ||
					m_memBlocks[curIndex].m_objectID > objID);
				curIndex ++;
			}

			resultObjIndex = curIndex;
		} else
		{
			resultObjIndex = -1;
		}
	}

	//linear search (legacy, slow):
	//	int	objectsCount = m_memBlocks.GetSize();
	// 	for (int objN = 0; objN < objectsCount; objN ++)
	// 	{
	// 		if (m_memBlocks[objN].m_objectID == objID)
	// 			return objN; //found!
	// 	}
	// 
	// 	//not found
	// 	return -1;

foundExitPoint:
	m_criticalSection.Unlock();

	return resultObjIndex;
}

UINT64 CHugeObjsKeeper::GetObjID(ULONGLONG size, CString objMD5, bool bIncrRefCounter)
{
	//return 0 if not found
	//note: size may be 0, in this case only objMD5 param will be used, but size param may greatly accelerate function execution time
	ASSERT(!objMD5.IsEmpty());
	m_criticalSection.Lock(INFINITE);

	UINT64	objID = 0; //result
	int		objsCount = (int)m_memBlocks.GetCount();

	for (int objN = 0; objN < objsCount; objN ++)
	{
		CHugeMemory*	pCurMemoryObj = m_memBlocks[objN].m_pHugeMem;

		if (pCurMemoryObj &&
			(size == 0 || size == pCurMemoryObj->GetSize()))
			if (pCurMemoryObj->GetMD5() == objMD5) //to-do: idea: use shortened MD5 for mem beginning only (Ex: 1KB)
			{
				objID = m_memBlocks[objN].m_objectID;
				if (bIncrRefCounter)
					m_memBlocks[objN].m_refCounter ++;

				break;
			}
	}

	m_criticalSection.Unlock();

	return objID; //not found
}

void CHugeObjsKeeper::AddAnotherObjectReferrence(UINT64 objectID)
{
	m_criticalSection.Lock(INFINITE);

	int		objN = GetObjectIndexByID(objectID, false);

	if (objN >= 0)
	{
		ASSERT(m_memBlocks[objN].m_refCounter >= 1);
		m_memBlocks[objN].m_refCounter ++;
	} else
	{
		ASSERT(false); //shouldn't happen
	}

	m_criticalSection.Unlock();
}

void CHugeObjsKeeper::RemoveObjectReferrence(UINT64 objID)
{
	//delete the object if there is no other refferrence to it
	ASSERT(objID != 0);
	m_criticalSection.Lock(INFINITE);

	//
	int				objN = GetObjectIndexByID(objID, false);

	if (objN >= 0)
	{
		ASSERT(m_memBlocks[objN].m_objectID == objID);
		ASSERT(m_memBlocks[objN].m_refCounter >= 1);
		m_memBlocks[objN].m_refCounter --;
		if (m_memBlocks[objN].m_refCounter == 0)
		{
			if (m_memBlocks[objN].m_pHugeMem)
			{
				delete m_memBlocks[objN].m_pHugeMem;
				m_memBlocks[objN].m_pHugeMem = NULL;
			} else
			{
				ASSERT(false); //must not happen
			}
			m_memBlocks.RemoveAt(objN);
		}
	} else
	{
		ASSERT(false); //should not happen
	}

	m_criticalSection.Unlock();
}

/////////////////////////////////////////////////////////////////////////////
// The Global object (for various purposes)
CHugeObjsKeeper g_huge_mem_objects_keeper;
//
/////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
//
// CAdvPtrList class members
//

#define NOT_INITIALIZED	-1


CAdvPtrList::CAdvPtrList() :
	m_NodesCount(0),
	m_pFirstNode(NULL),
	m_LastAccesedIndex(NOT_INITIALIZED),
	m_pLastAccesedNode(NULL)
{
}

/*virtual*/ CAdvPtrList::~CAdvPtrList()
{
	ASSERT(GetCount() == 0); //caller shoud remove all items, otherwise this assert may men memory leak or bad memory management
	Clear();
}

Ptr CAdvPtrList::GetNext(LPNODE& pNode)
{
	sNode* p_node = (sNode*)pNode;

	pNode = (LPNODE)p_node->pNext;
	return p_node->ptr;
}

Ptr CAdvPtrList::GetPrev(LPNODE& pNode)
{
	sNode* p_node = (sNode*)pNode;

	pNode = (LPNODE)p_node->pPrev;
	return p_node->ptr;
}

LPNODE CAdvPtrList::Find(Ptr value) const
{
	for (sNode* pNode = m_pFirstNode; pNode != NULL; pNode->pNext)
	{
		if (pNode->ptr.pVoid == value.pVoid)
			return pNode;
		pNode = pNode->pNext;
	}

	return NULL;
}

void CAdvPtrList::Add(Ptr newPtr)
{
	sNode* pNode = new sNode;
	
	pNode->ptr = newPtr;
	
	if (m_pFirstNode)
	{
		//insert new item at the very beginning (it is the fastest way)
		pNode->pPrev = NULL;
		pNode->pNext = m_pFirstNode;
		m_pFirstNode->pPrev = pNode;
		m_pFirstNode = pNode;
	} else
	{
		m_pFirstNode = pNode;
		m_pFirstNode->pPrev = NULL;
		m_pFirstNode->pNext = NULL;
	};
	
	m_NodesCount ++;

	if (m_LastAccesedIndex != NOT_INITIALIZED)
		m_LastAccesedIndex ++;
}

void CAdvPtrList::Insert(int index, Ptr ptr)
{
	ASSERT(index >= 0);
	ASSERT(index <= m_NodesCount);

	sNode*	pNode = new sNode;
	sNode*	pNodeToShiftUp = index < m_NodesCount ? GetNodeByIndex(index) : NULL;
	sNode*	pPrevNode = pNodeToShiftUp ? pNodeToShiftUp->pPrev : GetNodeByIndex(max(index - 1, 0));

	pNode->ptr = ptr;
	pNode->pPrev = pPrevNode;
	pNode->pNext = pNodeToShiftUp;

	if (pPrevNode)
	{
		ASSERT(pPrevNode->pNext == pNodeToShiftUp);
		pPrevNode->pNext = pNode;
	}

	if (pNodeToShiftUp)
		pNodeToShiftUp->pPrev = pNode;

	ASSERT((index == 0 && m_pFirstNode == pNodeToShiftUp) ||
		   index != 0);
	if (m_pFirstNode == pNodeToShiftUp)
		m_pFirstNode = pNode;

	m_NodesCount ++;
	if (m_LastAccesedIndex == index)
		m_pLastAccesedNode = pNode;
		else
		if (m_LastAccesedIndex > index)
			m_LastAccesedIndex ++;
}

Ptr CAdvPtrList::RawRemove(LPNODE pNode)
{
	ASSERT(m_pFirstNode);
	
	sNode*	pNodeToDel = (sNode*)pNode;
	sNode*	prevNode = pNodeToDel->pPrev;
	sNode*	nextNode = pNodeToDel->pNext;
	Ptr		value = pNodeToDel->ptr;
	
	if (prevNode)
		prevNode->pNext = nextNode;
	if (nextNode)
		nextNode->pPrev = prevNode;
	
	if (pNodeToDel == m_pFirstNode)
		m_pFirstNode = nextNode;

	delete pNodeToDel;
	m_NodesCount --;

	return value;
}

bool CAdvPtrList::Remove(Ptr ptr)
{
	//return FALSE if item wasn't found in the array
	LPNODE	pNode = Find(ptr);
	
	if (pNode)
	{
		Remove(pNode);
		return true;
	} else
	{
		//ptr not found
		return false;
	}
}

Ptr CAdvPtrList::Remove(LPNODE pNode)
{
	m_LastAccesedIndex = NOT_INITIALIZED;
	m_pLastAccesedNode = NULL;
	return RawRemove(pNode);
}


Ptr CAdvPtrList::Remove(int index)
{
	ASSERT(index < m_NodesCount);

	sNode*	pNodeToDel = GetNodeByIndex(index);

	if (m_NodesCount == 1 || index == m_LastAccesedIndex)
	{
		m_LastAccesedIndex = NOT_INITIALIZED;
		m_pLastAccesedNode = NULL;
	} else
	if (m_LastAccesedIndex != NOT_INITIALIZED &&
		m_LastAccesedIndex > index)
	{
		m_LastAccesedIndex --;
	};
	return RawRemove(pNodeToDel);	
}

Ptr CAdvPtrList::Get(int index)
{
	sNode*	pNode = GetNodeByIndex(index);
	
	return pNode->ptr;
}

void CAdvPtrList::Set(int index, Ptr value)
{
	sNode*	pNode = GetNodeByIndex(index);

	pNode->ptr = value;
}

CAdvPtrList::sNode* CAdvPtrList::GetNodeByIndex(int index)
{
	//return node pointer by index
	ASSERT(index < m_NodesCount);
	ASSERT(m_pFirstNode);
	
	sNode*		curNode;
	int			curIndex;
	
	if (m_LastAccesedIndex != NOT_INITIALIZED &&
		(index > m_LastAccesedIndex || m_LastAccesedIndex - index < index))
	{
		ASSERT(m_pLastAccesedNode != NULL);
		curIndex = m_LastAccesedIndex;
		curNode  = m_pLastAccesedNode;
	} else
	{
		curNode  = m_pFirstNode;
		curIndex = 0;
	}
	m_LastAccesedIndex = index;
	
	if (curIndex <= index)
	{
		while (curNode)
		{
			if (curIndex == index)
			{
				m_pLastAccesedNode = curNode;
				return curNode;
			} else
			{
				curNode = curNode->pNext;
			}
			curIndex ++;
		};
	} else //if (curIndex <= index)
	{
		while (curNode)
		{
			if (curIndex == index)
			{
				m_pLastAccesedNode = curNode;
				return curNode;
			} else
			{
				curNode = curNode->pPrev;
			}
			curIndex --;
		};
	} //end if (curIndex <= index)
	
	ASSERT(false); // never should not happen
	m_LastAccesedIndex = NOT_INITIALIZED;
	m_pLastAccesedNode = NULL;
	
	return NULL;
}

void CAdvPtrList::Clear()
{
	sNode*	curNode = m_pFirstNode;
	sNode*	nextNode;
	
	while (curNode)
	{
		nextNode = curNode->pNext;
		delete curNode;
		curNode = nextNode;
	};
	
	m_pFirstNode = NULL;
	m_NodesCount = 0;

	m_LastAccesedIndex = NOT_INITIALIZED;
	m_pLastAccesedNode = NULL;
}


//////////////////////////////////////////////
//
//	CAdvPtrsArray class members
//


CAdvPtrsArray::CAdvPtrsArray() : 
	m_ItemsCount(0), 
	m_AllocatedCount(0)
{
};

/*virtual*/ CAdvPtrsArray::~CAdvPtrsArray()
{
	Clear();
}

void CAdvPtrsArray::AddPtr(Ptr pItem)
{	
	if (m_AllocatedCount < m_ItemsCount + 1)
	{
		m_AllocatedCount += 0xFF; //allocate by 256 items at once - to increase allocation speed
		
		int	newArraySize = m_AllocatedCount * sizeof(Ptr);
		
		m_ptrsArray.Realloc(newArraySize);
	};
	
	m_ptrsArray.GetPtr().pPtr[m_ItemsCount] = pItem;
	m_ItemsCount ++;
}

void CAdvPtrsArray::SetPtr(int index, Ptr pItem)
{
	ASSERT(index >= 0 && index < m_ItemsCount);
	m_ptrsArray.GetPtr().pPtr[index] = pItem;
}

Ptr CAdvPtrsArray::GetPtr(int index)
{
	ASSERT(int(m_ptrsArray.GetSize() * sizeof(Ptr)) > index);
	ASSERT(index >= 0 && index < m_ItemsCount);
	
	return m_ptrsArray.GetPtr().pPtr[index];
}

void CAdvPtrsArray::RemovePtr(int index)
{
	const int	onePtrSize = sizeof(Ptr);
	
	ASSERT(m_ItemsCount > index);
	if (m_ItemsCount != index + 1)
	{
		Ptr		ptrs = m_ptrsArray.GetPtr();
		Ptr		ptrToRemove = ptrs;
		Ptr		ptrNextToRemoved;
		
		ptrToRemove = ptrs;
		ptrToRemove.Inc(index * onePtrSize);
		ptrNextToRemoved = ptrToRemove;
		ptrNextToRemoved.Inc(onePtrSize);
		memmove(ptrToRemove, ptrNextToRemoved, (m_ItemsCount - index - 1) * onePtrSize);
	}
	
	m_ItemsCount --;
}

bool CAdvPtrsArray::RemovePtr(Ptr ptr)
{
	//return true if pointer was found
	int	ptrIndex = FindPtrIndex(ptr);

	if (ptrIndex >= 0)
	{
		RemovePtr(ptrIndex);

		return TRUE;
	} else
	{
		//pointer not found
		return TRUE;
	}
}

int CAdvPtrsArray::FindPtrIndex(Ptr ptr)
{
	//may return -1 if nothing found
	for (int itemN = 0; itemN < m_ItemsCount; itemN ++)
		if (GetPtr(itemN) == ptr)
			return itemN;

	//not found
	return -1;
}

void CAdvPtrsArray::Clear()
{
	if (m_ptrsArray.GetSize())
	{
		m_ptrsArray.Free();
		m_ItemsCount = 0;
		m_AllocatedCount = 0;
	} else
	{
		ASSERT(!m_ItemsCount);
		ASSERT(!m_AllocatedCount);
	}
}

CAdvPtrsArray& CAdvPtrsArray::operator=(const CAdvPtrsArray& anotherContainer)
{
	if (this != &anotherContainer)
	{
		int	size = (int)anotherContainer.m_ptrsArray.GetSize();

		m_ItemsCount = anotherContainer.m_ItemsCount;
		m_AllocatedCount = anotherContainer.m_AllocatedCount;
		m_ptrsArray.Realloc(size);
		memcpy(m_ptrsArray.GetPtr(), anotherContainer.m_ptrsArray.GetPtr(), size);
	}

	return *this;
}

void CAdvPtrsArray::InvertItemsOrder()
{
	int		halfCount = m_ItemsCount / 2;
	int		itemNfromOtherSide;
	Ptr		pArray = m_ptrsArray.GetPtr().pVoid;
	Ptr		ptrVariable;

	for (int itemN = 0; itemN < halfCount; itemN ++)
	{
		itemNfromOtherSide = m_ItemsCount - itemN - 1;
		ASSERT(itemNfromOtherSide > itemN);

		ptrVariable = pArray.pPtr[itemN];
		pArray.pPtr[itemN] = pArray.pPtr[itemNfromOtherSide];
		pArray.pPtr[itemNfromOtherSide] = ptrVariable;
	}
}


//////////////////////////////////////////////
//
//	class CAMemFile
//


void CAMemFile::Attach(BYTE* lpBuffer, UINT nBufferSize, UINT nGrowBytes) 
{
	//note: original function CMemFile::Attach has a serious issue, it simply doesn't works as of 4/2018, as it sets zero file size, not matter what params where provided... (VMF)
	if (lpBuffer == NULL && nBufferSize != 0) 
	{
		AfxThrowInvalidArgException();
	}

	ASSERT(m_lpBuffer == NULL);

	m_nGrowBytes = nGrowBytes > 0 ? nGrowBytes : 1024; //modified my VMF, no zero growth! :)
	m_nPosition = 0;
	m_nBufferSize = nBufferSize;
	m_nFileSize = nBufferSize;
	//ERROR IN MFC (VMF): m_nFileSize = nGrowBytes == 0 ? nBufferSize : 0;
	m_lpBuffer = lpBuffer;
	m_bAutoDelete = FALSE;
}

//////////////////////////////////////////////
//
//	class CBinarySearch
//

CBinarySearch::CBinarySearch(int entriesCount) : 
	m_minIndex(-1), 
	m_entriesCount(entriesCount), 
	m_maxIndex(entriesCount), 
	m_curIndex(-1), 
	m_oldIndex(-1) 
{
}

bool CBinarySearch::GetElementIndexToTry(int* pElementIndexToTry)
{
	m_oldIndex = m_curIndex;
	m_curIndex = m_minIndex + max(1, ((m_maxIndex - m_minIndex) / 2)); //error for array size == 1
	*pElementIndexToTry = m_curIndex;
	ASSERT(m_curIndex >= 0 && m_curIndex <= m_entriesCount);

	return m_oldIndex != m_curIndex && m_curIndex < m_maxIndex;
}

void CBinarySearch::NextStep(bool bGoDown)
{
	if (bGoDown)
		m_maxIndex = m_curIndex;
	else
		m_minIndex = m_curIndex;// + 1; //warning: this "+1" was not working well in some cases!!!
}


//////////////////////////////////////////////
//
//	class CObjectsKeeper
//


CObjectsKeeper::CObjectsKeeper() :
	m_objectIDsCounter(1)
{
	m_criticalSection.Lock(INFINITE);

	m_objects.SetSize(0, 100); //allocate 100 entries per step, to make this array work fast

	m_criticalSection.Unlock();
}

/*virtual*/ CObjectsKeeper::~CObjectsKeeper()
{
	m_criticalSection.Lock(INFINITE);

	int	objectsCount = (int)m_objects.GetSize();

	ASSERT(objectsCount == 0); //all who allocated data here must release it
	//delete objects,though it's not good that creators didn't do it themselves...
	for (int objN = 0; objN < objectsCount; objN ++)
	{
		if (m_objects[objN].m_pObject)
			delete m_objects[objN].m_pObject;
	}
	m_objects.RemoveAll();

	m_criticalSection.Unlock();
}

UINT64 CObjectsKeeper::AttachObject(CObject* pObject)
{
	//return object ID, since this moment this class is responsible for object destruction
	ASSERT(m_objectIDsCounter > 0); //numeration starts from 1

	if (pObject)
	{
		UINT64			objID;
		SSingleObjInfo	objInfo;
		int				objN;

		objInfo.m_objectID = m_objectIDsCounter ++;
		objInfo.m_pObject = pObject;
		objInfo.m_refCounter = 1;

		m_criticalSection.Lock(INFINITE);

		objN = GetObjectIndexByID(objInfo.m_objectID, true);
		m_objects.InsertAt(objN, objInfo);
		objID = objInfo.m_objectID;

		m_criticalSection.Unlock();

		return objID;
	} else
	{
		ASSERT(false); //must never happen
		return 0;
	}
}

void CObjectsKeeper::AddAnotherObjectReferrence(UINT64 objectID)
{
	m_criticalSection.Lock(INFINITE);

	int		objN = GetObjectIndexByID(objectID, false);

	if (objN >= 0)
	{
		ASSERT(m_objects[objN].m_refCounter > 0);
		m_objects[objN].m_refCounter ++;
	} else
	{
		ASSERT(false); //shouldn't happen
	}

	m_criticalSection.Unlock();
}

void CObjectsKeeper::RemoveObjectReferrence(UINT64 objectID)
{
	m_criticalSection.Lock(INFINITE);

	int		objN = GetObjectIndexByID(objectID, false);

	if (objN >= 0)
	{
		ASSERT(m_objects[objN].m_refCounter > 0);
		m_objects[objN].m_refCounter --;
		if (m_objects[objN].m_refCounter == 0)
		{
			//delete object - it's no longer needed
			if (m_objects[objN].m_pObject)
				delete m_objects[objN].m_pObject;
			else
				ASSERT(false); //shouldn't happen
			m_objects.RemoveAt(objN);
		}
	} else
	{
		ASSERT(false); //shouldn't happen
	}

	m_criticalSection.Unlock();
}

int CObjectsKeeper::GetObjectsCount()
{
	m_criticalSection.Lock(INFINITE);

	int	objectsCount = (int)m_objects.GetSize();

	m_criticalSection.Unlock();

	return objectsCount;
}

int CObjectsKeeper::GetObjectIndexByID(UINT64 objectID, bool bForCreation)
{
	//return -1 if object not found in m_objectIDsCounter
	ASSERT(objectID != 0);
	m_criticalSection.Lock(INFINITE);

	//
	int	resultObjIndex; //result
	int	itemsCount = (int)m_objects.GetSize();

	if (!itemsCount)
	{
		resultObjIndex = bForCreation ? 0 : -1;
	} else
	{
		CBinarySearch	binarySeeker(itemsCount);
		int				curIndex;

		while (binarySeeker.GetElementIndexToTry(&curIndex))
		{
			ASSERT(curIndex < itemsCount);
			if (m_objects[curIndex].m_objectID > objectID)
			{
				binarySeeker.NextStep(true);
			} else
			if (m_objects[curIndex].m_objectID < objectID)
			{
				binarySeeker.NextStep(false);
			} else
			{
				ASSERT(m_objects[curIndex].m_objectID == objectID);
				if (bForCreation)
				{
					curIndex ++;
					ASSERT(curIndex <= itemsCount);
				}
				resultObjIndex = curIndex;

				goto foundExitPoint;
			}
		}

		if (bForCreation)
		{
			ASSERT(curIndex >= 0 && curIndex <= itemsCount);
			if (curIndex < itemsCount &&
				m_objects[curIndex].m_objectID <= objectID)
			{
				ASSERT(curIndex < itemsCount ||
					m_objects[curIndex].m_objectID > objectID);
				curIndex ++;
			}

			resultObjIndex = curIndex;
		} else
		{
			resultObjIndex = -1;
		}
	}

//linear search (slow):
//	int	objectsCount = m_objects.GetSize();
// 	for (int objN = 0; objN < objectsCount; objN ++)
// 	{
// 		if (m_objects[objN].m_objectID == objectID)
// 			return objN; //found!
// 	}
// 
// 	//not found
// 	return -1;

foundExitPoint:
	m_criticalSection.Unlock();

	return resultObjIndex;
}

CObject* CObjectsKeeper::GetObjectPtrByID(UINT64 objectID)
{
	int		objN = GetObjectIndexByID(objectID, false);

	if (objN >= 0)
	{
		m_criticalSection.Lock(INFINITE);

		CObject*	pObject = m_objects[objN].m_pObject;

		ASSERT(pObject);

		m_criticalSection.Unlock();

		return pObject;
	} else
	{
		ASSERT(false); //normally shouldn't happen
		return NULL;
	}
}

UINT64 CObjectsKeeper::GetObjectIDByPtr(CObject* pObject)
{
	//return object ID, or 0 if object doesn't exists
	ASSERT(pObject != NULL);
	m_criticalSection.Lock(INFINITE);

	//
	int	objectsCount = (int)m_objects.GetSize();

	for (int objN = 0; objN < objectsCount; objN ++)
	{
		if (m_objects[objN].m_pObject == pObject)
		{
			m_criticalSection.Unlock();

			return objN; //found!
		}
	}

	m_criticalSection.Unlock();

	//not found
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// The Global object (for various purposes)
CObjectsKeeper g_big_objects_keeper;
//
/////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////
//
//	class CExternalFilesMonitorBackgroundThread
//

class CExternalFilesMonitorBackgroundThread : public CWinThread
{
public:

	//Methods:
						CExternalFilesMonitorBackgroundThread(CExternalFilesMonitor* pExtFilesMonitor);

	void				Stop(); //this method destroys thread, so no need to delete its pointer
	CCriticalSection*	GetCriticalSectionObj() { return &m_criticalSection; } //for muti-thread access

	DECLARE_DYNAMIC(CExternalFilesMonitorBackgroundThread);

protected:

	//Methods:
	virtual			~CExternalFilesMonitorBackgroundThread();

	virtual int		Run();

	virtual BOOL	InitInstance();
	virtual int		ExitInstance();

	DECLARE_MESSAGE_MAP();

protected:

	//Data:
	BOOL					m_bNeedToExit;
	CExternalFilesMonitor*	m_pExtFilesMonitor;
	CCriticalSection		m_criticalSection; //for muti-thread access
};


BEGIN_MESSAGE_MAP(CExternalFilesMonitorBackgroundThread, CWinThread)
	//{{AFX_MSG_MAP(CExternalFilesMonitorBackgroundThread)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CExternalFilesMonitorBackgroundThread, CWinThread);


CExternalFilesMonitorBackgroundThread::CExternalFilesMonitorBackgroundThread(CExternalFilesMonitor* pExtFilesMonitor) :
	m_bNeedToExit(false),
	m_pExtFilesMonitor(pExtFilesMonitor)
{
	LogAddEntry(_T("Info: External attachments updates check for update thread is started")); //tmp: for debugging

	if (!CreateThread())
	{
		//should never happen
		ASSERT(false);
		LogAddEntry(_T("Error: Cannot create ext attachemnt files modification checker thread"));

		delete this;
	}
}

/*virtual*/ CExternalFilesMonitorBackgroundThread::~CExternalFilesMonitorBackgroundThread()
{
	LogAddEntry(_T("Info: External attachments updates check for update thread is finished")); //tmp: for debugging
}

/*virtual*/ int CExternalFilesMonitorBackgroundThread::Run()
{
	bool	bNeedToUnlockCriticalSection = false;

	try
	{
		while (!m_bNeedToExit)
		{
			bNeedToUnlockCriticalSection = true;
			m_criticalSection.Lock(INFINITE);

 			{
 				int		monitoredEntriesCount = (int)m_pExtFilesMonitor->m_modificationTrackingInfo.GetSize();
 
 				for (int entryN = monitoredEntriesCount - 1; entryN >= 0; entryN --)
 				{
 					SExternalFileMonitoringInfo&	curFileEntryInfo = m_pExtFilesMonitor->m_modificationTrackingInfo[entryN];
 
 					if (IsFileExists(curFileEntryInfo.m_filePathToMonitor))
					{
						COleDateTime	currentFileModDateTime = GetFileModficationDate(curFileEntryInfo.m_filePathToMonitor);

						if (currentFileModDateTime != curFileEntryInfo.m_fileModDateTime)
						{
							//the file is modified outside! Update modified file in AllMyNotes
							
							//find attachment doc and position, let's see if it's still there...
							CWnd*	pMainWnd = AfxGetMainWnd();

							if (pMainWnd)
							{
								COPYDATASTRUCT	copyDataStruct;

								copyDataStruct.dwData = ID_COPYDATA_EXT_ATTACHMENT_MODIFICATION;
								copyDataStruct.lpData = (PVOID)&curFileEntryInfo;
								copyDataStruct.cbData = sizeof(curFileEntryInfo);
								if (pMainWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&copyDataStruct))
								{
									//update our records, to do not warn about file modification again (need to re-query mod date for case if the file was modified during user prompt)
									currentFileModDateTime = GetFileModficationDate(curFileEntryInfo.m_filePathToMonitor);
									curFileEntryInfo.m_fileModDateTime = currentFileModDateTime;
								}
							} else
							{
								ASSERT(false); //normally shouldn't happen
							}
						}
 					} else
					{
						//the file is deleted, remove it from the watch list
						m_pExtFilesMonitor->m_modificationTrackingInfo.RemoveAt(entryN);
						monitoredEntriesCount --; //just for case
					}
 				}
 			}

			m_criticalSection.Unlock();
			bNeedToUnlockCriticalSection = false;
			if (!m_bNeedToExit)
			{
				int		monitoredEntriesCount = (int)m_pExtFilesMonitor->m_modificationTrackingInfo.GetSize();

				Sleep(monitoredEntriesCount > 10 ? 5000 : (monitoredEntriesCount > 3 ? 2000 : 1000));
			}
		}
	}
	catch (...)
	{
		ASSERT(false);
		LogAddEntry(_T("Error: General exception caught in check for ext files modification checked")); //tmp: for debugging
		if (bNeedToUnlockCriticalSection)
		{
			try
			{
				m_criticalSection.Unlock();
			}
			catch (...)
			{
				ASSERT(false); //must never happen
			}
		}
	}

	return 0;
}

void CExternalFilesMonitorBackgroundThread::Stop()
{
	//note: thread will be destroyed little after some time, not at once
	LogAddEntry(_T("Info: Ext file modification thread thread Stop request raised")); //tmp: for debugging
	try
	{
		m_bNeedToExit = true;
	}
	catch (...)
	{
		ASSERT(false);
	}
}

/*virtual*/ BOOL CExternalFilesMonitorBackgroundThread::InitInstance()
{
	return TRUE; //CWinThread::InitInstance();
}

/*virtual*/ int CExternalFilesMonitorBackgroundThread::ExitInstance()
{
	return CWinThread::ExitInstance();
}


////////////////////////////////////////////////
//
//	class CExternalFilesMonitor
//


CExternalFilesMonitor::CExternalFilesMonitor() :
	m_pFileModWatcherThread(NULL)
{
}

/*virtual*/ CExternalFilesMonitor::~CExternalFilesMonitor()
{
	DeleeteAllQueuedFiles();
}

void CExternalFilesMonitor::AddFileToBeDeletedLater(const CString& filePath, bool bWipeFileContentAlso /*= false*/)
{
	CAStringArray&	listOfFileNamesArray = bWipeFileContentAlso ? m_listOfFileNamesToDeleteAndEraseContent : m_listOfFileNamesToDelete;

	if (listOfFileNamesArray.FindString(filePath) < 0)
		listOfFileNamesArray.Add(filePath);
}

static void DeleteFilesInTheList(CAStringArray& listOfFileNamesArray, bool bWipeFileContentAlso)
{
	try
	{
		int		filesCount = (int)listOfFileNamesArray.GetSize();

		if (filesCount > 0)
		{
			LogAddEntry(_T("  Info: deleting temporary files..."));

			for (int fileN = filesCount - 1; fileN >= 0; fileN --)
			{
				CString		filePath = listOfFileNamesArray[fileN];

				if (IsFileExists(filePath))
				{
					DWORD	lastErr = 0;

tryToDelFileAgain:
					if (bWipeFileContentAlso)
						WipeFileContent(filePath);

					if (!::DeleteFile(filePath))
					{
						lastErr = GetLastError();
						if (lastErr == ERROR_ACCESS_DENIED ||
							lastErr == ERROR_SHARING_VIOLATION)
						{
#ifdef CLanguageModule
							CString		errMsg = CLanguageModule::GetMainInstance()->GetString(ERR_CANNOT_DELETE_TEMP_FILE);
#else
							CString		errMsg = _T("Error: Cannot delete file ^filename.The file is exclusively locked by another application.");
#endif

							if (!errMsg.Replace(_T("^filename"), filePath))
							{
								ASSERT(false);
								LogAddEntry(_T("Error: Cannot find ^filename tag in the ERR_CANNOT_DELETE_TEMP_FILE string resource."));
							}

#ifdef ShowMessage
							if (ShowMessage(errMsg, MB_ICONWARNING | MB_CYB_RETRYIGNORE) == IDRETRY)
#else
							if (AfxMessageBox(errMsg, MB_ICONWARNING | MB_ABORTRETRYIGNORE) == IDRETRY)
#endif
								goto tryToDelFileAgain;
						} else
						{
							ASSERT(lastErr == ERROR_FILE_NOT_FOUND);
						}
					}
				} else
				{
					//let's presume that it was deleted properly by the user or in some other way and simply skip it... Anyway it makes no sense to bother user about this fact
				}

				listOfFileNamesArray.RemoveAt(fileN);
			}

			LogAddEntry(_T("  Info: deleting temporary files complete..."));
		}
	}
	catch (...)
	{
		ASSERT(false); //must never happen
	}
}

void CExternalFilesMonitor::DeleeteAllQueuedFiles()
{
	DeleteFilesInTheList(m_listOfFileNamesToDelete, false);
	DeleteFilesInTheList(m_listOfFileNamesToDeleteAndEraseContent, true);
}

void CExternalFilesMonitor::AddFileToMonitorExternalModification(const SExternalFileMonitoringInfo& fileToMonitorInfo)
{
	try
	{
		ASSERT(IsFileExists(fileToMonitorInfo.m_filePathToMonitor));

		if (m_pFileModWatcherThread == NULL)
		{
			ASSERT(m_modificationTrackingInfo.GetSize() == 0);
			m_modificationTrackingInfo.Add((SExternalFileMonitoringInfo&)fileToMonitorInfo);

			m_pFileModWatcherThread = new CExternalFilesMonitorBackgroundThread(this);
		} else
		{
			CCriticalSection*	pCriticalSection = m_pFileModWatcherThread->GetCriticalSectionObj();

			ASSERT(pCriticalSection);
			pCriticalSection->Lock(INFINITE);

			int		alreadyMonitoredEntriesCount = (int)m_modificationTrackingInfo.GetSize();

			for (int entryN = 0; entryN < alreadyMonitoredEntriesCount; entryN ++)
			{
				SExternalFileMonitoringInfo&	curFileEntryInfo = m_modificationTrackingInfo[entryN];

				if (curFileEntryInfo.m_attachmentOldFileMD5 == fileToMonitorInfo.m_attachmentOldFileMD5 &&
					curFileEntryInfo.m_ddbDataFileName == fileToMonitorInfo.m_ddbDataFileName &&
					curFileEntryInfo.m_treeItemIDhexStr == fileToMonitorInfo.m_treeItemIDhexStr &&
					curFileEntryInfo.m_attachemntObjectLineInDoc == fileToMonitorInfo.m_attachemntObjectLineInDoc &&
					curFileEntryInfo.m_attachemntObjectPosInLine == fileToMonitorInfo.m_attachemntObjectPosInLine)
				{
					//replace old entry for case if the file was opened more than once
					curFileEntryInfo = fileToMonitorInfo;

					pCriticalSection->Unlock();

					return;
				}
			}
			m_modificationTrackingInfo.Add((SExternalFileMonitoringInfo&)fileToMonitorInfo);
			pCriticalSection->Unlock();
		}
	} catch (...)
	{
		ASSERT(false); //must never happen
	}
}

void CExternalFilesMonitor::UpdateFileInfoToMonitorExternalModification(const CString& filePath, UINT64 newFileSize, const CString& newMD5, COleDateTime newFileModficationDate)
{
	try
	{
		int		entriesCount = (int)m_modificationTrackingInfo.GetSize();

		for (int entryN = 0; entryN < entriesCount; entryN ++)
		{
			SExternalFileMonitoringInfo&	curFileEntryInfo = m_modificationTrackingInfo[entryN];

			if (curFileEntryInfo.m_filePathToMonitor == filePath)
			{
				curFileEntryInfo.m_attachmentOldFileSize = newFileSize;
				curFileEntryInfo.m_attachmentOldFileMD5 = newMD5;
				curFileEntryInfo.m_fileModDateTime = newFileModficationDate;
			}
		}
	} catch (...)
	{
		ASSERT(false); //must never happen
	}
}

void CExternalFilesMonitor::ClearAllExtFilesModificationWatching()
{
	try
	{
		if (m_pFileModWatcherThread)
		{
			m_pFileModWatcherThread->Stop();
			m_pFileModWatcherThread = NULL;
		}
	} catch (...)
	{
		ASSERT(false); //must never happen
		m_pFileModWatcherThread = NULL;
	}
}


/////////////////////////////////////////////////////////////////////////////
// The Global object (for various purposes)
CExternalFilesMonitor g_external_files_monitor;
//
/////////////////////////////////////////////////////////////////////////////
