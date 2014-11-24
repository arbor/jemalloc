/******************************************************************************/
#ifdef JEMALLOC_H_EXTERNS

void	*chunk_alloc_private(size_t size, size_t alignment, bool *zero);
bool	chunk_in_private(void *chunk);
bool	chunk_private_boot(void);
void	chunk_private_config(void *base, size_t len);
void	chunk_private_prefork(void);
void	chunk_private_postfork_parent(void);
void	chunk_private_postfork_child(void);

#endif /* JEMALLOC_H_EXTERNS */
/******************************************************************************/
#ifdef JEMALLOC_H_INLINES

#endif /* JEMALLOC_H_INLINES */
/******************************************************************************/
