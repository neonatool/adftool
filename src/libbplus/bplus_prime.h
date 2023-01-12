#ifndef H_BPLUS_PRIME_INCLUDED
# define H_BPLUS_PRIME_INCLUDED

# include <stdint.h>
# include <stddef.h>

static inline void prime (size_t order, uint32_t * row_0);

static inline void
prime (size_t order, uint32_t * row_0)
{
  for (size_t i = 0; i < order - 1; i++)
    {
      row_0[i] = ((uint32_t) (-1));
      row_0[order - 1 + i] = 0;
    }
  row_0[2 * order - 2] = 0;
  row_0[2 * order - 1] = ((uint32_t) (-1));
  row_0[2 * order] = ((uint32_t) ((uint32_t) 1) << 31);
}

#endif /* H_BPLUS_PRIME_INCLUDED */
