#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "pstat.h"




extern struct{
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;



int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_setuid(void)
{
  int uid;
  if(argint(0, &uid) < 0)
    return -1;
  
  if(uid < 0 || uid >= NUID) 
    return -1;
    
  myproc()->uid = uid;
  return 0;
}


//phase1 3rd add

int
sys_getreadcount(void) {
	extern int readcount;
	return readcount;
}

int
sys_settickets(void)
{
  int tickets;

  // 인자 값 가져오기
  if (argint(0, &tickets) < 0 || tickets < 1) {
    return -1; // 실패: 유효하지 않은 티켓 값
  }

  acquire(&ptable.lock); // ptable 접근 보호

  // 현재 프로세스 가져오기
  struct proc *curproc = myproc();

  // 티켓 값 설정
  curproc->tickets = tickets;

  release(&ptable.lock); // 락 해제
  return 0; // 성공
}

int
sys_getpinfo(void)
{
  struct pstat *pstat;

  // 사용자 공간의 포인터 가져오기
  if (argptr(0, (void*)&pstat, sizeof(*pstat)) < 0) {
    return -1; // 실패: 잘못된 포인터
  }

  acquire(&ptable.lock); // ptable 접근 보호

  for (int i = 0; i < NPROC; i++) {
    struct proc *p = &ptable.proc[i];
    pstat->inuse[i] = (p->state != UNUSED); // 프로세스 사용 여부
    pstat->tickets[i] = p->tickets; // 티켓 수
    pstat->pid[i] = p->pid; // 프로세스 ID
    pstat->ticks[i] = p->ticks; // 실행된 ticks
    pstat->uid[i] = p->uid;
  }

  release(&ptable.lock); // 락 해제
  return 0; // 성공
}
