#define	JEMALLOC_CHUNK_PRIVATE_C_
#include "jemalloc/internal/jemalloc_internal.h"
/******************************************************************************/

/*
 * Protects moving the heap pointer.  This avoids malloc races among threads.
 */
static malloc_mutex_t	private_mtx;

/* Base address of the private heap. */
static void		*private_base;
/* Current end of the private heap. */
static void		*private_top;
/* Upper limit on private heap addresses. */
static void		*private_max;

/******************************************************************************/

void *
chunk_alloc_private(size_t size, size_t alignment, bool *zero)
{
	void *ret;

	cassert(config_private);
	assert(size > 0 && (size & chunksize_mask) == 0);
	assert(alignment > 0 && (alignment & chunksize_mask) == 0);

	malloc_mutex_lock(&private_mtx);
	if (config_debug) {
		malloc_printf("private: %zx, %zx\n", size, alignment);
	}

	if (private_top < private_max) {
		size_t gap_size, cpad_size;
		void *cpad, *private_next;

		/*
		 * Calculate how much padding is necessary to
		 * chunk-align the end of the DSS.
		 */
		gap_size = (chunksize - CHUNK_ADDR2OFFSET(private_top)) &
		    chunksize_mask;
		/*
		 * Compute how much chunk-aligned pad space (if any) is
		 * necessary to satisfy alignment.  This space can be
		 * recycled for later use.
		 */
		cpad = (void *)((uintptr_t)private_top + gap_size);
		ret = (void *)ALIGNMENT_CEILING((uintptr_t)private_top,
						alignment);
		cpad_size = (uintptr_t)ret - (uintptr_t)cpad;
		private_next = (void *)((uintptr_t)ret + size);
		if ((uintptr_t)ret < (uintptr_t)private_top ||
		    (uintptr_t)private_next < (uintptr_t)private_top) {
			/* Wrap-around. */
			if (config_debug) {
				malloc_printf("Wrap!\n");
			}
			malloc_mutex_unlock(&private_mtx);
			return (NULL);
		}
		if ((uintptr_t)private_next > (uintptr_t)private_max) {
			/* No room. */
			if (config_debug) {
				malloc_printf("Out of room! %p, %p\n",
					      private_next, private_max);
			}
			malloc_mutex_unlock(&private_mtx);
			return (NULL);
		}
		private_top = private_next;
		malloc_mutex_unlock(&private_mtx);
		if (cpad_size != 0)
			chunk_unmap(cpad, cpad_size);
		if (*zero) {
			VALGRIND_MAKE_MEM_UNDEFINED(ret, size);
			memset(ret, 0, size);
		}
		return (ret);
	}
	malloc_mutex_unlock(&private_mtx);

	return (NULL);
}

bool
chunk_in_private(void *chunk)
{
	bool ret;

	cassert(config_private);

	malloc_mutex_lock(&private_mtx);
	if ((uintptr_t)chunk >= (uintptr_t)private_base
	    && (uintptr_t)chunk < (uintptr_t)private_top)
		ret = true;
	else
		ret = false;
	malloc_mutex_unlock(&private_mtx);

	return (ret);
}

bool
chunk_private_boot(void)
{

	cassert(config_private);

	if (malloc_mutex_init(&private_mtx))
		return (true);
	return (false);
}

void
chunk_private_config(void *base, size_t len)
{
	if (config_valgrind && opt_valgrind) {
		if (private_base != NULL && private_max != NULL) {
			VALGRIND_MAKE_MEM_NOACCESS(private_base,
						   private_max - private_base);
		}
	}

	private_base = base;
	private_top = private_base;
	private_max = base + len;

	if (config_debug) {
		malloc_printf("private heap from %p to %p.\n",
			      private_base, private_max);
	}
}

void
chunk_private_prefork(void)
{

	if (config_private)
		malloc_mutex_prefork(&private_mtx);
}

void
chunk_private_postfork_parent(void)
{

	if (config_private)
		malloc_mutex_postfork_parent(&private_mtx);
}

void
chunk_private_postfork_child(void)
{

	if (config_private)
		malloc_mutex_postfork_child(&private_mtx);
}

/******************************************************************************/
