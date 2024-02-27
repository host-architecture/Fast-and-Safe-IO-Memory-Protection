#ifndef _DMA_BOUNCE_
#define _DMA_BOUNCE_

#include <asm/page.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/dma-direction.h>
#include <linux/gfp.h>
#include <linux/types.h>
#include <linux/magazine.h>
#include <linux/mm_types.h>

#define BITS_IN_IOVA		48

#define CORES			32
#define PERMISSIONS		2
#define CORE_BITS		(ilog2(CORES))
#define PERMISSION_BITS		(ilog2(PERMISSIONS))

#define PERM_SHIFT		0
#define CORE_SHIFT		(PERM_SHIFT + PERMISSION_BITS)
#define IOVA_ENCODING_BITS	(1 + CORE_BITS + PERMISSION_BITS)
#define IOVA_INVALID_BITS	(IOVA_ENCODING_BITS + 1)
#define DMA_CACHE_FLAG		BIT(IOVA_ENCODING_BITS)
#define DMA_CACHE_FLAG_INVALID	BIT(IOVA_INVALID_BITS)
#define IOVA_RANGE_SHIFT	(BITS_IN_IOVA - (IOVA_ENCODING_BITS + 1))

#define DMA_CACHE_CORE_MASK	((BIT(CORE_BITS) -1) << CORE_SHIFT)
#define DMA_CACHE_SHIFT		16	/* 32K */ /*Due to skb lim + compound page size.*/
#define PAGES_IN_DMA_CACHE_ELEM (BIT(DMA_CACHE_SHIFT - PAGE_SHIFT))
#define DMA_CACHE_ELEM_SIZE	(BIT(DMA_CACHE_SHIFT))
#define NUM_ALLOCATORS		(BIT(IOVA_ENCODING_BITS - 1))
#define DMA_CACHE_MAX_ORDER	get_order(DMA_CACHE_ELEM_SIZE)

enum dma_cache_frag_type {
	DMA_CACHE_FRAG_PARTIAL_R,
	DMA_CACHE_FRAG_PARTIAL_W,
	DMA_CACHE_FRAG_FULL_R,
	DMA_CACHE_FRAG_FULL_W,
	DMA_CACHE_FRAG_TYPES
};

struct dev_iova_mag {
	struct	mag_allocator allocator[NUM_ALLOCATORS];
	atomic64_t last_idx[NUM_ALLOCATORS];
	struct page_frag_cache frag_cache[CORES * 2][DMA_CACHE_FRAG_TYPES][DMA_CACHE_MAX_ORDER + 1];
};

u64 dma_cache_iova_key(u64);
u64 dma_cache_iova_idx(u64);
u64 virt_to_iova(void *);
void iova_format(u64);
void iova_decode(u64);
enum dma_data_direction iova_perm(u64);

size_t dma_cache_size(void *);

#define dma_cache_alloc(d, s, dir) 		__dma_cache_alloc(d, s, dir, __FUNCTION__)
#define dma_cache_alloc_page(d, dir) 		__dma_cache_alloc_page(d, dir, __FUNCTION__)
#define dma_cache_alloc_pages(d, o, dir) 	__dma_cache_alloc_pages(d, o, dir, __FUNCTION__)
#define dma_cache_free(d, p)			__dma_cache_free(d,p, __FUNCTION__)

void *__dma_cache_alloc(struct device *, size_t, enum dma_data_direction dir, const char *func);
struct page *__dma_cache_alloc_page(struct device *dev, enum dma_data_direction dir, const char *func);
struct page *__dma_cache_alloc_pages(struct device *dev, int order, enum dma_data_direction dir, const char *func);
void __dma_cache_free(struct device *, struct page *, const char *);

int	register_iova_map(struct device *);
void	unregister_iova_map(struct device *dev);
#endif /*_DMA_BOUNCE_*/
