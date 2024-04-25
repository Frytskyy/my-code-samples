//  THIS SOURCE FILE IS THE PROPERTY OF VOLODYMYR FRYTSKYY AND IS NOT TO BE
//  RE-DISTRIBUTED OR USED BY ANY MEANS WHATSOEVER WITHOUT THE EXPRESSED WRITTEN
//  CONSENT OF VOLODYMYR FRYTSKYY.
//
//  CONTACT INFORMATION: fritskiy(At) gmail(dot) com
//
//  The simplest way to launch this code is to simply copy-paste it to https://www.onlinegdb.com/ and press Run button

#include <iostream>
#include <string>
#include <cassert>

// Single Node struct of our linked list
template <typename T> struct SOneNode
{
    //Data:
    SOneNode * m_pPrev = nullptr;
    SOneNode * m_pNext = nullptr;
    T        * m_pData = nullptr;
    bool       m_bAutoDeallocateData = false;
    
    //Methods:
    SOneNode(T * pData, bool bAutoDeallocateData = true): m_pData(pData), m_pPrev(nullptr), m_pNext(nullptr), m_bAutoDeallocateData(bAutoDeallocateData) {}
    ~SOneNode() { if(m_bAutoDeallocateData) { if(m_pData) delete m_pData; } }
    
    SOneNode * HyperJump(int stepsN)
    {
        SOneNode * pCurNode = this;
        
        if(stepsN > 0)
        {
            while (pCurNode != nullptr && stepsN > 0)
            {
                pCurNode = pCurNode->m_pNext;
                stepsN --;
            }
        }
        else if(stepsN < 0)
        {
            while (pCurNode != nullptr && stepsN < 0)
            {
                pCurNode = pCurNode->m_pPrev;
                stepsN ++;
            }
        }
        
        return pCurNode;
    }
};

// Linked List class itself :)
template <typename T> class CLinkedList
{
private:
    
    //Data:
    SOneNode<T> * m_pHead;
    SOneNode<T> * m_pTail;
    size_t        m_size;
        
public:
    
    //Methods:
                  CLinkedList(): m_pHead(nullptr), m_pTail(nullptr), m_size(0) {}
                  ~CLinkedList() { clear(); }
    
    SOneNode<T> * getHead() const { return m_pHead; }
    SOneNode<T> * getTailNode() const { return m_pTail; }
    size_t        getSize() const { return m_size; }
    bool          isEmpty() const { return m_size == 0; }
    void          clear()
    {
        SOneNode<T>*    pCurNode = m_pHead;
        
        while (pCurNode)
        {
            SOneNode<T> * ptrToKill = pCurNode;
            
            pCurNode = pCurNode->m_pNext;
            delete ptrToKill;
        }
        m_pHead = nullptr;
        m_pTail = nullptr;
        m_size = 0;
    }
    SOneNode<T>* addFront(T* pData, bool bAutoDeallocateData = true)
    {
        SOneNode<T>* pNewNode = new SOneNode<T>(pData, bAutoDeallocateData);
        
        if (isEmpty())
        {
            m_pHead = pNewNode;
            m_pTail = pNewNode;
        } else
        {
            pNewNode->m_pNext = m_pHead;
            m_pHead->m_pPrev = pNewNode;
            m_pHead = pNewNode;
        }
        m_size ++;
        
        return pNewNode;
    }
    SOneNode<T>* addBack(T* pData, bool bAutoDeallocateData = true)
    {
        SOneNode<T>* pNewNode = new SOneNode<T>(pData, bAutoDeallocateData);
        
        if (isEmpty())
        {
            m_pHead = pNewNode;
            m_pTail = pNewNode;
        } else
        {
            pNewNode->m_pPrev = m_pTail;
            m_pTail->m_pNext = pNewNode;
            m_pTail = pNewNode;
        }
        m_size ++;
        
        return pNewNode;
    }
    SOneNode<T>* insertInside(int posN, T* pData, bool bAutoDeallocateData = true)
    {
        if (posN < 0 || posN > (int)m_size)
            return nullptr; // Invalid pos
            
        if (posN == 0)
        {
            return addFront(pData, bAutoDeallocateData);
        } else if(posN == (int)m_size)
        {
            return addBack(pData, bAutoDeallocateData);
        }
        
        SOneNode<T>* pCurNode = m_pHead;
        
        for (int i = 0; i < posN; i ++)
        {
            pCurNode = pCurNode->m_pNext;
        }
        
        SOneNode<T>* pNewNode = new SOneNode<T>(pData, bAutoDeallocateData);
        
        pNewNode->m_pPrev = pCurNode->m_pPrev;
        pNewNode->m_pNext = pCurNode;
        pCurNode->m_pPrev->m_pNext = pNewNode;
        pCurNode->m_pPrev = pNewNode;
        m_size ++;
        
        return pNewNode;
    }
    void removeFront()
    {
        if (!m_pHead)
            return;
            
        SOneNode<T> * temp = m_pHead;
        
        if (m_pHead == m_pTail)
        {
            m_pHead = nullptr;
            m_pTail = nullptr;
        } else
        {
            m_pHead = m_pHead->m_pNext;
            m_pHead->m_pPrev = nullptr;
        }
        delete temp;
        m_size--;
    }
    void removeBack()
    {
        if(!m_pTail)
            return;
            
        SOneNode<T> * temp = m_pTail;
        
        if (m_pHead == m_pTail)
        {
            m_pHead = nullptr;
            m_pTail = nullptr;
        } else
        {
            m_pTail = m_pTail->m_pPrev;
            m_pTail->m_pNext = nullptr;
        }
        delete temp;
        m_size --;
    }
};

