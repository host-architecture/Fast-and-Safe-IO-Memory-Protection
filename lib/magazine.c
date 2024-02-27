#include <linux/magazine.h>
#include <linux/smp.h>
#include <linux/slab.h>
#include <linux/mm.h>

#ifndef assert
#define assert(expr) 	do { \
				if (unlikely(!(expr))) { \
					trace_printk("Assertion failed! %s, %s, %s, line %d\n", \
						   #expr, __FILE__, __func__, __LINE__); \
					panic("ASSERT FAILED: %s (%s)", __FUNCTION__, #expr); \
				} \
			} while (0)

#endif

#define CACHE_MASK      (BIT(INTERNODE_CACHE_SHIFT) - 1)

// page_to_nid - validate copy and mag alloc/free.

static inline void mag_lock(struct mag_allocator *allocator)
{
	spin_lock_bh(&allocator->lock);
}

static inline void mag_unlock(struct mag_allocator *allocator)
{
	spin_unlock_bh(&allocator->lock);
}

static inline u32 mag_pair_count(struct mag_pair *pair)
{
	return pair->count[0] + pair->count[1];
}

static inline struct mag_pair *get_cpu_mag_pair(struct mag_allocator *allocator)
{
	int cpu	= get_cpu();
	int idx = cpu << 1| ((in_softirq()) ? 1 : 0);

	assert(idx < NR_CPUS);
	return &allocator->pair[idx];
}

static inline void swap_mags(struct mag_pair *pair)
{
	pair->mag_ptr[0] ^= pair->mag_ptr[1];
	pair->mag_ptr[1] ^= pair->mag_ptr[0];
	pair->mag_ptr[0] ^= pair->mag_ptr[1];

	pair->count[0] ^= pair->count[1];
	pair->count[1] ^= pair->count[0];
	pair->count[0] ^= pair->count[1];
}

static void *mag_pair_alloc(struct mag_pair *pair)
{
	void *elem;

	if (unlikely(pair->count[0] == 0))
		return NULL;

	--pair->count[0];
	elem = pair->mags[0]->stack[pair->count[0]];

	/* Make sure that, if there are elems in the pair, idx 0 has them*/
	if (pair->count[0] == 0) {
		swap_mags(pair);
	}
	return elem;
}

static void mag_pair_free(struct mag_pair *pair, void *elem)
{
	u32 idx = 0;

	assert(pair->count[0] < MAG_DEPTH || pair->count[1] < MAG_DEPTH);

	if (pair->count[0] == MAG_DEPTH)
		idx = 1;

	pair->mags[idx]->stack[pair->count[idx]] = elem;
	++pair->count[idx];
}

static void mag_allocator_switch_full(struct mag_allocator *allocator, struct mag_pair *pair)
{
	u32 idx = (pair->count[1] == MAG_DEPTH) ? 1 : 0;
	assert(pair->count[idx] == MAG_DEPTH);

	mag_lock(allocator);

	list_add(&pair->mags[idx]->list, &allocator->full_list);
	++allocator->full_count;

	if (allocator->empty_count) {
		pair->mags[idx] = list_entry(allocator->empty_list.next, struct magazine, list);
		list_del_init(allocator->empty_list.next);
		--allocator->empty_count;
	} else {
		void *ptr = kzalloc(sizeof(struct magazine) + L1_CACHE_BYTES -1, GFP_ATOMIC|__GFP_COMP|__GFP_NOWARN);

		pair->mags[idx]	= (void *)ALIGN((u64)ptr, L1_CACHE_BYTES);
	}
	mag_unlock(allocator);

	pair->count[idx] = 0;
}

static void mag_allocator_switch_empty(struct mag_allocator *allocator, struct mag_pair *pair)
{
	int idx = (pair->count[0]) ? 1 : 0;

	mag_lock(allocator);
	if (allocator->full_count) {
		list_add(&pair->mags[idx]->list, &allocator->empty_list);
		++allocator->empty_count;

		pair->mags[idx] = list_entry(allocator->full_list.next, struct magazine, list);
		list_del_init(allocator->full_list.next);
		pair->count[idx] = MAG_DEPTH;
		--allocator->full_count;
	}
	mag_unlock(allocator);
}

void *mag_alloc_elem(struct mag_allocator *allocator)
{
	struct mag_pair	*pair = get_cpu_mag_pair(allocator);
	void 		*elem;

	if (unlikely(mag_pair_count(pair) == 0 )) {
		/*may fail, it's ok.*/
		mag_allocator_switch_empty(allocator, pair);
	}

	elem = mag_pair_alloc(pair);
	put_cpu();
	return elem;
}

void mag_free_elem(struct mag_allocator *allocator, void *elem)
{
	struct mag_pair	*pair = get_cpu_mag_pair(allocator);

	mag_pair_free(pair, elem);

	/* If both mags are full */
	if (unlikely(mag_pair_count(pair) == (MAG_DEPTH << 1))) {
		mag_allocator_switch_full(allocator, pair);
	}
	put_cpu();
}

/*Allocating a new pair of empty magazines*/
static inline void init_mag_pair(struct mag_pair *pair)
{
	int i;
	struct magazine *mag = kzalloc((sizeof(struct magazine) * MAG_COUNT) + L1_CACHE_BYTES -1, __GFP_COMP|__GFP_NOWARN);
	assert(mag);

	mag = (void *)ALIGN((u64)mag, L1_CACHE_BYTES);
	for (i = 0; i < MAG_COUNT; i++) {
		pair->mags[i] = &mag[i];
	}
	assert(pair->mags[0]);
}

void mag_allocator_init(struct mag_allocator *allocator)
{
	int idx;
//	TOD0:  Use percpu asshole
//	assert(!((u64)allocator & CACHE_MASK));
//1.	alloc_struct + pair per core x 2;
//2.	alloc empty mag x2 per idx (init mag_pair, init_mag)
	for (idx = 0 ; idx < num_online_cpus() * 2; idx++) {
		assert(idx < NR_CPUS);
		init_mag_pair(&allocator->pair[idx]);
	}

//3.	init spin lock.
	spin_lock_init(&allocator->lock);

//4. 	init all lists.
	INIT_LIST_HEAD(&allocator->empty_list);
	INIT_LIST_HEAD(&allocator->full_list);
//5. 	init all alloc func. /* Removed untill last_idx removed */
//6.    Counters allocated.
/* Noop */
}
