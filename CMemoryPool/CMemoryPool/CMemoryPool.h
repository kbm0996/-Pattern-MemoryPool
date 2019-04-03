/*---------------------------------------------------------------
MemoryPool

�޸� Ǯ Ŭ����.
Ư�� ������(����ü,Ŭ����,����)�� ������ �Ҵ� �� ��������.


- ����

procademy::CMemoryPool<DATA> MemPool(300, FALSE);
DATA *pData = MemPool.Alloc();

pData ���

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
		// �� �� �տ� ���� ��� ����ü.
		/* **************************************************************** */
		// st_BLOCK_NODE�� DATA data;�� ���� ���� �ʰ� ����ó�� ���Ƿ� ���������
		//�޸� ĳ�ö����� �ȸ¾Ƽ� ��Ʈ�� �ɰ����� ��� �� ������ �߻��� �� ����.
		struct st_BLOCK_NODE
		{
			st_BLOCK_NODE*	pNextBlock;
		};

	public:
		//////////////////////////////////////////////////////////////////////////
		// ������, �ı���.
		//
		// Parameters:	(int) �ִ� �� ����. 0 �Է½� FreeList ���
		//				(bool) ������ ȣ�� ����.
		// Return:
		//////////////////////////////////////////////////////////////////////////
		CMemoryPool(int iAllocMax = 0, bool bPlacementNew = false);
		virtual	~CMemoryPool();


		//////////////////////////////////////////////////////////////////////////
		// �� �ϳ��� �Ҵ�޴´�.
		//
		// Parameters: ����.
		// Return: (DATA *) ������ �� ������.
		//////////////////////////////////////////////////////////////////////////
		DATA* Alloc();

		//////////////////////////////////////////////////////////////////////////
		// ������̴� ���� �����Ѵ�.
		//
		// Parameters: (DATA *) �� ������.
		// Return: (BOOL) TRUE, FALSE.
		//////////////////////////////////////////////////////////////////////////
		bool	Free(DATA* pData);

		//////////////////////////////////////////////////////////////////////////
		// ����ȭ
		//
		//////////////////////////////////////////////////////////////////////////
		void Lock() { AcquireSRWLockExclusive(&_Lock); }
		void Unlock() { ReleaseSRWLockExclusive(&_Lock); }

		//////////////////////////////////////////////////////////////////////////
		// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
		//
		// Parameters: ����.
		// Return: (int) �޸� Ǯ ���� ��ü ����
		//////////////////////////////////////////////////////////////////////////
		int		GetAllocSize() { return _lAllocSize; }

		//////////////////////////////////////////////////////////////////////////
		// ���� ������� �� ������ ��´�.
		//
		// Parameters: ����.
		// Return: (int) ������� �� ����.
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
		_bPlacementNew = bPlacementNew;	// ������ ȣ�� ����

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
		// 0�� �ƴ� ���� �� �񱳺��� 0�� ���ϴ� ���� ���ɻ����� ȿ����
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