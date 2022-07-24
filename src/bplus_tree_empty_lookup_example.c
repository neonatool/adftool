#include <adftool_private.h>
#include <adftool_bplus_key.h>
#include <adftool_bplus.h>
#include <stdio.h>

/* This example checks that looking up anything in an empty tree works
   as expected.. */

static const uint32_t data[][9] = {
  {
   -1, -1, -1,
   -1, -1, -1, 0,
   -1, 1 << 31}
};

/* This is the parameters of the B+ tree: how we fetch rows, and how
   we compare keys. */
static struct bplus *bplus = NULL;

/* This is how we look up something in the back-end. */
static int
fetch (uint32_t node_id, size_t *row_actual_length,
       size_t request_start, size_t request_length,
       uint32_t * response, void *context)
{
  (void) context;
  /* context is where you would find the data for instance. */
  assert (node_id < 3);
  if (request_start + request_length > 9)
    {
      request_length = 9 - request_start;
    }
  fprintf (stderr,
	   _("−> Load %u: [%d, %d, %d, %d, %d, %d, %d, %d, %08x]\n"),
	   node_id, (int32_t) data[node_id][0], (int32_t) data[node_id][1],
	   (int32_t) data[node_id][2], (int32_t) data[node_id][3],
	   (int32_t) data[node_id][4], (int32_t) data[node_id][5],
	   (int32_t) data[node_id][6], (int32_t) data[node_id][7],
	   data[node_id][8]);
  *row_actual_length = 9;
  const uint32_t *row = data[node_id];
  memcpy (response, row + request_start, request_length * sizeof (uint32_t));
  return 0;
}

/* This is how we search for a particular key. */
static size_t
search_key (const char *key, size_t start, size_t max, uint32_t * results)
{
  /* We need to wrap key first. */
  struct adftool_bplus_key bplus_key;
  key_set_unknown (&bplus_key, (void *) key);
  size_t n_results;
  int error =
    bplus_lookup (&bplus_key, bplus, start, max, &n_results, results);
  assert (!error);
  return n_results;
}

static void
check_string_absent (const char *string)
{
  size_t n_matches = search_key (string, 0, 0, NULL);
  assert (n_matches == 0);
}

int
main ()
{
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
  /* We need to set the parameters. */
  static struct bplus my_bplus;
  bplus = &my_bplus;
  bplus_set_fetch (bplus, fetch, NULL);
  check_string_absent ("anything");
  return 0;
}
