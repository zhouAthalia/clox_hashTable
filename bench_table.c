//ch20 q3

#define _POSIX_C_SOURCE 199309L 

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

static double nowNs(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1e9 + ts.tv_nsec;
}

static uint64_t rngState = 88172645463325252ull;
static uint64_t nextRandom(void) {
  rngState ^= rngState << 13;
  rngState ^= rngState >> 7;
  rngState ^= rngState << 17;
  return rngState;
}


static void runCase(const char* label, Value* keys, Value* misses, int n) {
  Table t;
  initTable(&t);

  double t0 = nowNs();
  for (int i = 0; i < n; i++) tableSet(&t, keys[i], NUMBER_VAL(i));
  double insertNs = (nowNs() - t0) / n;

  Value v;
  int hits = 0;
  t0 = nowNs();
  for (int i = 0; i < n; i++) {
    if (tableGet(&t, keys[i], &v)) hits++;
  }
  double hitNs = (nowNs() - t0) / n;

  int falseHits = 0;
  t0 = nowNs();
  for (int i = 0; i < n; i++) {
    if (tableGet(&t, misses[i], &v)) falseHits++;
  }
  double missNs = (nowNs() - t0) / n;

  printf("%-26s n=%-7d cap=%-7d load=%.2f  insert=%6.1f  hit=%6.1f  miss=%6.1f ns/op  hits=%d falseHits=%d\n",
         label, n, t.capacity, (double)t.count / t.capacity,
         insertNs, hitNs, missNs, hits, falseHits);

  freeTable(&t);
}

static void benchIntegers(int n) {
  Value* keys = malloc(sizeof(Value) * n);
  Value* misses = malloc(sizeof(Value) * n);
  for (int i = 0; i < n; i++) {
    keys[i] = NUMBER_VAL((double)i + 1.0);
    misses[i] = NUMBER_VAL((double)(n + i) + 1.0);
  }
  runCase("sequential integers", keys, misses, n);
  free(keys);
  free(misses);
}

static void benchRandomDoubles(int n) {
  Value* keys = malloc(sizeof(Value) * n);
  Value* misses = malloc(sizeof(Value) * n);
  for (int i = 0; i < n; i++) {
    keys[i] = NUMBER_VAL((double)(nextRandom() % 1000000000ull) / 1000.0);
    misses[i] = NUMBER_VAL(-(double)(nextRandom() % 1000000000ull) / 1000.0 - 1.0);
  }
  runCase("random fractional doubles", keys, misses, n);
  free(keys);
  free(misses);
}

static void benchStrings(int n) {
  Value* keys = malloc(sizeof(Value) * n);
  Value* misses = malloc(sizeof(Value) * n);
  char buf[32];
  for (int i = 0; i < n; i++) {
    int len = snprintf(buf, sizeof(buf), "key%d", i);
    keys[i] = OBJ_VAL(copyString(buf, len));
    len = snprintf(buf, sizeof(buf), "absent%d", i);
    misses[i] = OBJ_VAL(copyString(buf, len));
  }
  runCase("interned strings", keys, misses, n);
  free(keys);
  free(misses);
}




static void benchChurn(int rounds, int live) {
  Table t;
  initTable(&t);
  int maxCapacity = 0;

  double t0 = nowNs();
  for (int r = 0; r < rounds; r++) {
    for (int i = 0; i < live; i++) {
      tableSet(&t, NUMBER_VAL((double)(r * live + i)), NUMBER_VAL(i));
    }
    for (int i = 0; i < live; i++) {
      tableDelete(&t, NUMBER_VAL((double)(r * live + i)));
    }
    if (t.capacity > maxCapacity) maxCapacity = t.capacity;
  }
  double ns = (nowNs() - t0) / ((double)rounds * live * 2);

  printf("%-26s rounds=%d live=%d  final cap=%d count=%d max cap=%d  %.1f ns/op (insert+delete)\n",
         "insert/delete churn", rounds, live, t.capacity, t.count,
         maxCapacity, ns);
  freeTable(&t);
}

int main(void) {
  initVM();

  printf("Hash table benchmarks (ch 20 q3, answer by Claude)\n");
  printf("------------------------------------------------------------------------\n");

  benchIntegers(100000);
  benchRandomDoubles(100000);
  benchStrings(100000);

  printf("\nLoad-factor sweep (interned strings; cap is the table's real capacity):\n");
  benchStrings(24577);   
  benchStrings(32768);   
  benchStrings(49152);   

  printf("\n");
  benchChurn(200, 1000);

  freeVM();
  return 0;
}
