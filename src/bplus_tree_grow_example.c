#include <adftool_private.h>
#include <stdio.h>

#define _(String) gettext (String)
#define N_(String) (String)

/* The API is storage-agnostic. Here we make storage for a 4-B+ with a
   root node and 2 leaves, and room for a new root to put on top. */
static uint32_t data[][9] = {
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
   0, 1 << 31},
  {0}
};

static const size_t data_dimension = sizeof (data[0]) / sizeof (data[0][0]);

static uint32_t n_nodes = 3;

/* This is what we would like to get at the end: */
static const uint32_t expected[][9] = {
  /* This is the new hat on top of the old root, the old root (its
     only child) is now ID 3. */
  {
   /* */ -1, -1, -1,
   /* */ 3, 0, 0, 0,
   /* */ -1, 0
   },
  /* The leaves have updated their parent, it is now number 3. */
  {
   0, 1, 2,
   0, 1, 2, 2,
   3, 1 << 31},
  {
   3, 4, 5,
   3, 4, 5, 0,
   3, 1 << 31},
  /* The old root has now 0 as a parent. */
  {
   /* */ 2, -1, -1,
   /* */ 1, 2, 0, 0,
   /* */ 0, 0
   }
};

/* This is the parameters of the B+ tree: how we fetch rows, how we
   allocate nodes, and how we save nodes. */
static struct adftool_bplus_parameters *parameters = NULL;

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

/* There is no need to compare keys in the back-end. */

/* However, we need to allocate nodes. We aren’t allowed to fail here,
   so if your backend may fail, you want to collect all the updates
   and allocations and do them at the end. */
static void
allocate (uint32_t * node_id, void *context)
{
  (void) context;
  assert (n_nodes == 3);
  fprintf (stderr, _("−> Allocate a new node: %u\n"), n_nodes);
  *node_id = n_nodes++;
}

/* We also need to save rows of nodes. */
static void
store (uint32_t node_id, size_t start, size_t len, const uint32_t * row,
       void *context)
{
  (void) context;
  assert (start + len <= sizeof (data[0]) / sizeof (data[0][0]));
  assert (node_id < n_nodes);
  uint32_t full_row[9] = { 0 };
  memcpy (full_row + start, row, len * sizeof (uint32_t));
  fprintf (stderr,
	   _("−> Store %u, [%d, %d, %d, %d, %d, %d, %d, %d, %08x]\n"),
	   node_id, (int32_t) full_row[0], (int32_t) full_row[1],
	   (int32_t) full_row[2], (int32_t) full_row[3],
	   (int32_t) full_row[4], (int32_t) full_row[5],
	   (int32_t) full_row[6], (int32_t) full_row[7], full_row[8]);
  memcpy (data[node_id] + start, row, len * sizeof (data[0][0]));
}

/* This is how we search for a particular key. */
static void
grow (void)
{
  /* We need to wrap key first. */
  int error = adftool_bplus_grow (parameters);
  assert (!error);
}

int
main ()
{
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
  /* We need to set the parameters. */
  parameters = adftool_bplus_parameters_alloc ();
  if (parameters == NULL)
    {
      fprintf (stderr, _("adftool memory allocation problem detected.\n"));
      abort ();
    }
  adftool_bplus_parameters_set_fetch (parameters, fetch, NULL);
  adftool_bplus_parameters_set_allocate (parameters, allocate, NULL);
  adftool_bplus_parameters_set_store (parameters, store, NULL);
  grow ();
  assert (n_nodes == sizeof (expected) / sizeof (expected[0]));
  for (size_t i = 0; i < n_nodes; i++)
    {
      static const size_t dimension =
	(sizeof (expected[i]) / sizeof (expected[i][0]));
      assert (dimension == data_dimension);
      for (size_t j = 0; j < dimension; j++)
	{
	  assert (data[i][j] == expected[i][j]);
	}
    }
  adftool_bplus_parameters_free (parameters);
  return 0;
}
