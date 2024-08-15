/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2014-2015 ARM Ltd.
 */
#ifndef __DMA_IOMMU_H
#define __DMA_IOMMU_H

#include <linux/errno.h>
#include <linux/types.h>

#ifdef CONFIG_IOMMU_DMA
#include <linux/dma-mapping.h>
#include <linux/iommu.h>
#include <linux/msi.h>

/* iova map and unmap functions for batch iova*/
void iommu_dma_unmap_page_iova(struct device *dev, dma_addr_t dma_handle,
		size_t size, size_t iova_size, bool free_iova, enum dma_data_direction dir, unsigned long attrs);
void iommu_dma_unmap_page(struct device *dev, dma_addr_t dma_handle,
		size_t size, enum dma_data_direction dir, unsigned long attrs);
dma_addr_t iommu_dma_map_page_iova(struct device *dev, struct page *page, dma_addr_t iova_addr,bool first_iova,
		unsigned long offset, size_t size, enum dma_data_direction dir,
		unsigned long attrs);
dma_addr_t iommu_dma_map_page(struct device *dev, struct page *page,
		unsigned long offset, size_t size, enum dma_data_direction dir,
		unsigned long attrs);
dma_addr_t iommu_dma_alloc_iova(struct iommu_domain *domain,
		size_t size, u64 dma_limit, struct device *dev);
void iommu_dma_free_iova(struct iommu_dma_cookie *cookie,
		dma_addr_t iova, size_t size, struct iommu_iotlb_gather *gather);
void iommu_dma_free_iova_fs(struct iommu_domain *domain,
		dma_addr_t iova, size_t size);

/* Domain management interface for IOMMU drivers */
int iommu_get_dma_cookie(struct iommu_domain *domain);
int iommu_get_msi_cookie(struct iommu_domain *domain, dma_addr_t base);
void iommu_put_dma_cookie(struct iommu_domain *domain);

/* Setup call for arch DMA mapping code */
void iommu_setup_dma_ops(struct device *dev, u64 dma_base, u64 dma_limit);
int iommu_dma_init_fq(struct iommu_domain *domain);

/* The DMA API isn't _quite_ the whole story, though... */
/*
 * iommu_dma_prepare_msi() - Map the MSI page in the IOMMU device
 *
 * The MSI page will be stored in @desc.
 *
 * Return: 0 on success otherwise an error describing the failure.
 */
int iommu_dma_prepare_msi(struct msi_desc *desc, phys_addr_t msi_addr);

/* Update the MSI message if required. */
void iommu_dma_compose_msi_msg(struct msi_desc *desc,
			       struct msi_msg *msg);

void iommu_dma_get_resv_regions(struct device *dev, struct list_head *list);

void iommu_dma_free_cpu_cached_iovas(unsigned int cpu,
		struct iommu_domain *domain);

extern bool iommu_dma_forcedac;

#else /* CONFIG_IOMMU_DMA */

struct iommu_domain;
struct msi_desc;
struct msi_msg;
struct device;


static inline void iommu_setup_dma_ops(struct device *dev, u64 dma_base,
				       u64 dma_limit)
{
}

static inline int iommu_dma_init_fq(struct iommu_domain *domain)
{
	return -EINVAL;
}

static inline int iommu_get_dma_cookie(struct iommu_domain *domain)
{
	return -ENODEV;
}

static inline int iommu_get_msi_cookie(struct iommu_domain *domain, dma_addr_t base)
{
	return -ENODEV;
}

static inline void iommu_put_dma_cookie(struct iommu_domain *domain)
{
}

static inline int iommu_dma_prepare_msi(struct msi_desc *desc,
					phys_addr_t msi_addr)
{
	return 0;
}

static inline void iommu_dma_compose_msi_msg(struct msi_desc *desc,
					     struct msi_msg *msg)
{
}

static inline void iommu_dma_get_resv_regions(struct device *dev, struct list_head *list)
{
}

#endif	/* CONFIG_IOMMU_DMA */
#endif	/* __DMA_IOMMU_H */
