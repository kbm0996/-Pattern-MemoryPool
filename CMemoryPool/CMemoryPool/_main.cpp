#include <conio.h>
#include <Windows.h>
#include <process.h>
#include <cstdio>
#pragma comment(lib,"winmm.lib")
#include "CMemoryPool.h"

// �׽�Ʈ ����
//
// - �Ҵ�� �޸𸮸� �� �Ҵ� �ϴ°� ?
// - �߸��� �޸𸮸� �Ҵ� �ϴ°� ?

#define THREAD_CNT 10
#define ALLOC_CNT 10000

struct st_TEST_DATA
{
	volatile LONG64	lData;
	volatile LONG64	lCount;
};

LONG64 g_lAllocTps;
LONG64 g_lFreeTps;

bool g_bShutdown = false;

mylib::CMemoryPool<st_TEST_DATA> *g_pMemPool;
bool Control();
void PrintState();
unsigned int __stdcall WorkerThread(LPVOID pParam);

void main()
{
	g_pMemPool = new mylib::CMemoryPool<st_TEST_DATA>(0);

	HANDLE hWorkerThread[THREAD_CNT];
	for (int iCnt = 0; iCnt < THREAD_CNT; ++iCnt)
	{
		hWorkerThread[iCnt] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, NULL, FALSE, NULL);
	}

	while (1)
	{
		if (!Control())
		{
			g_bShutdown = true;
			break;
		}


		PrintState();

		Sleep(1);
	}

	WaitForMultipleObjects(THREAD_CNT, hWorkerThread, TRUE, INFINITE);
	for (int iCnt = 0; iCnt < THREAD_CNT; ++iCnt)
	{
		CloseHandle(hWorkerThread[iCnt]);
	}
}

bool Control()
{
	static bool bControl = false;
	WCHAR chKey;

	if (_kbhit())
	{
		chKey = _getwch();
		switch (chKey)
		{
		case L'u':
		case L'U':
			if (bControl == false)
			{
				bControl = true;
				wprintf(L"[ Control Mode ] \n");
				wprintf(L"Press  L	- Key Lock \n");
				wprintf(L"Press  Q	- Quit \n");
			}
			break;
		case L'l':
		case L'L':
			if (bControl == true)
			{
				bControl = false;
				wprintf(L"[ Control Mode ] \n");
				wprintf(L"Press  U	- Key Unlock \n");
				wprintf(L"Press  Q	- Quit \n");
			}
			break;
		case L'q':
		case L'Q':
			if (bControl == true)
				return false;
			break;
		}
	}

	return true;
}

void PrintState()
{
	static DWORD dwSystemTick = timeGetTime();
	DWORD dwCurrentTick = timeGetTime();

	// �������� �ݵ�� �˻��ؾ��ϴ� �ּ����� �׸�
	if (dwCurrentTick - dwSystemTick >= 500)
	{
		system("cls");
		wprintf(L"===========================================\n");
		wprintf(L"Alloc TPS		: %lld \n", g_lAllocTps);
		wprintf(L"Free TPS		: %lld \n\n", g_lFreeTps);

		wprintf(L"MemPool Use Size	: %d \n", g_pMemPool->GetUseSize());
		wprintf(L"MemPool Alloc Size	: %d \n", g_pMemPool->GetAllocSize());
#ifdef __LF_QUEUE__
		wprintf(L"Queue Alloc Size	: %d \n", g_pQueue->GetUseSize());
#endif

		dwSystemTick = dwCurrentTick;
		g_lAllocTps = g_lFreeTps = 0;
	}
}

// �������� �����忡�� ���������� Alloc �� Free �� �ݺ������� ��
// ��� �����ʹ� 0x0000000055555555 ���� �ʱ�ȭ �Ǿ� ����.

// ������ 100,000 ���� st_TEST_DATA  �����͸� �޸�Ǯ�� �־��. 
//  ��� �����ʹ� Data - 0x0000000055555555 / Cound - 0 �ʱ�ȭ.

// ������ ������� �Ʒ��� �ൿ�� ���� (������ 10��)

// 0. Alloc (10000��)
// 1. 0x0000000055555555 �� �´��� Ȯ��.
// 2. Interlocked + 1 (Data + 1 / Count + 1)
// 3. �ణ���
// 4. ������ 0x0000000055555556 �� �´��� (Count == 1) Ȯ��.
// 5. Interlocked - 1 (Data - 1 / Count - 1)
// 6. �ణ���
// 7. 0x0000000055555555 �� �´��� (Count == 0) Ȯ��.
// 8. Free

