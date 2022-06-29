#ifndef H_ADFTOOL_INCLUDED
#define H_ADFTOOL_INCLUDED

#include <stdint.h>
#include <stddef.h>

#ifdef LIBADFTOOL_WITHOUT_HDF5
typedef void *hid_t;
#else /* not LIBADFTOOL_WITHOUT_HDF5 */
#include <hdf5.h>
#endif /* not LIBADFTOOL_WITHOUT_HDF5 */

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/* Keys are compared against each other in a B+ tree. However, since
   we are defining an index, some keys are already known and so we
   just use their index in the key table, or they are not known. For
   instance, the key used in a query can be random and thus we know
   its literal value, but it does not exist in the table. */

  int adftool_with_hdf5 (void);

  struct adftool_bplus_key;

  struct adftool_bplus_key *adftool_bplus_key_alloc (void);

  void adftool_bplus_key_free (struct adftool_bplus_key *key);

  /* Return 0 if no error, something else if the key is not of the
     correct type. */
  int adftool_bplus_key_get_known (const struct adftool_bplus_key *key,
				   uint32_t * index);

  int adftool_bplus_key_get_unknown (const struct adftool_bplus_key *key,
				     void **key_value);

  void adftool_bplus_key_set_known (struct adftool_bplus_key *key,
				    uint32_t key_index);

  void adftool_bplus_key_set_unknown (struct adftool_bplus_key *key,
				      void *key_value);

  struct adftool_bplus_parameters;

  struct adftool_bplus_parameters *adftool_bplus_parameters_alloc (void);

  void adftool_bplus_parameters_free (struct adftool_bplus_parameters
				      *parameters);

  void adftool_bplus_parameters_set_fetch (struct adftool_bplus_parameters
					   *parameters,
					   int (*fetch) (uint32_t, size_t *,
							 size_t, size_t,
							 uint32_t *, void *),
					   void *context);

  void adftool_bplus_parameters_set_compare (struct adftool_bplus_parameters
					     *parameters,
					     int (*compare) (const struct
							     adftool_bplus_key
							     *,
							     const struct
							     adftool_bplus_key
							     *, int *,
							     void *),
					     void *context);

  void adftool_bplus_parameters_set_fetch_from_hdf5 (struct
						     adftool_bplus_parameters
						     *parameters,
						     hid_t dataset);

/* The lookup function. The parameters "fetch" and "compare" must be
   set. */

  int
    adftool_bplus_lookup (const struct adftool_bplus_key *needle,
			  struct adftool_bplus_parameters
			  *parameters, size_t start, size_t max,
			  size_t *n_results, uint32_t * results);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* not H_ADFTOOL_INCLUDED */
