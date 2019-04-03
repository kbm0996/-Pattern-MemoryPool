/*---------------------------------------------------------------
MemoryPool

메모리 풀 클래스.
특정 데이터(구조체,클래스,변수)를 일정량 할당 후 나눠쓴다.


- 사용법

procademy::CMemoryPool<DATA> MemPool(300, FALSE);
DATA *pData = MemPool.Alloc();

pData 사용

MemPool.Free(pData);
----------------------------------------------------------------*/
#ifndef  __MEMORY_POOL__
#define  __MEMORY_POOL__
#include <new.h>
#include <Windows.h>

namespace mylib
{
	template <class DATA>
	class CMemoryPool
	{
	private:
		/* **************************************************************** */
		// 각 블럭 앞에 사용될 노드 구조체.
		/* **************************************************************** */
		// st_BLOCK_NODE에 DATA data;를 따로 두지 않고 다음처럼 임의로 계산했을때
		//메모리 캐시라인이 안맞아서 비트가 쪼개져서 얻는 등 문제가 발생할 수 있음.
		struct st_BLOCK_NODE
		{
			st_BLOCK_NODE*	pNextBlock;
		};

	public:
		//////////////////////////////////////////////////////////////////////////
		// 생성자, 파괴자.
		//
		// Parameters:	(int) 최대 블럭 개수. 0 입력시 FreeList 모드
		//				(bool) 생성자 호출 여부.
		// Return:
		//////////////////////////////////////////////////////////////////////////
		CMemoryPool(int iAllocMax = 0, bool bPlacementNew = false);
		virtual	~CMemoryPool();


		//////////////////////////////////////////////////////////////////////////
		// 블럭 하나를 할당받는다.
		//
		// Parameters: 없음.
		// Return: (DATA *) 데이터 블럭 포인터.
		//////////////////////////////////////////////////////////////////////////
		DATA* Alloc();

		//////////////////////////////////////////////////////////////////////////
		// 사용중이던 블럭을 해제한다.
		//
		// Parameters: (DATA *) 블럭 포인터.
		// Return: (BOOL) TRUE, FALSE.
		//////////////////////////////////////////////////////////////////////////
		bool	Free(DATA* pData);

		//////////////////////////////////////////////////////////////////////////
		// 동기화
		//
		//////////////////////////////////////////////////////////////////////////
		void Lock() { AcquireSRWLockExclusive(&_Lock); }
		void Unlock() { ReleaseSRWLockExclusive(&_Lock); }

		//////////////////////////////////////////////////////////////////////////
		// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
		//
		// Parameters: 없음.
		// Return: (int) 메모리 풀 내부 전체 개수
		//////////////////////////////////////////////////////////////////////////
		int		GetAllocSize() { return _lAllocSize; }

		//////////////////////////////////////////////////////////////////////////
		// 현재 사용중인 블럭 개수를 얻는다.
		//
		// Parameters: 없음.
		// Return: (int) 사용중인 블럭 개수.
		//////////////////////////////////////////////////////////////////////////
		int		GetUseSize() { return _lUseSize; }

	private:
		// NODE
		st_BLOCK_NODE*	_pTopNode;
		__int64			_iUnique;
		// MONITOR
		LONG64			_lAllocSize;
		LONG64			_lUseSize;
		// OPTION
		bool			_bFreelist;
		bool			_bPlacementNew;
		SRWLOCK			_Lock;
	};

	template<class DATA>
	inline CMemoryPool<DATA>::CMemoryPool(int iAllocMax, bool bPlacementNew)
	{
		InitializeSRWLock(&_Lock);

		_lUseSize = 0;
		_lAllocSize = iAllocMax;

		_bFreelist = true;
		_bPlacementNew = bPlacementNew;	// 생성자 호출 여부

		if (iAllocMax != 0)
		{
			_bFreelist = false;

			st_BLOCK_NODE *pCurNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE) + sizeof(DATA));
			_pTopNode = pCurNode;

			st_BLOCK_NODE *pPrevNode = pCurNode;
			for (int iCnt = 0; iCnt < iAllocMax - 1; ++iCnt)
			{
				pCurNode = (st_BLOCK_NODE *)malloc(sizeof(st_BLOCK_NODE) + sizeof(DATA));
				pPrevNode->pNextBlock = pCurNode;
				pPrevNode = pCurNode;
			}
		}
	}

	template<class DATA>
	inline CMemoryPool<DATA>::~CMemoryPool()
	{
		st_BLOCK_NODE *pCurNode;
		for (int iCnt = 0; iCnt < _lAllocSize - 1; ++iCnt)
		{
			pCurNode = _pTopNode;
			_pTopNode = _pTopNode->pNextBlock;
			free(pCurNode);
		}

		free(_pTopNode);
	}

	template<class DATA>
	inline DATA * CMemoryPool<DATA>::Alloc()
	{
		st_BLOCK_NODE *pBlock_New;

		++_lUseSize;
		// 0이 아닌 숫자 간 비교보다 0과 비교하는 것이 성능상으로 효율적
		if (_lAllocSize - _lUseSize < 0)
		{
			if (_bFreelist)
			{
				++_lAllocSize;
				pBlock_New = (st_BLOCK_NODE *)malloc(sizeof(st_BLOCK_NODE) + sizeof(DATA));
				pBlock_New->pNextBlock = nullptr;

				new ((DATA *)(pBlock_New + 1)) DATA;

				return (DATA *)(pBlock_New + 1);
			}
			return nullptr;
		}

		pBlock_New = _pTopNode;
		_pTopNode = _pTopNode->pNextBlock;

		if (_bPlacementNew)
			new ((DATA *)(pBlock_New + 1)) DATA;

		return (DATA *)(pBlock_New + 1);
	}

	template<class DATA>
	inline bool CMemoryPool<DATA>::Free(DATA * pData)
	{
		((st_BLOCK_NODE *)pData - 1)->pNextBlock = _pTopNode;
		_pTopNode = ((st_BLOCK_NODE *)pData - 1);
		--_lUseSize;
		return true;
	}
}
#endif