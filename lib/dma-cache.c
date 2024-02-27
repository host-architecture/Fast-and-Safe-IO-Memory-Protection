#include <linux/dma-cache.h>
#include <linux/dma-mapping.h>

#ifndef assert
#define assert(expr) 	do { \
				if (unlikely(!(expr))) { \
					trace_printk("Assertion failed! %s, %s, %s, line %d\n", \
						   #expr, __FILE__, __func__, __LINE__); \
					panic("ASSERT FAILED: %s (%s)", __FUNCTION__, #expr); \
				} \
			} while (0)

#endif

#define trace(...)

static inline enum dma_data_direction idx2perm(u64 idx)
{
	assert(idx < 2);
	return idx + 1;
}

static inline u64 perm2idx(enum dma_data_direction permission)
{
	if (unlikely(!((permission == DMA_FROM_DEVICE) || (permission == DMA_TO_DEVICE))))
		panic("Permission unaccaptable %d\n", permission);

	return (permission -1);
}

/* IOVA: 48 bits:
 *  [ 1 copy_iova indicator:: core idx: TX/RX][iova range]
*/
static inline u64 	iova_encoding(enum dma_data_direction permission)
{
	//return	DMA_CACHE_FLAG |
	return	((smp_processor_id() << CORE_SHIFT) |
		(perm2idx(permission) << PERM_SHIFT));
}

static inline u64	alloc_key(enum dma_data_direction permission)
{
	return	((numa_node_id() << CORE_SHIFT) |
		 (perm2idx(permission) << PERM_SHIFT));
}

//Watch for the assymetry with iova_encoding;
static inline u64	iova_get_encoding(u64 iova)
{
	//return ((iova >> IOVA_RANGE_SHIFT) & ~DMA_CACHE_FLAG);
	return (iova & ~PAGE_MASK);
}

static inline u64 iova_key(u64 iova)
{
	u64 encoding = iova_get_encoding(iova);
	u64 node = cpu_to_node(encoding >> CORE_SHIFT);
	u64 perm = ((encoding & ~DMA_CACHE_CORE_MASK) >> PERM_SHIFT);
	return (node  << CORE_SHIFT|perm << PERM_SHIFT);
}

/*
enum dma_data_direction iova_perm(u64 iova)
{
	u64 encoding = iova_get_encoding(iova);
	u64 perm = ((encoding & ~DMA_CACHE_CORE_MASK) >> PERM_SHIFT);
	return idx2perm(perm);
}

u64 dma_cache_iova_key(u64 iova)
{
	u64 encoding = iova_get_encoding(iova);
	u64 core = encoding >> CORE_SHIFT;
	u64 perm = ((encoding & ~DMA_CACHE_CORE_MASK) >> PERM_SHIFT);
	return (core  << CORE_SHIFT|perm << PERM_SHIFT);
}
u64 dma_cache_iova_idx(u64 iova)
{
	return (iova & (BIT(IOVA_RANGE_SHIFT) - 1)) >> DMA_CACHE_SHIFT;
}
*/
/*
void iova_format(u64 iova)
{
	u64 encoding = iova_get_encoding(iova);
	u64 core = encoding >> CORE_SHIFT;
	u64 perm = ((encoding & ~DMA_CACHE_CORE_MASK) >> PERM_SHIFT);
	trace_printk("iova %llx: key %llx core %llx dir %s idx %llx\n", iova, iova_key(iova),
		     core , (perm) ? "RX" : "TX", dma_cache_iova_idx(iova));
	assert(perm < 2);
}

void iova_decode(u64 iova)
{
	u64 encoding = iova_get_encoding(iova);
	u64 core = encoding >> CORE_SHIFT;
	u64 perm = ((encoding & ~DMA_CACHE_CORE_MASK) >> PERM_SHIFT);
	pr_err("iova %llx: key %llx core %llx dir %s idx %llx\n", iova, iova_key(iova),
		     core , (perm) ? "RX" : "TX", dma_cache_iova_idx(iova));
	assert(perm < 2);
}
*/
u64 virt_to_iova(void *virt)
{
	u64 iova;
	struct page *page = virt_to_page(virt);
	struct page *head = compound_head(page);

	if (!head->iova)
		return -1;

	iova = (head->iova & PAGE_MASK) + ((page - head) * PAGE_SIZE) + ((u64)virt & (PAGE_SIZE -1));
	return iova;
}
EXPORT_SYMBOL(virt_to_iova);
/*
static inline void validate_iova(u64 iova)
{
	u64 encoding = iova_get_encoding(iova);
	u64 perm = ((encoding & ~DMA_CACHE_CORE_MASK) >> PERM_SHIFT);

	assert((encoding >> CORE_SHIFT) == smp_processor_id());
	assert(cpu_to_node((encoding >> CORE_SHIFT)) == numa_mem_id());

	assert(alloc_key(idx2perm(perm)) == iova_key(iova));
}
*/
static inline u64 alloc_new_iova(struct device	*dev,
				 enum dma_data_direction	dir)
{
	return iova_encoding(dir);
/*
	u64 iova = iova_encoding(dir) << IOVA_RANGE_SHIFT;
	u64 idx = atomic64_inc_return(&dev->iova_mag->last_idx[alloc_key(dir)]);

	return (iova | (idx -1) << DMA_CACHE_SHIFT);
*/
}

/*
static inline void map_each_page(struct device *dev, struct page *page,
				 enum dma_data_direction dir, u64 iova)
{
	int i;
	struct dma_map_ops *ops = get_dma_ops(dev);

	for (i = 0; i < PAGES_IN_DMA_CACHE_ELEM; i++) {
		if (ops->map_page(dev, page, 0, PAGE_SIZE, dir, 0, iova) != iova) {
			panic("Couldnt MAP page %llx (%d)", iova, i);
		}
		page++;
		iova += PAGE_SIZE;
	}
}
*/

static inline struct page *inc_mapping(struct device	*dev,
				       enum dma_data_direction	dir)
{
	dma_addr_t	iova = 0;
	u64		encoding;
	//struct dma_map_ops *ops = get_dma_ops(dev);
	struct page	*page;

	page = alloc_pages( __GFP_COMP | __GFP_NOWARN |
			   __GFP_NORETRY | GFP_ATOMIC | __GFP_IO
			   , get_order(DMA_CACHE_ELEM_SIZE));
	if (!page) {
		panic("Couldnt alloc pages\n");
		return ERR_PTR(-ENOMEM);
	}

	encoding = alloc_new_iova(dev, dir);

	iova = dma_map_page(dev, page, 0, DMA_CACHE_ELEM_SIZE, dir);

	page->iova	= encoding;
	trace("new elem %lx <%llx>",(unsigned long)page, page->iova);
	//validate_iova(page->iova);
	page->device	= dev;

	return page;
}

struct page *alloc_mapped_pages(struct device *dev, enum dma_data_direction dir)
{
	u64	idx = alloc_key(dir);
	struct  mag_allocator *allocator = &dev->iova_mag->allocator[idx];
	struct page *elem = mag_alloc_elem(allocator);

	assert(idx < NUM_ALLOCATORS);
	if (unlikely(!elem)) {
		elem = inc_mapping(dev, dir);
		set_page_count(elem, 0x1000);
	}

	assert(page_count(elem) == 0x1000);
	init_page_count(elem);
	assert(numa_mem_id() == numa_node_id());
	assert(page_to_nid(elem) == numa_mem_id());

	return elem;
}

struct page_frag_cache *get_frag_cache(struct dev_iova_mag *iova_mag, int idx,
					enum dma_cache_frag_type frag_type)
{
	int cpu	= get_cpu();
	int cpu_idx = cpu << 1| ((in_softirq()) ? 1 : 0);

	return &iova_mag->frag_cache[cpu_idx][frag_type][idx];
}

#define MIN_COPY_ALLOC_MASK (64 - 1)
void *alloc_mapped_frag(struct device *dev, struct page_frag_cache *nc, size_t fragsz, enum dma_data_direction dir)
{
	unsigned int size = DMA_CACHE_ELEM_SIZE;
	struct page *page;
	int offset;

	assert(fragsz <= ALIGN(fragsz, 64));
	fragsz = ALIGN(fragsz, 64);
	offset = nc->offset - fragsz;

	if (unlikely(offset < 0)) {
		if (likely(nc->va)) {
		/* The fist frag was release with refcnt = 2 */
			put_page(virt_to_page(nc->va));
		}
		nc->va = NULL;
	}

	if (unlikely(!nc->va)) {
		page = alloc_mapped_pages(dev, dir);
		if (!page)
			return NULL;

		nc->va = page_address(page);
		offset = size - fragsz;
	}
	/* We need to make sure the page is not free before its replaced in the cache */
	get_page(virt_to_page(nc->va));

	nc->offset = offset;

	return nc->va + offset;
}

//Must set skb->head_frag
void *__dma_cache_alloc(struct device *dev, size_t size, enum dma_data_direction dir, const char *func)
{
	void *va;
	struct page_frag_cache *nc = get_frag_cache(dev->iova_mag, 0,
						    (dir == DMA_TO_DEVICE) ? DMA_CACHE_FRAG_PARTIAL_R : DMA_CACHE_FRAG_PARTIAL_W);
	assert(size <= DMA_CACHE_ELEM_SIZE);
	va = alloc_mapped_frag(dev, nc, size, dir);
	if (dir == DMA_TO_DEVICE)
		trace("[1]%s: size %zu page %lx (%d) %lx\n", func, size, (unsigned long)virt_to_head_page(va), atomic_read(&(virt_to_head_page(va))->_refcount), (unsigned long)(va));
	return va;
}
EXPORT_SYMBOL(__dma_cache_alloc);

struct page *__dma_cache_alloc_page(struct device *dev, enum dma_data_direction dir, const char *func)
{
	struct page_frag_cache *nc = get_frag_cache(dev->iova_mag, 1,
						    (dir == DMA_TO_DEVICE) ? DMA_CACHE_FRAG_FULL_R : DMA_CACHE_FRAG_FULL_W);

	void *va = alloc_mapped_frag(dev, nc, PAGE_SIZE, dir);

	assert(virt_addr_valid(va));
	if (dir == DMA_TO_DEVICE)
		trace("[2]%s: size %u page %lx (%d)\n", func, 4096, (unsigned long)virt_to_head_page(va), atomic_read(&(virt_to_head_page(va))->_refcount));
	return virt_to_page(va);
}
EXPORT_SYMBOL(__dma_cache_alloc_page);

struct page *__dma_cache_alloc_pages(struct device *dev, int order, enum dma_data_direction dir, const char *func)
{
	assert(order <= DMA_CACHE_MAX_ORDER);

	if (order == DMA_CACHE_MAX_ORDER) {
		struct page *page;
		WARN_ONCE((order != DMA_CACHE_MAX_ORDER), "order is %d", order);
		page = alloc_mapped_pages(dev, dir);
		if (dir == DMA_TO_DEVICE)
			trace("[3]%s: size %u page %lx (%d)\n", func, 4096 << order, (unsigned long)page, atomic_read(&page->_refcount));
		return page;
	} else {
		struct page_frag_cache *nc = get_frag_cache(dev->iova_mag, order + 1,
						    (dir == DMA_TO_DEVICE) ? DMA_CACHE_FRAG_FULL_R : DMA_CACHE_FRAG_FULL_W);
		void *va = alloc_mapped_frag(dev, nc, PAGE_SIZE << order, dir);

		if (dir == DMA_TO_DEVICE)
			trace("[4]%s: size %u page %lx (%d)\n", func, 4096 << order, (unsigned long)virt_to_head_page(va), atomic_read(&(virt_to_head_page(va))->_refcount));
		assert(virt_addr_valid(va));
		return virt_to_page(va);
	}
}
EXPORT_SYMBOL(__dma_cache_alloc_pages);

void __dma_cache_free(struct device *dev, struct page *elem, const char *func)
{
	int	idx = iova_key(elem->iova);
	struct  mag_allocator *allocator = &dev->iova_mag->allocator[idx];

	assert(page_to_nid(elem) == (idx >> CORE_SHIFT));
	assert(idx < NUM_ALLOCATORS);
	assert(elem == compound_head(elem));
	trace("[5]%s: page %lx (%d)\n", func, (unsigned long)elem, atomic_read(&(elem->_refcount)));
	set_page_count(elem, 0x1000);
	mag_free_elem(allocator, elem);

	//mag_free_elem(allocator, elem, iova_core(elem->iova)); // When magazine support available
}
EXPORT_SYMBOL(__dma_cache_free);

int	register_iova_map(struct device *dev)
{
	uint64_t i;
	struct page *page = alloc_pages(__GFP_COMP | __GFP_ZERO,
                                        get_order(sizeof(struct dev_iova_mag)));

	assert(page);
	dev->iova_mag = page_address(page);

	pr_err("\t num allocators %lu + %u\n", NUM_ALLOCATORS, num_online_cpus() * 2);
	for (i = 0; i < NUM_ALLOCATORS; i++) {
		mag_allocator_init(&dev->iova_mag->allocator[i]);
	}
	return 0;
}
EXPORT_SYMBOL(register_iova_map);

void	unregister_iova_map(struct device *dev)
{
	dev->iova_mag = NULL;

//page_address(page);
//
//for (i = 0; i < NUM_ALLOCATORS; i++) {
//	mag_allocator_init(&dev->iova_mag->allocator[i]);
//}
}
EXPORT_SYMBOL(unregister_iova_map);
