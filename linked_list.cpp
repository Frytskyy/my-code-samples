//	THIS SOURCE FILE IS THE PROPERTY OF VOLODYMYR FRYTSKYY AND IS NOT TO BE
//	RE-DISTRIBUTED OR USED BY ANY MEANS WHATSOEVER WITHOUT THE EXPRESSED WRITTEN
//	CONSENT OF VOLODYMYR FRYTSKYY.
//
//	CONTACT INFORMATION: fritskiy (At) gmail (dot) com
//
//  The simplest way to launch this code is to simply copy-paste it to https://www.onlinegdb.com/ and press Run button

#include <iostream>
#include <string>
#include <cassert>


// Single Node struct of our linked list
template <typename T>
struct SOneNode 
{
    //Data:
    T           m_data;
    SOneNode*   m_pPrev;
    SOneNode*   m_pNext;

    //Methods:
                SOneNode(const T& value) : m_data(value), m_pPrev(nullptr), m_pNext(nullptr) {}
    SOneNode*   HyperJump(int stepsN)
    {
        SOneNode *pCurrentNode = this;

        if (stepsN > 0) 
        {
            while (pCurrentNode != nullptr && stepsN > 0) 
            {
                pCurrentNode = pCurrentNode->m_pNext;
                stepsN --;
            }
        } else if (stepsN < 0) 
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
template <typename T>
class CLinkedList 
{
private:

    //Data:
    SOneNode<T> *m_pHead;
    SOneNode<T> *m_pTail;
    size_t       m_size;

public:

    //Methods:
                    CLinkedList() : m_pHead(nullptr), m_pTail(nullptr), m_size(0) {}
                    ~CLinkedList() { clear(); }

    SOneNode<T>*    getHead() const { return m_pHead; }
    SOneNode<T>*    getTailNode() const { return m_pTail; }

    size_t          getSize() const { return m_size; }
    bool            isEmpty() const { return m_size == 0; }
    void            clear() 
    {
        SOneNode<T>* current = m_pHead;

        while (current != nullptr) 
        {
            SOneNode<T>* ptrToKill = current;

            current = current->m_pNext;
            delete ptrToKill;
        }
        m_pHead = nullptr;
        m_pTail = nullptr;
        m_size = 0;
    }

    void addFront(const T& value) 
    {
        SOneNode<T>* newNode = new SOneNode<T>(value);
        
        if (isEmpty()) 
        {
            m_pHead = newNode;
            m_pTail = newNode;
        } else 
        {
            newNode->m_pNext = m_pHead;
            m_pHead->m_pPrev = newNode;
            m_pHead = newNode;
        }
        m_size ++;
    }

    void addBack(const T& value)
    {
        SOneNode<T>* newNode = new SOneNode<T>(value);
        
        if (isEmpty()) 
        {
            m_pHead = newNode;
            m_pTail = newNode;
        } else 
        {
            newNode->m_pPrev = m_pTail;
            m_pTail->m_pNext = newNode;
            m_pTail = newNode;
        }
        m_size ++;
    }

    void addValue(int posN, const T& value) 
    {
        if (posN < 0 || posN > static_cast<int>(m_size)) 
        {
            return; // Invalid pos
        }

        if (posN == 0) 
        {
            addFront(value);
            return;
        } else if (posN == static_cast<int>(m_size)) 
        {
            addBack(value);
            return;
        }

        SOneNode<T>* current = m_pHead;
        
        for (int i = 0; i < posN; i ++) 
        {
            current = current->m_pNext;
        }

        SOneNode<T>* newNode = new SOneNode<T>(value);
        
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
            
        SOneNode<T>* temp = m_pHead;

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
        m_size --;
    }

    void removeBack() 
    {
        if (isEmpty()) 
        {
            return;
        }
        
        SOneNode<T>* temp = m_pTail;
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
    list.addFront(1);
    assert(!list.isEmpty());
    assert(list.getSize() == 1);

    list.addBack(2);
    assert(list.getSize() == 2);

    list.addFront(3);
    assert(list.getSize() == 3);

    list.removeBack();
    assert(list.getSize() == 2);

    list.removeFront();
    assert(list.getSize() == 1);

    list.clear();
    assert(list.isEmpty());
    assert(list.getSize() == 0);

    // Test adding value at position
    list.addBack(1);
    list.addBack(2);
    list.addBack(4);
    list.addValue(1, 3);
    assert(list.getSize() == 4);

    SOneNode<int>* pJumpedNode = list.getHead();
    
    assert(pJumpedNode->m_data == 1);
    pJumpedNode = pJumpedNode->m_pNext;
    assert(pJumpedNode->m_data == 3);
    pJumpedNode = pJumpedNode->m_pNext;
    assert(pJumpedNode->m_data == 2);
    pJumpedNode = pJumpedNode->m_pNext;
    assert(pJumpedNode->m_data == 4);

    list.clear();

    // Test the HyperJump function
    list.addBack(1);
    list.addBack(2);
    list.addBack(3);
    list.addBack(4);

    pJumpedNode = list.getHead();
    assert(pJumpedNode);
    pJumpedNode = pJumpedNode->HyperJump(2);
    assert(pJumpedNode);
    assert(pJumpedNode->m_data == 3);
    pJumpedNode = pJumpedNode->HyperJump(-1);
    assert(pJumpedNode);
    assert(pJumpedNode->m_data == 2);
    pJumpedNode = pJumpedNode->HyperJump(5);
    assert(pJumpedNode == nullptr);

    list.clear();

    std::cout << "All unit tests passed successfully!" << std::endl;
}

int main() 
{
    // DBG: Let's do unit tests first
    runUnitTests(); //normally I put unit test in #ifdef DEBUG block, but as we are testing here even in release target I guess it's ok to be relaxed in test assignment

    // Lets add some fun data to our list...
    CLinkedList<std::string> starWarsList;
    starWarsList.addFront("May the Force be with you");
    starWarsList.addBack("Do or do not, there is no try");
    starWarsList.addFront("I find your lack of faith disturbing");
    starWarsList.addValue(1, "Use the Force, Luke");
    
    // Print the list
    std::cout << "Iteration from Head to end" << std::endl;
    
    int                     nodeN = 0;
    SOneNode<std::string>  *current = starWarsList.getHead();
    
    while (current != nullptr) 
    {
        std::cout << "Node " << nodeN ++ << ": '" << current->m_data << "'" << std::endl;
        current = current->m_pNext;
    }
    std::cout << std::endl;
    
    // Print the list in reverse order
    nodeN --;
    std::cout << "Iteration from Tail to beginning" << std::endl;
    current = starWarsList.getTailNode();
    while (current != nullptr) 
    {
        std::cout << "Node " << nodeN -- << ": '" << current->m_data << "'" << std::endl;
        current = current->m_pPrev;
    }
    std::cout << std::endl;
    
    // Demonstrate HyperJump function
    current = starWarsList.getHead();
    SOneNode<std::string>* jumped = current->HyperJump(2);
    if (jumped != nullptr) 
    {
        std::cout << "HyperJump(head + 2): " << jumped->m_data << std::endl;
        
        jumped = jumped->HyperJump(-1);
        if (jumped != nullptr) 
        {
            std::cout << "HyperJump(last jump node-1): " << jumped->m_data << std::endl;
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

