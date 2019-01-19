#ifndef CyclicQueue_h__
#define CyclicQueue_h__

#include "platform.h"

template <typename T>
struct CyclicQueue
{
  ~CyclicQueue();

  void PushBack(T item);
  void PopFront(_Out_ T *pItem);
  void Clear();
  bool Any();

  T *pStart = nullptr;
  T *pFront = nullptr;
  T *pBack = nullptr;
  T *pLast = nullptr;
  size_t capacity = 0, count = 0;
};

template<typename T>
inline CyclicQueue<T>::~CyclicQueue()
{
  fpFreePtr(&pStart);
}

template<typename T>
inline void CyclicQueue<T>::PushBack(T item)
{
  if (pStart == nullptr || pBack + 1 == pFront)
  {
    const size_t newCapacity = std::max(1024ULL, capacity * 2 + 1);
    const size_t offsetStart = pFront - pStart;
    const size_t offsetEnd = pFront - pBack;
    pStart = (T *)realloc(pStart, sizeof(T) * newCapacity);

    if (pStart == nullptr)
      __debugbreak();

    pFront = pStart + offsetStart;
    pBack = pStart + offsetEnd;
    pLast = pStart + capacity;

    if (offsetStart > 0 && offsetStart + count > capacity)
    {
      const size_t wrappedCount = count - (capacity - offsetStart);
      memmove(pLast, pStart, wrappedCount);
      pBack = pLast + wrappedCount;
    }

    capacity = newCapacity;
    pLast = pStart + capacity;
  }

  count++;
  *pBack = item;
  pBack++;

  if (pBack == pLast)
    pBack = pStart;
}

template<typename T>
void CyclicQueue<T>::PopFront(T *pItem)
{
  *pItem = *pFront;
  pFront++;
  count--;
  
  if (pFront == pLast)
    pFront = pStart;
}

template<typename T>
inline void CyclicQueue<T>::Clear()
{
  pFront = pBack = pStart;
  count = 0;
}

template<typename T>
inline bool CyclicQueue<T>::Any()
{
  return pBack != pFront;
}

#endif // CyclicQueue_h__