// Unit tests
void runUnitTests()
{
    CLinkedList<int> list;
    
    // Test initial state
    assert(list.isEmpty());
    assert(list.getSize() == 0);

    // Test adding and removing nodes
    list.addFront(new int(1));
    assert(!list.isEmpty());
    assert(list.getSize() == 1);
    list.addBack(new int(2));
    assert(list.getSize() == 2);
    list.addFront(new int(3));
    assert(list.getSize() == 3);
    list.removeBack();
    assert(list.getSize() == 2);
    list.removeFront();
    assert(list.getSize() == 1);
    list.clear();
    assert(list.isEmpty());
    assert(list.getSize() == 0);

    // Test adding node at position
    list.addBack(new int(1));
    list.addBack(new int(2));
    list.addBack(new int(4));
    list.insertInside(1, new int(3));
    assert(list.getSize() == 4);
    
    SOneNode<int> *pJumpedNode = list.getHead();
    
    assert(*pJumpedNode->m_pData == 1);
    pJumpedNode = pJumpedNode->m_pNext;
    assert(*pJumpedNode->m_pData == 3);
    pJumpedNode = pJumpedNode->m_pNext;
    assert(*pJumpedNode->m_pData == 2);
    pJumpedNode = pJumpedNode->m_pNext;
    assert(*pJumpedNode->m_pData == 4);
    list.clear();

    // Test the HyperJump function
    list.addBack(new int(1));
    list.addBack(new int(2));
    list.addBack(new int(3));
    list.addBack(new int(4));
    pJumpedNode = list.getHead();
    assert(pJumpedNode);
    pJumpedNode = pJumpedNode->HyperJump(2);
    assert(pJumpedNode);
    assert(*pJumpedNode->m_pData == 3);
    pJumpedNode = pJumpedNode->HyperJump(-1);
    assert(pJumpedNode);
    assert(*pJumpedNode->m_pData == 2);
    pJumpedNode = pJumpedNode->HyperJump(5);
    assert(pJumpedNode == nullptr);
    list.clear();

    std::cout << "All unit tests passed successfully!" << std::endl << std::endl;
}

int main()
{
    // DBG: Let's do unit tests first
    runUnitTests(); //normally I put unit test in #ifdef DEBUG block, but as we are testing here even in release target I guess it's ok to be relaxed in test assignment
    
    // Lets add some fun data to our list...
    CLinkedList<std::string> starWarsList;

    starWarsList.addFront(new std::string("May the Force be with you"));
    starWarsList.addBack(new std::string("Do or do not, there is no try"));
    starWarsList.addFront(new std::string("I find your lack of faith disturbing"));
    starWarsList.insertInside(1, new std::string("Use the Force, Luke"));

    // Print the list
    std::cout << "Iteration from Head to end" << std::endl;

    int                     nodeN = 0;
    SOneNode<std::string> * pNode = starWarsList.getHead();
    while (pNode != nullptr)
    {
        std::cout << "Node " << nodeN ++ << ": '" << *pNode->m_pData << "'" << std::endl;
        pNode = pNode->m_pNext;
    }
    std::cout << std::endl;
    
    // Print the list in reverse order
    nodeN --;
    std::cout << "Iteration from Tail to beginning" << std::endl;
    pNode = starWarsList.getTailNode();
    while (pNode != nullptr)
    {
        std::cout << "Node " << nodeN -- << ": '" << *pNode->m_pData << "'" << std::endl;
        pNode = pNode->m_pPrev;
    }
    std::cout << std::endl;
    
    // Demonstrate HyperJump function
    pNode = starWarsList.getHead();
    
    SOneNode<std::string> *jumped = pNode->HyperJump(2);
    
    if (jumped != nullptr)
    {
        std::cout << "HyperJump(head + 2): " << *jumped->m_pData << std::endl;
        jumped = jumped->HyperJump(-1);
        if (jumped != nullptr)
        {
            std::cout << "HyperJump(last jump node-1): " << *jumped->m_pData << std::endl;
        } else
        {
            std::cout << "HyperJump(last jump node-1): Out of bounds" << std::endl;
        }
    } else
    {
        std::cout << "HyperJump(head + 2): Out of bounds" << std::endl;
    }
    
    return 0;
}

