//////////////////////////////////////////////////////////////////////////
// A fast memory pool and smart pointer classes
//////////////////////////////////////////////////////////////////////////

#ifndef __INCLUDE_GUARD_9893529A_CE41_4CD6_8BDF_F69984967BA5
#define __INCLUDE_GUARD_9893529A_CE41_4CD6_8BDF_F69984967BA5
#ifdef _MSC_VER
	#pragma once
#endif

#include "defs.h"

//A hash table implementation with, with no empty checks
//This class is for internal use only. Use at your own risk
template<class T, size_t ta_size, size_t ta_noKey>
class HashTable
{
	struct Element
	{
		size_t key;
		T data;

		Element() : key(ta_noKey)
		{}
	};

	Element m_data[ta_size];

	_FORCE_INLINE size_t getIndex(size_t _key)
	{
		size_t startIdx = _key % ta_size;
		size_t idx = startIdx;
		if(m_data[idx].key != _key)
		{
			//Table should never get full !!!!
			while(m_data[idx].key != _key && m_data[idx].key != ta_noKey)
				idx = (idx + 1) % ta_size;


			if(m_data[idx].key == ta_noKey)
				m_data[idx].key = _key;
		}

		return idx;
	}

public:
	_FORCE_INLINE HashTable() {/*m_data = new Element[ta_size];*/}
	_FORCE_INLINE T& operator[] (size_t _key) { return m_data[getIndex(_key)].data;}
	_FORCE_INLINE const T& operator[] (size_t _key) const { return m_data[getIndex(_key)].data;}
	_FORCE_INLINE ~HashTable() { /*delete m_data;*/ }
};

//Memory pool for fast memory allocation and deallocation.
//For internal use. Use at your own risk.
template<size_t ta_pageSize, size_t ta_aligment, size_t ta_maxDanglingPages>
class MemoryPool
{
	struct Page
	{
		MemoryPool *owner;
		size_t indexInOwnersList;

		std::vector<byte> pageContents;
		std::vector<size_t> freeList;
		size_t freeListEnd, lastUsedSlot;
		size_t objectSize;

		Page() {}
		Page(
			size_t _objectSize, MemoryPool *_owner, size_t _indexInOwnerList)
			: pageContents(ta_pageSize * _objectSize), freeList(ta_pageSize), 
			lastUsedSlot(0), freeListEnd(0), owner(_owner), indexInOwnersList(_indexInOwnerList),
			objectSize(_objectSize)
		{}

		void* allocate()
		{
			size_t retPlace;
			if(freeListEnd > 0)
				retPlace = freeList[--freeListEnd];
			else
				retPlace = lastUsedSlot++;

			return &pageContents.front() + objectSize * retPlace;
		}

		void free(void * _ptr)
		{
			size_t index = ((byte*)_ptr - &pageContents.front()) / objectSize;
			freeList[freeListEnd++] = index;
		}

		bool isFull() const {return freeListEnd == 0 && lastUsedSlot == ta_pageSize;}
		bool isBlank() const {return freeListEnd == lastUsedSlot;}
		bool wasEmptyBeforeAlloc() const {return freeListEnd == lastUsedSlot - 1;}
		bool wasFullBeforeDealloc() const {return freeListEnd == 1 && lastUsedSlot == ta_pageSize;}
		bool hasSlotsLeft() const {return freeListEnd > 0 || lastUsedSlot < ta_pageSize;}
	};

	struct PageList
	{
		std::vector<Page *> pages;
		size_t firstPageWithFreeData;
		size_t danglingPages;

		PageList()
		{
			firstPageWithFreeData = 0;
			danglingPages = 0;
		}
	};

	//stdext::hash_map<size_t, PageList> m_pages;
	HashTable<PageList, (1 << 16), (size_t)-1> m_pages;

public:

	MemoryPool() {}

	void* allocate(size_t _size)
	{
		size_t realAligment = std::max(_ALIGNOF(Page*), ta_aligment);
		size_t realSize = realAligment + _size;
		//realAligment should 
		size_t key = realSize;

		PageList &list = m_pages[key];
		_ASSERT(list.firstPageWithFreeData <= list.pages.size());
		if(list.firstPageWithFreeData == list.pages.size())
		{
			list.pages.push_back(new Page(realSize, this, list.firstPageWithFreeData));
			list.danglingPages++;
		}

		Page &page = *list.pages[list.firstPageWithFreeData];
		void* ret = (byte*)page.allocate() + realAligment;
		*((Page* *)ret - 1) = (Page*)&page;
		
		//The page is full. No more allocations possible
		if(page.isFull())
			list.firstPageWithFreeData++;
		else if(page.wasEmptyBeforeAlloc())
			//The page was totally empty and is now only partially empty
			list.danglingPages --;

		return ret;
	}

