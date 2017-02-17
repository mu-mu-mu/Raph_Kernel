/*
 *
 * Copyright (c) 2017 Raphine Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Author: Liva
 * 
 */

#include "masstree.h"
#include <membench/sync.h>
#include <tty.h>
#include <global.h>
#include <timer.h>
#include <task.h>
#include <spinlock.h>

static void
random_keyval_test(void)
{
	for (unsigned s = 1; s <= 100; s++) {
		masstree_t *tree = masstree_create(NULL);
		size_t n = 10000, i = n;

		while (--i) {
			unsigned long val = rand(), key = rand() % 1000;
			void *pval = (void *)(uintptr_t)val, *pkey = &key;

			masstree_put(tree, pkey, sizeof(key), pval);
			pval = masstree_get(tree, pkey, sizeof(key));

			if ((unsigned long)(uintptr_t)pval != val) {
				gtty->CprintfRaw("%lx, %lx, %zu\n", val, key, n - i);
				abort();
			}
		}
	}
}

static void
seq_del_test(void)
{
	masstree_t *tree = masstree_create(NULL);
	unsigned nitems = 0x1f;
	void *ref;

	for (unsigned i = 0; i < nitems; i++) {
		uint64_t key = i;
		masstree_put(tree, &key, sizeof(uint64_t), (void *)0x1);
	}
	for (unsigned i = 0; i < nitems; i++) {
		uint64_t key = i;
		bool ok = masstree_del(tree, &key, sizeof(uint64_t));
		assert(ok);
	}
	ref = masstree_gc_prepare(tree);
	masstree_gc(tree, ref);
	masstree_destroy(tree);
}


static void
invseq_del_test(void)
{
	masstree_t *tree = masstree_create(NULL);
	unsigned nitems = 0x1f;
	void *ref;

	for (unsigned i = 0; i < nitems; i++) {
		uint64_t key = i;
		masstree_put(tree, &key, sizeof(uint64_t), (void *)0x2);
	}
	for (unsigned i = 0; i < nitems; i++) {
		uint64_t key = nitems - i - 1;
		bool ok = masstree_del(tree, &key, sizeof(uint64_t));
		assert(ok);
	}
	ref = masstree_gc_prepare(tree);
	masstree_gc(tree, ref);
	masstree_destroy(tree);
}

static masstree_t *tree;
static size_t kIterNum = 1000 * 1000;
static int cnt;
static SyncLow sync_1={0};
static SyncLow sync_2={0};
SpinLock *lock;

static void masstree_test_main() {
  uint64_t t1, t2;
  volatile int apicid = cpu_ctrl->GetCpuId().GetApicId();

  if (apicid == 0) {
    cnt = kIterNum;
    tree = masstree_create(NULL);
    lock = new SpinLock;
  }

  sync_1.Do();

  if (apicid == 0) {
    t1 = timer->ReadMainCnt();
  }
    
  while(true) {
    int tmp_cnt = __sync_fetch_and_sub(&cnt, 1);
    if (tmp_cnt < 0) {
      break;
    }
      
    /*
		 * Key range of 4k will create multiple internode
		 * layers with some contention amongst them.
		 */
		uint64_t key = rand() & 0xfff;
		void *val;

		switch (rand() % 3) {
		case 0:
			val = masstree_get(tree, &key, sizeof(key));
			assert(!val || (uintptr_t)val == (uintptr_t)key);
			break;
		case 1:
			masstree_put(tree, &key, sizeof(key),
			    (void *)(uintptr_t)key);
			break;
		case 2:
			masstree_del(tree, &key, sizeof(key));
			break;
		}
  }

  sync_2.Do();

  if (apicid == 0) {
    t2 = timer->ReadMainCnt();
    gtty->CprintfRaw("<%d us> ", ((t2 - t1) * timer->GetCntClkPeriod()) / 1000);
    for (uint64_t key = 0; key <= 0xfff; key++) {
      if (masstree_get(tree, &key, sizeof(key)) != 0) {
        masstree_del(tree, &key, sizeof(key));
      }
    }
    void *ref = masstree_gc_prepare(tree);
    masstree_gc(tree, ref);
    masstree_destroy(tree);
  }
}

void register_masstree_callout() {
  for (int i = 0; i < cpu_ctrl->GetHowManyCpus(); i++) {
    CpuId cpuid(i);
    
    auto task_ = make_sptr(new TaskWithStack(cpuid));
    task_->Init();
    task_->SetFunc(make_uptr(new Function<sptr<TaskWithStack>>([](sptr<TaskWithStack> task){
            masstree_test_main();
            // random_keyval_test();
            // seq_del_test();
            // invseq_del_test();
          }, task_)));
    task_ctrl->Register(cpuid, task_);
  }
}
