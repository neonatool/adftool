#include <adftool_private.h>
#include <adftool_bplus.h>

struct adftool_results *
adftool_results_alloc (void)
{
  struct adftool_results *ret = malloc (sizeof (struct adftool_results));
  if (ret != NULL)
    {
      ret->n_results = 0;
      ret->statements = NULL;
    }
  return ret;
}

void
adftool_results_free (struct adftool_results *results)
{
  if (results != NULL)
    {
      adftool_results_resize (results, 0);
      free (results->statements);
    }
  free (results);
}

size_t
adftool_results_count (const struct adftool_results *results)
{
  return results->n_results;
}

const struct adftool_statement *
adftool_results_get (const struct adftool_results *results, size_t i)
{
  assert (i < results->n_results);
  return results->statements[i];
}

int
adftool_results_resize (struct adftool_results *results, size_t size)
{
  for (size_t i = 0; i < results->n_results; i++)
    {
      adftool_statement_free (results->statements[i]);
    }
  results->statements =
    realloc (results->statements, size * sizeof (struct adftool_statement *));
  if (results->statements == NULL)
    {
      return 1;
    }
  results->n_results = size;
  int allocation_error = 0;
  for (size_t i = 0; i < results->n_results; i++)
    {
      results->statements[i] = adftool_statement_alloc ();
      if (results->statements[i] == NULL)
	{
	  allocation_error = 1;
	}
    }
  if (allocation_error)
    {

      adftool_results_resize (results, 0);
      return 1;
    }
  return 0;
}

int
adftool_results_set (struct adftool_results *results, size_t i,
		     const struct adftool_statement *statement)
{
  assert (i < results->n_results);
  return adftool_statement_copy (results->statements[i], statement);
}
