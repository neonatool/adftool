#include <adftool_private.h>
#include <adftool_bplus_key.h>
#include <adftool_bplus.h>
#include <stdio.h>

/* This example uses internal things of adftool. Thus it can just be
   compiled alongside it. */

/* The API is storage-agnostic. Here we make storage for a 4-B+ with
   a root node and 2 leaves. */
static const uint32_t data[][9] = {
  {
   /* */ 2, -1, -1,
   /* */ 1, 2, 0, 0,
   /* */ -1, 0
   },
  {
   0, 1, 2,
   0, 1, 2, 2,
   0, 1 << 31},
  {
   3, 4, 5,
   3, 4, 5, 0,
   0, 1 << 31}
};

/* Suppose that we store strings in our index. */
static const char *dictionary[] = {
  "bass",
  "cello",
  "guitar",
  "harp",
  "ukulele",
  "violin"
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

/* This is how we compare keys in the back-end. */
static const char *
literal_key (const struct adftool_bplus_key *key)
{
  uint32_t index;
  void *value;
  if (key_get_known (key, &index) == 0)
    {
      return dictionary[index];
    }
  if (key_get_unknown (key, &value) == 0)
    {
      return (const char *) value;
    }
  abort ();
}

/* WARNING: compare returns 0 on success. The result is passed as
   *result, NOT returned from the function. */
static int
compare (const struct adftool_bplus_key *key_1,
	 const struct adftool_bplus_key *key_2, int *result, void *context)
{
  (void) context;
  /* context could be used to dereference known keys for instance. */
  const char *s1 = literal_key (key_1);
  const char *s2 = literal_key (key_2);
  fprintf (stderr, _("−> Compare %s to %s: %d\n"), s1, s2, strcmp (s1, s2));
  *result = strcmp (s1, s2);
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
check_string_present (const char *string, uint32_t expected_index)
{
  /* search_key will iterate over all the answers, but we only need to
     store the first one (ignore 0 of them, then store 1). It is
     stored in the 1-cell "array", &index. */
  uint32_t index;
  fprintf (stderr, _("Finding %s…\n"), string);
  size_t n_matches = search_key (string, 0, 1, &index);
  fprintf (stderr, _("Found %s.\n"), string);
  assert (n_matches == 1);
  assert (index == expected_index);
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
  if (bplus == NULL)
    {
      fprintf (stderr, _("adftool memory allocation problem detected.\n"));
      abort ();
    }
  bplus_set_fetch (bplus, fetch, NULL);
  bplus_set_compare (bplus, compare, NULL);
  int i;
  for (i = 0; i < 6; i++)
    {
      check_string_present (dictionary[i], i);
    }
  check_string_absent ("alto sax");
  check_string_absent ("castanets");
  check_string_absent ("clarinet");
  check_string_absent ("handpan");
  check_string_absent ("trombone");
  check_string_absent ("vibraslap");
  check_string_absent ("xylophone");
  return 0;
}
