#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h" 


void
spin_forever(void)
{
  volatile int x = 0;
  while(1) {
    x = (x + 1) % 10; 
  }
}


void
create_user_procs(int uid, int num_children)
{
  if(setuid(uid) < 0) {
    printf(1, "Error: setuid(uid: %d) failed\n", uid);
    exit();
  }
  
  if(uid < 0 || uid >= NUID) {
    printf(1, "Error: Invalid uid %d (must be 0-%d)\n", uid, NUID - 1);
    exit();
  }

  if(settickets(100) < 0) {
     printf(1, "Error: manager settickets(100) failed for uid %d\n", uid);
     exit();
  }

  for(int i = 0; i < num_children; i++) {
    int pid = fork();
    if(pid == 0) { 

      spin_forever(); 
    }
    if (pid < 0) {
      printf(1, "Error: fork failed for uid %d\n", uid);
    }
  }
  
  spin_forever();
}

void
get_all_user_ticks(struct pstat *st, int *ticks_array)
{
  for(int i = 0; i < NUID; i++) {
    ticks_array[i] = 0;
  }
  if(getpinfo(st) < 0) {
    printf(1, "Error: getpinfo failed\n");
    return;
  }
  for(int i = 0; i < NPROC; i++) {
    if(st->inuse[i]) {
      int uid = st->uid[i];
      if(uid >= 0 && uid < NUID) {
        ticks_array[uid] += st->ticks[i];
      }
    }
  }
}


int
main(int argc, char *argv[])
{
  int pid1, pid2, pid3;

  int children1 = 1; 
  int children2 = 4; 
  int children3 = 14; 

  printf(1, "Starting currency scheduler test (NUID=%d, CAP=%d)...\n",
         NUID, USER_CURRENCY);
  
  if(settickets(USER_CURRENCY) < 0) {
      printf(1, "Error: Failed to set tickets for test harness\n");
      exit();
  }

  pid1 = fork();
  if(pid1 == 0) {
    create_user_procs(1, children1);
    exit();
  }

  pid2 = fork();
  if(pid2 == 0) {
    create_user_procs(2, children2);
    exit();
  }

  pid3 = fork();
  if(pid3 == 0) {
    create_user_procs(3, children3);
    exit();
  }
  
  sleep(200); 
  printf(1, "Processes created. Measuring ticks for 10 seconds...\n");

  struct pstat st;
  int ticks_start[NUID];
  int ticks_end[NUID];
  int ticks_delta[NUID];

  get_all_user_ticks(&st, ticks_start);

  sleep(1000); 

  get_all_user_ticks(&st, ticks_end);

  int demand1 = (children1 + 1) * 100; 
  int demand2 = (children2 + 1) * 100; 
  int demand3 = (children3 + 1) * 100; 

  int eff1 = (demand1 > USER_CURRENCY) ? USER_CURRENCY : demand1; 
  int eff2 = (demand2 > USER_CURRENCY) ? USER_CURRENCY : demand2; 
  int eff3 = (demand3 > USER_CURRENCY) ? USER_CURRENCY : demand3; 

  long total_eff_ticks = eff1 + eff2 + eff3; 
  if (total_eff_ticks == 0) total_eff_ticks = 1;

  int pct_exp1 = (eff1 * 100) / total_eff_ticks; 
  int pct_exp2 = (eff2 * 100) / total_eff_ticks; 
  int pct_exp3 = (eff3 * 100) / total_eff_ticks; 

  long total_test_ticks = 0; 

  printf(1, "--------------------------------\n");
  printf(1, "--- Test Results ---\n");
  
  for(int i = 0; i < NUID; i++) {
       ticks_delta[i] = ticks_end[i] - ticks_start[i];
       
       if (i == 1) {
         printf(1, "User 1 (Demand: %d)  ran for: %d ticks\n", demand1, ticks_delta[i]);
         total_test_ticks += ticks_delta[i];
       } else if (i == 2) {
         printf(1, "User 2 (Demand: %d) ran for: %d ticks\n", demand2, ticks_delta[i]);
         total_test_ticks += ticks_delta[i];
       } else if (i == 3) {
         printf(1, "User 3 (Demand: %d) ran for: %d ticks\n", demand3, ticks_delta[i]);
         total_test_ticks += ticks_delta[i];
       } else if (ticks_delta[i] > 0) {
         printf(1, "User %d (System/Other) ran for: %d ticks\n", i, ticks_delta[i]);
       }
  }
  printf(1, "--------------------------------\n");

  if (total_test_ticks == 0) total_test_ticks = 1; 
  
  printf(1, "\nTotal ticks (User 1,2,3): %d\n", (int)total_test_ticks);
  
  printf(1, "Expected Ratio (%d:%d:%d) => %d%% : %d%% : %d%%\n", 
         eff1, eff2, eff3, pct_exp1, pct_exp2, pct_exp3);

  int pct1 = (ticks_delta[1] * 100) / total_test_ticks;
  int pct2 = (ticks_delta[2] * 100) / total_test_ticks;
  int pct3 = (ticks_delta[3] * 100) / total_test_ticks;
  
  printf(1, "Actual Ratio   (Actual Ticks)  => %d%% : %d%% : %d%%\n", pct1, pct2, pct3);
  printf(1, "--------------------------------\n");


  printf(1, "Test finished. Cleaning up processes...\n");
  kill(pid1);
  kill(pid2);
  kill(pid3);
  
  wait();
  wait();
  wait();

  exit();
}