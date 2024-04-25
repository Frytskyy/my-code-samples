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
        SOneNode * pCurrentNode = this;
        
        if(stepsN > 0)
        {
            while (pCurrentNode != nullptr && stepsN > 0)
            {
                pCurrentNode = pCurrentNode->m_pNext;
                stepsN --;
            }
        }
        else if(stepsN < 0)
        {
            while (pCurrentNode != nullptr && stepsN < 0)
            {
                pCurrentNode = pCurrentNode->m_pPrev;
                stepsN ++;
            }
        }
        return pCurrentNode;
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
        SOneNode < T > * current = m_pHead;
        while(current != nullptr)
        {
            SOneNode < T > * ptrToKill = current;
            current = current->m_pNext;
            delete ptrToKill;
        }
        m_pHead = nullptr;
        m_pTail = nullptr;
        m_size = 0;
    }
    void          addFront(T * pData)
    {
        SOneNode<T> * newNode = new SOneNode<T>(pData);
        
        if (isEmpty())
        {
            m_pHead = newNode;
            m_pTail = newNode;
        }
        else
        {
            newNode->m_pNext = m_pHead;
            m_pHead->m_pPrev = newNode;
            m_pHead = newNode;
        }
        m_size ++;
    }
    void          addBack(T * pData)
    {
        SOneNode<T> * newNode = new SOneNode<T>(pData);
        if (isEmpty())
        {
            m_pHead = newNode;
            m_pTail = newNode;
        }
        else
        {
            newNode->m_pPrev = m_pTail;
            m_pTail->m_pNext = newNode;
            m_pTail = newNode;
        }
        m_size ++;
    }
    void addValue(int posN, T * pData)
    {
        if (posN < 0 || posN > (int)m_size)
            return; // Invalid pos
            
        if (posN == 0)
        {
            addFront(pData);
            return;
        } else if(posN == (int)m_size)
        {
            addBack(pData);
            return;
        }
        
        SOneNode < T > * current = m_pHead;
        
        for (int i = 0; i < posN; i ++)
        {
            current = current->m_pNext;
        }
        
        SOneNode<T> * newNode = new SOneNode<T>(pData);
        
        newNode->m_pPrev = current->m_pPrev;
        newNode->m_pNext = current;
        current->m_pPrev->m_pNext = newNode;
        current->m_pPrev = newNode;
        m_size ++;
    }
    void removeFront()
    {
        if (isEmpty())
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
        if(isEmpty())
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

    // Test adding value at position
    list.addBack(new int(1));
    list.addBack(new int(2));
    list.addBack(new int(4));
    list.addValue(1, new int(3));
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
    starWarsList.addValue(1, new std::string("Use the Force, Luke"));

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


/* CLI console output:

All unit tests passed successfully!

Iteration from Head to end
Node 0: 'I find your lack of faith disturbing'
Node 1: 'Use the Force, Luke'
Node 2: 'May the Force be with you'
Node 3: 'Do or do not, there is no try'

Iteration from Tail to beginning
Node 3: 'Do or do not, there is no try'
Node 2: 'May the Force be with you'
Node 1: 'Use the Force, Luke'
Node 0: 'I find your lack of faith disturbing'

HyperJump(head + 2): May the Force be with you
HyperJump(last jump node-1): Use the Force, Luke


...Program finished with exit code 0
Press ENTER to exit console.
*/
