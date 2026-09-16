# Operating system for Electrical and Computer Engineering

학부 **ECE359 운영체제** 수업에서 진행한 C 기반 Unix 유틸리티 및 xv6 커널 구현 과제 모음입니다. 파일 입출력부터 시스템 콜, CPU 스케줄링, 커널 스레드와 동기화까지 다룹니다.

## 과제 구성

| 디렉터리 | 주제 | 주요 구현 |
| --- | --- | --- |
| [01-unix-utilities](01-unix-utilities/) | Unix 유틸리티 | RLE 압축 `wzip`, 압축 해제 `wunzip` |
| [02-xv6-scheduling/phase1](02-xv6-scheduling/phase1/) | 시스템 콜 | `getreadcount()`와 read 호출 횟수 추적 |
| [02-xv6-scheduling/phase2](02-xv6-scheduling/phase2/) | Lottery Scheduler | 티켓 기반 CPU 배분, `settickets()`, `getpinfo()` |
| [02-xv6-scheduling/phase3](02-xv6-scheduling/phase3/) | UID 기반 2단계 스케줄링 | 사용자 추첨 후 프로세스 추첨, 사용자별 티켓 상한 |
| [03-kernel-threads](03-kernel-threads/) | 커널 스레드 | `clone()`, `join()`, 사용자 스레드 라이브러리 및 락 |

각 xv6 디렉터리는 해당 단계의 독립적인 소스 트리입니다. 서로 합쳐진 단일 커널이 아니므로 단계별 디렉터리에서 빌드합니다.

## 1. Unix utilities

### 구현

- [`wzip.c`](01-unix-utilities/wzip.c): 여러 입력 파일을 임시 파일로 연결한 뒤, 연속된 바이트를 `(반복 횟수, 바이트)`로 기록합니다. 파일 경계를 넘어 이어지는 동일 문자도 하나의 run으로 처리합니다.
- [`wunzip.c`](01-unix-utilities/wunzip.c): 반복 횟수와 바이트를 읽어 표준 출력으로 복원합니다.
- 저장 형식은 네이티브 `int`와 1바이트 문자입니다. `int` 크기와 endian에 의존하며 일반 ZIP 파일 형식과는 다릅니다.

### 빌드 및 사용

Linux 또는 WSL의 GCC 환경을 기준으로 합니다.

```sh
cd 01-unix-utilities
gcc -Wall -Werror -o wzip wzip.c
gcc -Wall -Werror -o wunzip wunzip.c

./wzip input.txt > compressed.rle
./wunzip compressed.rle > restored.txt
cmp input.txt restored.txt
```

`wzip`은 현재 디렉터리에 `merged_temp.dat`를 만들고 정상 종료 시 삭제하므로 쓰기 권한이 필요합니다. 같은 이름의 파일과 동시 실행에 주의해야 하는 제출본 구현입니다.

**기록 범위:** `wzip.pdf`와 `wunzip.pdf`에는 구현 코드만 있으며 시험 결과는 없습니다. `HW1_unix-utilities.tar`에는 시험 도구와 fixture가 있지만 제출 C 소스는 없어, 두 PDF에서 C 소스를 복원했습니다. 과제 안내의 예시 PASS 출력은 실제 수행 결과로 집계하지 않았습니다.

## 2. xv6 system calls and scheduling

### Phase 1: read 호출 횟수 추적

`syscall()`에서 `SYS_read` 진입을 감지해 전역 `readcount`와 프로세스별 `readid`를 증가시키고, `getreadcount()`로 전역 횟수를 조회합니다. 성공적으로 읽은 바이트 수가 아니라 **read 시스템 콜의 호출 횟수**를 셉니다.

주요 코드: [`syscall.c`](02-xv6-scheduling/phase1/syscall.c), [`sysproc.c`](02-xv6-scheduling/phase1/sysproc.c), [`proc.h`](02-xv6-scheduling/phase1/proc.h), [`test_1.c`](02-xv6-scheduling/phase1/test_1.c), [`test_2.c`](02-xv6-scheduling/phase1/test_2.c).

**보고서 결과** :

```text
test 1: passed
test 2: passed
```

보고서는 단일 CPU를 가정하며, 전역 카운터의 멀티코어 동시 접근에는 별도 동기화가 필요하다고 명시합니다.

### Phase 2: Lottery Scheduler

실행 가능한 프로세스의 티켓을 합산하고, 난수 추첨으로 다음 실행 프로세스를 선택합니다. `settickets()`로 가중치를 설정하고 `getpinfo()`로 티켓·PID·CPU tick 통계를 조회합니다. tick은 타이머 인터럽트에서 집계합니다.

주요 코드: [`proc.c`](02-xv6-scheduling/phase2/proc.c), [`sysproc.c`](02-xv6-scheduling/phase2/sysproc.c), [`trap.c`](02-xv6-scheduling/phase2/trap.c), [`test_lottery.c`](02-xv6-scheduling/phase2/test_lottery.c).