	static void free(void *_data)
	{
		Page *page = *((Page**)_data - 1);

		page->free(_data);
		if(page->wasFullBeforeDealloc() || page->isBlank())
		{
			MemoryPool *pool = page->owner;
			PageList &list = pool->m_pages[page->objectSize];

			size_t pageToSwapWith;
			if(!page->isBlank())
			{
				_ASSERT(list.firstPageWithFreeData > 0);
				pageToSwapWith = --list.firstPageWithFreeData;
			}
			else
			{
				if(list.danglingPages >= ta_maxDanglingPages)
				{
					delete list.pages.back();
					list.pages.resize(list.pages.size() - 1);
					list.danglingPages--;
				}

				pageToSwapWith = list.pages.size() - list.danglingPages - 1;
				list.danglingPages++;
			}

			std::swap(list.pages[page->indexInOwnersList], list.pages[pageToSwapWith]);
			std::swap(list.pages[page->indexInOwnersList]->indexInOwnersList, 
				list.pages[pageToSwapWith]->indexInOwnersList);
		}
	}
};


//Derive your classes from this class if you need
//	high performance frequent allocations/deallocations
class FastAllocObjectBase
{

	typedef MemoryPool<8192, 16, 4> t_memoryPool;
	static t_memoryPool& getMemoryPool()
	{
		static _THREAD_LOCAL t_memoryPool* pool = NULL;
		if (pool == NULL)
			pool = new t_memoryPool;

		return *pool;
	};

public:

	void *operator new(size_t _size)
	{
		return getMemoryPool().allocate(_size);
	}
	void operator delete (void * _obj)
	{
		getMemoryPool().free(_obj);
	}
};

//A base for reference counted objects. A reference counted object
//	is deleted automatically, once it's reference count drops to 0.
//Derived from FastAllocObjectBase and thus suited for frequent
//	allocations and deallocations
struct RefCntBase : public FastAllocObjectBase
{
	long m_refCnt;
public:
	RefCntBase()
		: m_refCnt(0)
	{}

	RefCntBase(const RefCntBase&)
		: m_refCnt(0)
	{}

	void addRef()
	{
#ifdef _OPENMP
		_InterlockedIncrement(&m_refCnt);
#else
		m_refCnt++;
#endif 
	}

	long release()
	{
#ifdef _OPENMP
		long ret = _InterlockedDecrement(&m_refCnt);
#else
		long ret = --m_refCnt;
#endif
		if(m_refCnt == 0)
			delete this;

		return ret;
	}

	RefCntBase& operator=(const RefCntBase& _other)
	{
		return *this;
	}

	virtual ~RefCntBase()
	{
	}
};


//A smart pointer class. The underlying type of the smart pointer should
//	be a descendant of RefCntBase. You can work with smart pointers the same
//	way you work with normal pointers. Allocate the data of a smart pointer with new.
//	If you need to create a smart pointer to an object allocated through different means
//	(on the stack for ex.), do not forget to addRef the object before assigning it to the smart
//	pointer. 
//Also, be careful not to create "cycles" of smart pointers (i.e. two objects
//	that reference each other by smart pointers). Garbage collection through smart
//	pointers with reference counting can not handle cycles.
//You can read a lot more about smart pointers here:
//	http://en.wikipedia.org/wiki/Smart_pointer
//	http://www.informit.com/articles/article.aspx?p=25264
//	Boost's implementation of a smart (intrusive) pointer:
//		http://www.boost.org/doc/libs/1_36_0/libs/smart_ptr/intrusive_ptr.html
template<class T>
class SmartPtr
{
	T *m_pointer;

	template<class ta_ptr>
	SmartPtr& assign (ta_ptr* _data)
	{
		if(m_pointer != NULL)
			m_pointer->release();

		m_pointer = static_cast<T*>(_data);
		if(m_pointer != NULL)
			m_pointer->addRef();
		return *this;
	}

	template<class ta_other> friend class SmartPtr;

public:
	SmartPtr() : m_pointer(NULL) {}
	
	SmartPtr(T* _data) : m_pointer(_data)
	{
		if(m_pointer != NULL)
			m_pointer->addRef();
	}

	template<class ta_other>
	SmartPtr(const SmartPtr<ta_other>& _other) : m_pointer(NULL) { assign(_other.m_pointer);}

	SmartPtr(const SmartPtr& _other) : m_pointer(NULL) { assign(_other.m_pointer);}

	template<class ta_other>
	SmartPtr& operator= (const SmartPtr<ta_other>& _other)
	{ return assign(_other.m_pointer); }

	SmartPtr& operator= (const SmartPtr& _other)
	{ return assign(_other.m_pointer); }

	T* operator-> () { return m_pointer; }
	const T* operator-> () const { return m_pointer; }

	T& operator* () { return *m_pointer; }
	const T& operator* () const { return *m_pointer; }

	T* data() { return m_pointer; };
	const T* data() const { return m_pointer; };

	~SmartPtr()
	{
		if(m_pointer != NULL)
			m_pointer->release();
	}
};



#endif //__INCLUDE_GUARD_9893529A_CE41_4CD6_8BDF_F69984967BA5
