# 메모리 풀
## 📢 개요
 **메모리 단편화**(Memory Fragmentation)란 힙에 사용 가능한 메모리 공간이 뭉쳐 있지 않고 작게 조각나 있는 상태를 의미한다. 전체 사용 가능한 메모리 양은 많아도 연속해서 사용 가능한 영역이 작아져 성능이 저하된다. new 연산자나 malloc로 메모리를 매번 동적으로 할당, 해제하면 메모리 단편화가 발생할 수 있다. 동적 할당이 잦은 실시간 프로그램에서는 더 자주 발생한다.
  
 **메모리 풀**(Memory Pool)은 메모리를 고정 크기로 미리 할당해둔 것이다. 풀에 들어있는 메모리를 재사용함으로써 메모리 사용 성능을 최적화 할 수 있다. 메모리를 재사용할 때 사용 가능한 메모리 블록을 지속적으로 추적하면 사용 가능한 메모리 블록을 찾느라고 시간을 낭비하지 않아도 된다. 추적하는 방법 중 사용 가능한 메모리 블록의 포인터를 별도의 리스트에 저장하는 것을 **빈칸 목록**(Free List)이라고 한다.
  
  ![capture](https://www.oreilly.com/library/view/unity-2017-game/9781788392365/assets/cd05d279-9a5f-4620-9d02-e44183044217.png)
  
  **figure 1. Memory Fragmentation Example*
   
## 📑 구성
### 💻 CMemoryPool
일반적인 메모리풀
### 💻 CLFMemoryPool
`쓰레드 안전(Thread- safety)`하게 `interlocked 함수(atomic 연산)`를 기반으로 한 `락프리(Lock-free)` 알고리즘을 적용시킨 메모리풀

* **락프리(Lock-free) 알고리즘** : `CAS(Compare And Swap)`하면서 삽입, 삭제가 성공적으로 이루어질 때까지 무한 반복하는 것. SRWLock이나 CriticalSection의 옵션 중 하나인 스핀락(Spinlock)과 유사

### 💻 CLFMemoryPool_TLS
락프리 메모리풀에서 성능을 향상시키기 위해 `TLS(Thread Local Storage)`를 적용시킨 메모리풀