**보고서 측정 결과** :

| 실행 | 30 tickets: ticks / 점유율 | 20 tickets: ticks / 점유율 | 10 tickets: ticks / 점유율 | 총 ticks |
| --- | --- | --- | --- | --- |
| 1 | 5,856 / 50.67% | 3,766 / 32.58% | 1,936 / 16.75% | 11,558 |
| 2 | 5,412 / 49.75% | 3,591 / 33.01% | 1,876 / 17.24% | 10,879 |
| 3 | 6,676 / 50.47% | 4,427 / 33.47% | 2,124 / 16.06% | 13,227 |
| 4 | 5,820 / 49.68% | 3,914 / 33.41% | 1,982 / 16.92% | 11,716 |
| 이론 비율 | 50.00% | 33.33% | 16.67% | - |

수치와 백분율은 원문 표기를 유지했습니다. 표의 점유율은 이론 비율과 약 0.75 percentage point 이내의 차이를 보입니다. 유한 시간의 확률적 측정이며 모든 실행에서 동일한 비율을 보장한다는 뜻은 아닙니다.

### Phase 3: UID 기반 2단계 Lottery Scheduler

1. UID별 티켓 합계에 `USER_CURRENCY = 1000` 상한을 적용해 사용자를 추첨합니다.
2. 선택된 사용자 내부에서 각 프로세스의 원래 티켓 비중으로 다시 추첨합니다.

`getpinfo()`의 UID 정보를 통해 사용자별 통계를 조회합니다. 상한은 사용자 선택 단계에만 적용합니다.

주요 코드: [`proc.c`](02-xv6-scheduling/phase3/proc.c), [`sysproc.c`](02-xv6-scheduling/phase3/sysproc.c), [`param.h`](02-xv6-scheduling/phase3/param.h), [`test_currency.c`](02-xv6-scheduling/phase3/test_currency.c).

**보고서 결과** :

```text
Build complete.
test cur2: passed
test cur3: passed
```

이는 보고서에 보이는 두 시험의 결과입니다. 캡처에 사용된 `test-currency.sh` 외부 채점 도구는 제공된 ZIP에 포함되어 있지 않습니다.

## 3. Kernel threads

- `clone()`으로 부모와 주소 공간을 공유하는 커널 스레드를 생성합니다.
- `join()`으로 종료된 자식 스레드를 기다리고 사용자 스택 주소를 반환합니다.
- `thread_create()` / `thread_join()`으로 사용자 측 생성·정리를 감쌉니다.
- `xchg` 기반 `lock_acquire()` / `lock_release()`로 공유 출력의 동기화를 실험합니다.

주요 코드: [`proc.c`](03-kernel-threads/proc.c), [`ulib.c`](03-kernel-threads/ulib.c), [`sysproc.c`](03-kernel-threads/sysproc.c), [`test_thread.c`](03-kernel-threads/test_thread.c).

**보고서 관찰 결과** :

| 조건 | 캡처에서 확인한 동작 |
| --- | --- |
| 락 사용 | 각 스레드의 `sleep for 100 ticks` 출력이 줄 단위로 분리됨 |
| 락 미사용 | 여러 스레드의 출력 문자가 서로 섞임 |

락 사용 시에도 스레드 실행 순서는 고정되지 않습니다. 보고서의 첫 실행은 1→2→3, 다음 실행은 1→3→2 순서입니다. 이는 상호 배제 관찰 기록이며 전체 스레드 구현의 정확성이나 성능을 검증하는 결과는 아닙니다.

## xv6 빌드 및 실행

환경: **x86 xv6**, 32비트 x86 ELF를 지원하는 GCC/binutils, GNU Make, Perl, QEMU. RISC-V xv6용 빌드 절차가 아닙니다.

```sh
# Lottery scheduler
cd 02-xv6-scheduling/phase2
make qemu-nox CPUS=1
# xv6 셸에서: test_lottery
```

```sh
# Kernel threads: 별도 터미널 또는 저장소 루트에서 시작
cd 03-kernel-threads
make qemu-nox
# xv6 셸에서: test_thread
```

QEMU 종료: `Ctrl-a`, 다음 `x`.
- xv6 과제 보고서 공동 작성자: **박준상, 배선민**. 개인별 구현 기여는 원자료에 구분되어 있지 않습니다.
- xv6 기반 코드의 저작권·기여자 정보는 각 트리의 `LICENSE`와 원본 `README`에 보존했습니다. 전체 xv6 코드를 과제 작성자의 독자 구현으로 주장하지 않습니다.

저장소에는 소스 코드와 빌드에 필요한 파일, 원본 라이선스·README만 포함합니다. 제출 PDF, 압축파일, 실행 바이너리, 디스크 이미지, 오브젝트, 편집기 임시파일 및 기존 Git 이력은 포함하지 않습니다.