unsigned int __stdcall WorkerThread(LPVOID pParam)
{
	int iCnt;
	st_TEST_DATA *pData[ALLOC_CNT];
	st_TEST_DATA *pResultData;
	while (!g_bShutdown)
	{
		// 0. Alloc (10000��)
		for (iCnt = 0; iCnt < ALLOC_CNT; ++iCnt)
		{
			g_pMemPool->Lock();
			pData[iCnt] = g_pMemPool->Alloc();
			g_pMemPool->Unlock();

			pData[iCnt]->lCount = 0;
			pData[iCnt]->lData = 0x0000000055555555;

			InterlockedIncrement64(&g_lAllocTps);
		}

		// 1. 0x0000000055555555 �� �´��� Ȯ��.
		for (iCnt = 0; iCnt < ALLOC_CNT; ++iCnt)
		{
			if (pData[iCnt]->lData != 0x0000000055555555)
			{
				int *p = nullptr;
				*p = 0;
			}
		}

		// 2. Interlocked + 1 (Data + 1 / Count + 1)
		for (iCnt = 0; iCnt < ALLOC_CNT; ++iCnt)
		{
			InterlockedIncrement64(&pData[iCnt]->lCount);
			InterlockedIncrement64(&pData[iCnt]->lData);
		}

		// 3. �ణ���
		Sleep(1);

		// 4. ������ 0x0000000055555556 �� �´��� (Count == 1) Ȯ��.
		for (iCnt = 0; iCnt < ALLOC_CNT; ++iCnt)
		{
			if (pData[iCnt]->lData != 0x0000000055555556 || pData[iCnt]->lCount != 1)
			{
				int *p = nullptr;
				*p = 0;
			}
		}

		// 5. Interlocked - 1 (Data - 1 / Count - 1)
		for (iCnt = 0; iCnt < ALLOC_CNT; ++iCnt)
		{
			InterlockedDecrement64(&pData[iCnt]->lCount);
			InterlockedDecrement64(&pData[iCnt]->lData);
		}

		// 6. �ణ���
		Sleep(1);

		// 7. 0x0000000055555555 �� �´��� (Count == 0) Ȯ��.
		for (iCnt = 0; iCnt < ALLOC_CNT; ++iCnt)
		{
			if (pData[iCnt]->lData != 0x0000000055555555 || pData[iCnt]->lCount != 0)
			{
				int *p = nullptr;
				*p = 0;
			}
		}

		// 8. Free
		for (iCnt = 0; iCnt < ALLOC_CNT; ++iCnt)
		{
			g_pMemPool->Lock();
			g_pMemPool->Free(pData[iCnt]);
			g_pMemPool->Unlock();

			InterlockedIncrement64(&g_lFreeTps);
		}
	}
	return 0;
}

//#include "CMemoryPool.h"
//
//void main()
//{
//	int *pData;
//	int *pData2;
//	int *pData3;
//	int *pData4;
//	int *pData5;
//	int *pData6;
//	int *pData7;
//
//	mylib::CMemoryPool<int> MemPool;
//
//	pData = MemPool.Alloc();
//	pData[0] = 1;
//
//	pData2 = MemPool.Alloc();
//	pData2[0] = 2;
//
//	pData3 = MemPool.Alloc();
//	pData3[0] = 3;
//
//	MemPool.Free(pData3);
//	MemPool.Free(pData);
//
//	pData4 = MemPool.Alloc();
//	pData4[0] = 4;
//
//	pData5 = MemPool.Alloc();
//	pData5[0] = 5;
//
//	MemPool.Free(pData4);
//
//	pData6 = MemPool.Alloc();
//	pData6[0] = 6;
//
//	pData7 = MemPool.Alloc();
//	pData7[0] = 7;
//
//	pData = MemPool.Alloc();
//	pData[0] = 8;
//
//	//MemPool.Free(pData);
//	MemPool.Free(pData2);
//	//MemPool.Free(pData3);
//	//MemPool.Free(pData4);
//	MemPool.Free(pData5);
//	MemPool.Free(pData6);
//	MemPool.Free(pData7);
//}