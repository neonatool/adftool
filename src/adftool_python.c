#include <config.h>

#if defined _WIN32 && !defined __CYGWIN__
# define LIBADFTOOL_PYTHON_DLL_MADNESS 1
#else
# define LIBADFTOOL_PYTHON_DLL_MADNESS 0
#endif

#if BUILDING_LIBADFTOOL_PYTHON && HAVE_VISIBILITY
# define LIBADFTOOL_PYTHON_DLL_EXPORTED __attribute__((__visibility__("default")))
#elif BUILDING_LIBADFTOOL_PYTHON && LIBADFTOOL_PYTHON_DLL_MADNESS
# define LIBADFTOOL_PYTHON_DLL_EXPORTED __declspec(dllexport)
#elif LIBADFTOOL_PYTHON_DLL_MADNESS
# define LIBADFTOOL_PYTHON_DLL_EXPORTED __declspec(dllimport)
#else
# define LIBADFTOOL_PYTHON_DLL_EXPORTED
#endif

#define LIBADFTOOL_PYTHON_API \
  LIBADFTOOL_PYTHON_DLL_EXPORTED

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include <attribute.h>
#include <adftool.h>
#include <locale.h>
#include <time.h>
#include "gettext.h"
#include "relocatable.h"

#define _(String) dgettext (PACKAGE, (String))
#define N_(String) (String)

static PyObject *adftool_io_error;
static PyObject *adftool_rdf_error;

struct adftool_py_file
{
  /* *INDENT-OFF* */
  PyObject_HEAD
  /* *INDENT-ON* */
  struct adftool_file *ptr;
};

struct adftool_py_term
{
  /* *INDENT-OFF* */
  PyObject_HEAD
  /* *INDENT-ON* */
  struct adftool_term *ptr;
};

struct adftool_py_statement
{
  /* *INDENT-OFF* */
  PyObject_HEAD
  /* *INDENT-ON* */
  struct adftool_statement *ptr;
};

struct adftool_py_fir
{
  /* *INDENT-OFF* */
  PyObject_HEAD
  /* *INDENT-ON* */
  struct adftool_fir *ptr;
};

static PyObject *lytonepal (PyObject *, PyObject *);

static PyObject *file_new (PyTypeObject *, PyObject *, PyObject *);
static int file_init (PyObject *, PyObject *, PyObject *);
static void file_dealloc (PyObject *);
static PyObject *file_open (struct adftool_py_file *, PyObject *);
static PyObject *file_close (struct adftool_py_file *, PyObject *);
static PyObject *file_open_data (struct adftool_py_file *, PyObject *);
static PyObject *file_open_generated (struct adftool_py_file *, PyObject *);
static PyObject *file_get_data (struct adftool_py_file *, PyObject *);
static PyObject *lookup (struct adftool_py_file *, PyObject *);
static PyObject *lookup_objects (struct adftool_py_file *, PyObject *);
static PyObject *lookup_integer (struct adftool_py_file *, PyObject *);
static PyObject *lookup_double (struct adftool_py_file *, PyObject *);
static PyObject *lookup_date (struct adftool_py_file *, PyObject *);
static PyObject *lookup_string (struct adftool_py_file *, PyObject *);
static PyObject *lookup_subjects (struct adftool_py_file *, PyObject *);
static PyObject *delete (struct adftool_py_file *, PyObject *);
static PyObject *insert (struct adftool_py_file *, PyObject *);
static PyObject *find_channel_identifier (struct adftool_py_file *,
					  PyObject *);
static PyObject *get_channel_column (struct adftool_py_file *, PyObject *);
static PyObject *add_channel_type (struct adftool_py_file *, PyObject *);
static PyObject *get_channel_types (struct adftool_py_file *, PyObject *);
static PyObject *find_channels_by_type (struct adftool_py_file *, PyObject *);
static PyObject *eeg_get_data (struct adftool_py_file *, PyObject *);
static PyObject *eeg_set_data (struct adftool_py_file *, PyObject *);
static PyObject *eeg_get_time (struct adftool_py_file *, PyObject *);
static PyObject *eeg_set_time (struct adftool_py_file *, PyObject *);

static PyObject *term_new (PyTypeObject *, PyObject *, PyObject *);
static int term_init (PyObject *, PyObject *, PyObject *);
static void term_dealloc (PyObject *);
static PyObject *term_copy (struct adftool_py_term *, PyObject *);
static PyObject *term_set_named (struct adftool_py_term *, PyObject *);
static PyObject *term_set_blank (struct adftool_py_term *, PyObject *);
static PyObject *term_set_literal (struct adftool_py_term *, PyObject *);
static PyObject *term_set_integer (struct adftool_py_term *, PyObject *);
static PyObject *term_set_double (struct adftool_py_term *, PyObject *);
static PyObject *term_set_date (struct adftool_py_term *, PyObject *);
static PyObject *term_is_named (struct adftool_py_term *, PyObject *);
static PyObject *term_is_blank (struct adftool_py_term *, PyObject *);
static PyObject *term_is_literal (struct adftool_py_term *, PyObject *);
static PyObject *term_is_typed_literal (struct adftool_py_term *, PyObject *);
static PyObject *term_is_langstring (struct adftool_py_term *, PyObject *);
static PyObject *term_value (struct adftool_py_term *, PyObject *);
static PyObject *term_meta (struct adftool_py_term *, PyObject *);
static PyObject *term_as_integer (struct adftool_py_term *, PyObject *);
static PyObject *term_as_double (struct adftool_py_term *, PyObject *);
static PyObject *term_as_date (struct adftool_py_term *, PyObject *);
static PyObject *term_compare (struct adftool_py_term *, PyObject *);
static PyObject *term_parse_n3 (struct adftool_py_term *, PyObject *);
static PyObject *term_to_n3 (struct adftool_py_term *, PyObject *);

static PyObject *statement_new (PyTypeObject *, PyObject *, PyObject *);
static int statement_init (PyObject *, PyObject *, PyObject *);
static void statement_dealloc (PyObject *);
static PyObject *statement_set_subject (struct adftool_py_statement *,
					PyObject *);
static PyObject *statement_set_predicate (struct adftool_py_statement *,
					  PyObject *);
static PyObject *statement_set_object (struct adftool_py_statement *,
				       PyObject *);
static PyObject *statement_set_graph (struct adftool_py_statement *,
				      PyObject *);
static PyObject *statement_set_deletion_date (struct adftool_py_statement *,
					      PyObject *);
static PyObject *statement_unset_subject (struct adftool_py_statement *,
					  PyObject *);
static PyObject *statement_unset_predicate (struct adftool_py_statement *,
					    PyObject *);
static PyObject *statement_unset_object (struct adftool_py_statement *,
					 PyObject *);
static PyObject *statement_unset_graph (struct adftool_py_statement *,
					PyObject *);
static PyObject *statement_unset_deletion_date (struct adftool_py_statement *,
						PyObject *);
static PyObject *statement_get (struct adftool_py_statement *, PyObject *);
static PyObject *statement_compare (struct adftool_py_statement *,
				    PyObject *);

static PyObject *fir_new (PyTypeObject *, PyObject *, PyObject *);
static int fir_init (PyObject *, PyObject *, PyObject *);
static void fir_dealloc (PyObject *);
static PyObject *fir_order (struct adftool_py_fir *, PyObject *);
static PyObject *fir_coefficients (struct adftool_py_fir *, PyObject *);
static PyObject *fir_design_bandpass (struct adftool_py_fir *, PyObject *);
static PyObject *fir_apply (struct adftool_py_fir *, PyObject *);
static PyObject *fir_auto_bandwidth (struct adftool_py_fir *, PyObject *);
static PyObject *fir_auto_order (struct adftool_py_fir *, PyObject *);

static PyMemberDef adftool_file_members[] = {
  {NULL}
};

static PyMethodDef adftool_file_methods[] = {
  {"open", (PyCFunction) file_open, METH_VARARGS,
   N_("Open the file under the given name. "
      "If the write flag is set, open it for reading "
      "and writing. " "Otherwise, open it just for reading.")},
  {"close", (PyCFunction) file_close, METH_NOARGS,
   N_("Close the opened file.")},
  {"open_data", (PyCFunction) file_open_data, METH_VARARGS,
   N_("Open a virtual file with initial content. "
      "The argument is read-only. Use '.get_data()' "
      "to return the final content.")},
  {"open_generated", (PyCFunction) file_open_generated, METH_VARARGS,
   N_("Open a virtual file and populate it with a generated EEG.")},
  {"get_data", (PyCFunction) file_get_data, METH_VARARGS,
   N_("Discard the first start bytes, then fill the "
      "bytes object with the next bytes of the file. "
      "Return the total number of bytes in the file.")},
  {"lookup", (PyCFunction) lookup, METH_VARARGS,
   N_("Search for statements matching the pattern in the "
      "file, discard the first start statements, and fill "
      "the results with the next statements. "
      "Return the total number of statements " "that match the pattern.")},
  {"lookup_objects", (PyCFunction) lookup_objects, METH_VARARGS,
   N_("Search for non-deleted objects that match the pattern "
      "subject <predicate> ?. Discard the first start statements, "
      "and fill the results with the next objects. "
      "Return the total number of undeleted objects that match "
      "the pattern.")},
  {"lookup_integer", (PyCFunction) lookup_integer, METH_VARARGS,
   N_("Search for non-deleted literal integers that match the pattern "
      "subject <predicate> ?. Discard the first start values, "
      "and fill the results with the next values. "
      "Return the total number of undeleted values that match "
      "the pattern.")},
  {"lookup_double", (PyCFunction) lookup_double, METH_VARARGS,
   N_("Search for non-deleted literal doubles that match the pattern "
      "subject <predicate> ?. Discard the first start values, "
      "and fill the results with the next values. "
      "Return the total number of undeleted values that match "
      "the pattern.")},
  {"lookup_date", (PyCFunction) lookup_date, METH_VARARGS,
   N_("Search for non-deleted literal dates that match the pattern "
      "subject <predicate> ?. Discard the first start values, "
      "and fill the results with the next values. "
      "Return the total number of undeleted values that match "
      "the pattern.")},
  {"lookup_string", (PyCFunction) lookup_string, METH_VARARGS,
   N_("Search for non-deleted (possibly langtagged) literal strings "
      "that match the pattern "
      "subject <predicate> ?. Discard the first start values, "
      "and fill the results with the next values. "
      "Return the total number of undeleted values that match "
      "the pattern.")},
  {"lookup_subjects", (PyCFunction) lookup_subjects, METH_VARARGS,
   N_("Search for non-deleted subjects that match the pattern "
      "? <predicate> object. Discard the first start statements, "
      "and fill the results with the next subjects. "
      "Return the total number of undeleted subjects that match "
      "the pattern.")},
  {"delete", (PyCFunction) delete, METH_VARARGS,
   N_("Mark every statement that match the pattern as deleted.")},
  {"insert", (PyCFunction) insert, METH_VARARGS,
   N_("Insert the new statement in the file.")},
  {"find_channel_identifier", (PyCFunction) find_channel_identifier,
   METH_VARARGS,
   N_("Find the term identifying the channel "
      "whose data is stored in the given column.")},
  {"get_channel_column", (PyCFunction) get_channel_column, METH_VARARGS,
   N_("Find in which column the channel data is stored.")},
  {"add_channel_type", (PyCFunction) add_channel_type, METH_VARARGS,
   N_("Add a new type to the channel.")},
  {"get_channel_types", (PyCFunction) get_channel_types, METH_VARARGS,
   N_("Query all the types for a channel.")},
  {"find_channels_by_type", (PyCFunction) find_channels_by_type, METH_VARARGS,
   N_("Query all the channels of a specific type.")},
  {"get_eeg_data", (PyCFunction) eeg_get_data, METH_NOARGS,
   N_("Get the raw EEG data.")},
  {"set_eeg_data", (PyCFunction) eeg_set_data, METH_VARARGS,
   N_("Set all the raw EEG data at once.")},
  {"get_eeg_time", (PyCFunction) eeg_get_time, METH_NOARGS,
   N_("Get the EEG recording time and sampling frequency.")},
  {"set_eeg_time", (PyCFunction) eeg_set_time, METH_VARARGS,
   N_("Set the EEG recording time and sampling frequency.")},
  {NULL}
};

static PyTypeObject adftool_type_file = {
  /* *INDENT-OFF* */
  PyVarObject_HEAD_INIT (NULL, 0)
  /* *INDENT-ON* */
  .tp_name = "adftool.File",
  .tp_doc = N_("Handle to a file on the file system or in memory"),
  .tp_basicsize = sizeof (struct adftool_py_file),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_new = file_new,
  .tp_init = file_init,
  .tp_dealloc = file_dealloc,
  .tp_members = adftool_file_members,
  .tp_methods = adftool_file_methods
};

static PyMemberDef adftool_term_members[] = {
  {NULL}
};

static PyMethodDef adftool_term_methods[] = {
  {"copy", (PyCFunction) term_copy, METH_VARARGS,
   N_("Set the object to a copy of its argument.")},
  {"set_named", (PyCFunction) term_set_named, METH_VARARGS,
   N_("Set the object as an IRI reference.")},
  {"set_blank", (PyCFunction) term_set_blank, METH_VARARGS,
   N_("Set the object as a labeled blank node.")},
  {"set_literal", (PyCFunction) term_set_literal, METH_VARARGS,
   N_("Set the object as a literal node, with value as its "
      "literal value, and either type, langtag, or none of " "them.")},
  {"set_integer", (PyCFunction) term_set_integer, METH_VARARGS,
   N_("Set the object as a literal integer.")},
  {"set_double", (PyCFunction) term_set_double, METH_VARARGS,
   N_("Set the object as a literal floating point number.")},
  {"set_date", (PyCFunction) term_set_date, METH_VARARGS,
   N_("Set the object as a literal date, a tuple where the "
      "first element is the number of seconds since the "
      "epoch, and the second element is the number of "
      "nanoseconds since the start of the last second.")},
  {"is_named", (PyCFunction) term_is_named, METH_NOARGS,
   N_("Return whether the term is an IRI reference.")},
  {"is_blank", (PyCFunction) term_is_blank, METH_NOARGS,
   N_("Return whether the term is a labeled blank node.")},
  {"is_literal", (PyCFunction) term_is_literal, METH_NOARGS,
   N_("Return whether the term is a literal.")},
  {"is_typed_literal", (PyCFunction) term_is_typed_literal, METH_NOARGS,
   N_("Return whether the term is a typed literal.")},
  {"is_langstring", (PyCFunction) term_is_langstring, METH_NOARGS,
   N_("Return whether the term is a langstring.")},
  {"value", (PyCFunction) term_value, METH_VARARGS,
   N_("Skip start bytes, then fill the argument with the "
      "next bytes of the identifier or literal value. "
      "Return the total number of bytes required.")},
  {"meta", (PyCFunction) term_meta, METH_VARARGS,
   N_("Skip start bytes, then fill the argument with the "
      "next bytes of the namespace, type or langtag. "
      "Return the total number of bytes required.")},
  {"as_integer", (PyCFunction) term_as_integer, METH_NOARGS,
   N_("Interpret the term as a literal number, and convert "
      "it to an integer.")},
  {"as_double", (PyCFunction) term_as_double, METH_NOARGS,
   N_("Interpret the term as a literal number, and convert "
      "it to a floating point number.")},
  {"as_date", (PyCFunction) term_as_date, METH_NOARGS,
   N_("Interpret the term as a literal date, and return "
      "as a tuple the number of seconds since the epoch "
      "and the number of nanoseconds since the start of " "the second.")},
  {"compare", (PyCFunction) term_compare, METH_VARARGS,
   N_("Compare the N3 representation of self and the "
      "other term. Return a negative value is self "
      "comes first, a positive value if self comes "
      "last, and 0 if they are equal.")},
  {"parse_n3", (PyCFunction) term_parse_n3, METH_VARARGS,
   N_("Set self to a term parsed from the argument. "
      "Return the number of bytes used in the argument.")},
  {"to_n3", (PyCFunction) term_to_n3, METH_VARARGS,
   N_("Skip start bytes, then fill the argument with the "
      "next bytes of the N3 representation. "
      "Return the total number of bytes required.")},
  {NULL}
};

static PyTypeObject adftool_type_term = {
  /* *INDENT-OFF* */
  PyVarObject_HEAD_INIT (NULL, 0)
  /* *INDENT-ON* */
  .tp_name = "adftool.Term",
  .tp_doc = N_("Implementation of a RDF term: "
	       "IRI reference, blank node, or literal."),
  .tp_basicsize = sizeof (struct adftool_py_term),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_new = term_new,
  .tp_init = term_init,
  .tp_dealloc = term_dealloc,
  .tp_members = adftool_term_members,
  .tp_methods = adftool_term_methods
};

static PyMemberDef adftool_statement_members[] = {
  {NULL}
};

static PyMethodDef adftool_statement_methods[] = {
  {"set_subject", (PyCFunction) statement_set_subject, METH_VARARGS,
   N_("Set the statement subject.")},
  {"set_predicate", (PyCFunction) statement_set_predicate, METH_VARARGS,
   N_("Set the statement predicate.")},
  {"set_object", (PyCFunction) statement_set_object, METH_VARARGS,
   N_("Set the statement object.")},
  {"set_graph", (PyCFunction) statement_set_graph, METH_VARARGS,
   N_("Set the statement graph.")},
  {"set_deletion_date", (PyCFunction) statement_set_deletion_date,
   METH_VARARGS,
   N_("Set the statement deletion_date.")},
  {"unset_subject", (PyCFunction) statement_unset_subject, METH_NOARGS,
   N_("Unset the statement subject.")},
  {"unset_predicate", (PyCFunction) statement_unset_predicate, METH_NOARGS,
   N_("Unset the statement predicate.")},
  {"unset_object", (PyCFunction) statement_unset_object, METH_NOARGS,
   N_("Unset the statement object.")},
  {"unset_graph", (PyCFunction) statement_unset_graph, METH_NOARGS,
   N_("Unset the statement graph.")},
  {"unset_deletion_date", (PyCFunction) statement_unset_deletion_date,
   METH_NOARGS,
   N_("Unset the statement deletion_date.")},
  {"get", (PyCFunction) statement_get, METH_NOARGS,
   N_("Get the fields of the statement.")},
  {"compare", (PyCFunction) statement_compare, METH_VARARGS,
   N_("Compare the N3 representation of the terms in self "
      "and the terms in the other statement, in the specified "
      "lexicographic order. Return a negative value is self "
      "comes first, a positive value if self comes "
      "last, and 0 if they are equal.")},
  {NULL}
};

static PyTypeObject adftool_type_statement = {
  /* *INDENT-OFF* */
  PyVarObject_HEAD_INIT (NULL, 0)
  /* *INDENT-ON* */
  .tp_name = "adftool.Statement",
  .tp_doc = N_("Implementation of a RDF statement: "
	       "subject, predicate, object, graph, " "and deletion date."),
  .tp_basicsize = sizeof (struct adftool_py_statement),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_new = statement_new,
  .tp_init = statement_init,
  .tp_dealloc = statement_dealloc,
  .tp_members = adftool_statement_members,
  .tp_methods = adftool_statement_methods
};

static PyMemberDef adftool_fir_members[] = {
  {NULL}
};

static PyMethodDef adftool_fir_methods[] = {
  {"order", (PyCFunction) fir_order, METH_NOARGS,
   N_("Get the (odd) number of coefficients of the filter.")},
  {"coefficients", (PyCFunction) fir_coefficients, METH_NOARGS,
   N_("Get the coefficients of the filter as a list.")},
  {"design_bandpass", (PyCFunction) fir_design_bandpass, METH_VARARGS,
   N_("Set up the filter as band-pass.")},
  {"apply", (PyCFunction) fir_apply, METH_VARARGS,
   N_("Apply the filter.")},
  {"auto_bandwidth", (PyCFunction) fir_auto_bandwidth,
   METH_VARARGS | METH_STATIC, N_("Compute default transition bandwidth")},
  {"auto_order", (PyCFunction) fir_auto_order, METH_VARARGS | METH_STATIC,
   N_("Compute a default filter order")},
  {NULL}
};

static PyTypeObject adftool_type_fir = {
  /* *INDENT-OFF* */
  PyVarObject_HEAD_INIT (NULL, 0)
  /* *INDENT-ON* */
  .tp_name = "adftool.Fir",
  .tp_doc = N_("Implementation of a " "finite impulse response filter."),
  .tp_basicsize = sizeof (struct adftool_py_fir),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_new = fir_new,
  .tp_init = fir_init,
  .tp_dealloc = fir_dealloc,
  .tp_members = adftool_fir_members,
  .tp_methods = adftool_fir_methods
};

static PyMethodDef adftool_methods[] = {
  {"lytonepal", (PyCFunction) lytonepal, METH_VARARGS,
   N_("Get the full URI for a Lytonepal concept")},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef adftool_module = {
  .m_base = PyModuleDef_HEAD_INIT,
  .m_name = "adftool",
  .m_doc = N_("Manipulate linked data in HDF5 ADF files"),
  .m_size = -1,
  .m_methods = adftool_methods
};

static void
initialize_libadftool_python (void)
{
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  for (size_t i = 0;
       i + 1 < sizeof (adftool_methods) / sizeof (adftool_methods[0]); i++)
    {
      adftool_methods[i].ml_doc =
	dgettext (PACKAGE, adftool_methods[i].ml_doc);
    }
  adftool_module.m_doc = dgettext (PACKAGE, adftool_module.m_doc);
  adftool_type_term.tp_doc = dgettext (PACKAGE, adftool_type_term.tp_doc);
  for (size_t i = 0;
       i + 1 <
       sizeof (adftool_term_members) / sizeof (adftool_term_members[0]); i++)
    {
      adftool_term_members[i].doc =
	dgettext (PACKAGE, adftool_term_members[i].doc);
    }
  for (size_t i = 0;
       i + 1 <
       sizeof (adftool_term_methods) / sizeof (adftool_term_methods[0]); i++)
    {
      adftool_term_methods[i].ml_doc =
	dgettext (PACKAGE, adftool_term_methods[i].ml_doc);
    }
  adftool_type_file.tp_doc = dgettext (PACKAGE, adftool_type_file.tp_doc);
  for (size_t i = 0;
       i + 1 <
       sizeof (adftool_file_members) / sizeof (adftool_file_members[0]); i++)
    {
      adftool_file_members[i].doc =
	dgettext (PACKAGE, adftool_file_members[i].doc);
    }
  for (size_t i = 0;
       i + 1 <
       sizeof (adftool_file_methods) / sizeof (adftool_file_methods[0]); i++)
    {
      adftool_file_methods[i].ml_doc =
	dgettext (PACKAGE, adftool_file_methods[i].ml_doc);
    }
  adftool_type_statement.tp_doc =
    dgettext (PACKAGE, adftool_type_statement.tp_doc);
  for (size_t i = 0;
       i + 1 <
       sizeof (adftool_statement_members) /
       sizeof (adftool_statement_members[0]); i++)
    {
      adftool_statement_members[i].doc =
	dgettext (PACKAGE, adftool_statement_members[i].doc);
    }
  for (size_t i = 0;
       i + 1 <
       sizeof (adftool_statement_methods) /
       sizeof (adftool_statement_methods[0]); i++)
    {
      adftool_statement_methods[i].ml_doc =
	dgettext (PACKAGE, adftool_statement_methods[i].ml_doc);
    }
  adftool_type_fir.tp_doc = dgettext (PACKAGE, adftool_type_fir.tp_doc);
  for (size_t i = 0;
       i + 1 <
       sizeof (adftool_fir_members) / sizeof (adftool_fir_members[0]); i++)
    {
      adftool_fir_members[i].doc =
	dgettext (PACKAGE, adftool_fir_members[i].doc);
    }
  for (size_t i = 0;
       i + 1 <
       sizeof (adftool_fir_methods) / sizeof (adftool_fir_methods[0]); i++)
    {
      adftool_fir_methods[i].ml_doc =
	dgettext (PACKAGE, adftool_fir_methods[i].ml_doc);
    }
}

LIBADFTOOL_PYTHON_API PyMODINIT_FUNC
PyInit_adftool (void)
{
  initialize_libadftool_python ();
  if (PyType_Ready (&adftool_type_file) < 0)
    {
      return NULL;
    }
  if (PyType_Ready (&adftool_type_term) < 0)
    {
      return NULL;
    }
  if (PyType_Ready (&adftool_type_statement) < 0)
    {
      return NULL;
    }
  if (PyType_Ready (&adftool_type_fir) < 0)
    {
      return NULL;
    }
  PyObject *m = PyModule_Create (&adftool_module);
  if (m == NULL)
    {
      return NULL;
    }
  adftool_io_error = PyErr_NewException ("adftool.io_error", NULL, NULL);
  Py_XINCREF (adftool_io_error);
  adftool_rdf_error = PyErr_NewException ("adftool.rdf_error", NULL, NULL);
  Py_XINCREF (adftool_rdf_error);
  Py_INCREF (&adftool_type_term);
  Py_INCREF (&adftool_type_file);
  Py_INCREF (&adftool_type_statement);
  Py_INCREF (&adftool_type_fir);
  int term_error =
    PyModule_AddObject (m, "Term", (PyObject *) & adftool_type_term);
  int file_error =
    PyModule_AddObject (m, "File", (PyObject *) & adftool_type_file);
  int statement_error = PyModule_AddObject (m, "Statement",
					    (PyObject *) &
					    adftool_type_statement);
  int fir_error = PyModule_AddObject (m, "Fir",
				      (PyObject *) & adftool_type_fir);
  int io_error = PyModule_AddObject (m, "io_error", adftool_io_error);
  int rdf_error = PyModule_AddObject (m, "rdf_error", adftool_rdf_error);
  if (term_error < 0 || file_error < 0 || statement_error < 0 || io_error < 0
      || rdf_error < 0 || fir_error < 0)
    {
      Py_DECREF (&adftool_type_term);
      Py_DECREF (&adftool_type_file);
      Py_DECREF (&adftool_type_statement);
      Py_DECREF (&adftool_type_fir);
      Py_XDECREF (adftool_io_error);
      Py_CLEAR (adftool_io_error);
      Py_XDECREF (adftool_rdf_error);
      Py_CLEAR (adftool_rdf_error);
      Py_DECREF (m);
      return NULL;
    }
  return m;
}

static PyObject *
lytonepal (PyObject *self, PyObject * args)
{
  const char *c_concept;
  if (PyArg_ParseTuple (args, "s", &c_concept) == 0)
    {
      return NULL;
    }
  size_t required = adftool_lytonepal (c_concept, 0, 0, NULL);
  char *c_full = malloc (required + 1);
  if (c_full == NULL)
    {
      return NULL;
    }
  const size_t ck_required =
    adftool_lytonepal (c_concept, 0, required + 1, c_full);
  assert (ck_required == required);
  PyObject *full = PyUnicode_FromString (c_full);
  free (c_full);
  return full;
}

static PyObject *
file_new (PyTypeObject * subtype, PyObject * args, PyObject * kwds)
{
  (void) args;
  (void) kwds;
  struct adftool_py_file *self =
    (struct adftool_py_file *) subtype->tp_alloc (subtype, 0);
  if (self != NULL)
    {
      self->ptr = NULL;
    }
  return (PyObject *) self;
}

static int
file_init (PyObject * self, PyObject * args, PyObject * kwds)
{
  (void) self;
  (void) args;
  (void) kwds;
  return 0;
}

static void
file_dealloc (PyObject * self)
{
  struct adftool_py_file *aself = (struct adftool_py_file *) self;
  adftool_file_close (aself->ptr);
}

static PyObject *
file_open (struct adftool_py_file *self, PyObject * args)
{
  const char *filename;
  int write;
  if (PyArg_ParseTuple (args, "sp", &filename, &write) == 0)
    {
      return NULL;
    }
  struct adftool_file *new_file = adftool_file_open (filename, write);
  if (new_file)
    {
      adftool_file_close (self->ptr);
      self->ptr = new_file;
      Py_INCREF (Py_None);
      return Py_None;
    }
  else
    {
      if (write)
	{
	  PyErr_SetString (adftool_io_error,
			   _("Cannot open that file. "
			     "Does the parent directory exist?"));
	}
      else
	{
	  PyErr_SetString (adftool_io_error,
			   _("Cannot open that file. "
			     "Are you sure that this is an existing "
			     "valid ADF file?"));
	}
      return NULL;
    }
}

static PyObject *
file_close (struct adftool_py_file *self, PyObject * args)
{
  (void) args;
  adftool_file_close (self->ptr);
  self->ptr = NULL;
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
file_open_data (struct adftool_py_file *self, PyObject * args)
{
  const char *initial_data;
  Py_ssize_t initial_size;
  if (PyArg_ParseTuple (args, "s#", &initial_data, &initial_size) == 0)
    {
      return NULL;
    }
  struct adftool_file *new_file =
    adftool_file_open_data (initial_size, initial_data);
  if (new_file)
    {
      adftool_file_close (self->ptr);
      self->ptr = new_file;
      Py_INCREF (Py_None);
      return Py_None;
    }
  else
    {
      PyErr_SetString (adftool_io_error, _("Cannot open that virtual file."));
      return NULL;
    }
}

static PyObject *
file_open_generated (struct adftool_py_file *self, PyObject * args)
{
  (void) args;
  struct adftool_file *new_file = adftool_file_open_generated ();
  if (new_file)
    {
      adftool_file_close (self->ptr);
      self->ptr = new_file;
      Py_INCREF (Py_None);
      return Py_None;
    }
  else
    {
      PyErr_SetString (adftool_io_error, _("Cannot generate a new EEG."));
      return NULL;
    }
}

static PyObject *
file_get_data (struct adftool_py_file *self, PyObject * args)
{
  Py_ssize_t start;
  Py_buffer destination;
  if (PyArg_ParseTuple (args, "nw*", &start, &destination) == 0)
    {
      return NULL;
    }
  if (start < 0)
    {
      PyErr_SetString (adftool_io_error, _("The file offset is negative."));
      return NULL;
    }
  assert (start >= 0);
  assert (!(destination.readonly));
  assert (destination.len >= 0);
  size_t ret = adftool_file_get_data (self->ptr, start, destination.len,
				      destination.buf);
  return PyLong_FromSsize_t (ret);
}

static PyObject *
lookup (struct adftool_py_file *self, PyObject * args)
{
  struct adftool_py_statement *pattern;
  PyListObject *container;
  Py_ssize_t start;
  int ok = PyArg_ParseTuple (args, "O!nO!", &adftool_type_statement,
			     (PyObject **) & pattern, &start, &PyList_Type,
			     (PyObject **) & container);
  if (!ok)
    {
      return NULL;
    }
  if (start < 0)
    {
      return NULL;
    }
  Py_ssize_t py_max = PyList_Size ((PyObject *) container);
  assert (py_max >= 0);
  size_t max = py_max;
  struct adftool_statement **results =
    malloc (max * sizeof (struct adftool_statement *));
  if (results == NULL)
    {
      return NULL;
    }
  int all_statements = 1;
  for (size_t i = 0; i < max; i++)
    {
      PyObject *nextitem = PyList_GetItem ((PyObject *) container, i);
      int check =
	PyObject_IsInstance (nextitem, (PyObject *) & adftool_type_statement);
      if (check <= 0)
	{
	  all_statements = 0;
	}
      else
	{
	  struct adftool_py_statement *result =
	    (struct adftool_py_statement *) nextitem;
	  results[i] = result->ptr;
	}
    }
  size_t n_results = 0;
  if (all_statements)
    {
      int error =
	adftool_lookup (self->ptr, pattern->ptr, start, max, &n_results,
			results);
      if (error)
	{
	  PyErr_SetString (adftool_io_error,
			   _("Cannot look up data in the file."));
	  n_results = 0;
	}
    }
  free (results);
  return PyLong_FromSsize_t (n_results);
}

static PyObject *
lookup_objects (struct adftool_py_file *self, PyObject * args)
{
  struct adftool_py_term *subject;
  const char *predicate;
  Py_ssize_t start;
  PyListObject *container;
  int ok = PyArg_ParseTuple (args, "O!snO!", &adftool_type_term,
			     (PyObject **) & subject, &predicate, &start,
			     &PyList_Type, (PyObject **) & container);
  if (!ok)
    {
      return NULL;
    }
  if (start < 0)
    {
      return NULL;
    }
  Py_ssize_t py_max = PyList_Size ((PyObject *) container);
  assert (py_max >= 0);
  size_t max = py_max;
  struct adftool_term **results =
    malloc (max * sizeof (struct adftool_term *));
  if (results == NULL)
    {
      return NULL;
    }
  int all_terms = 1;
  for (size_t i = 0; i < max; i++)
    {
      PyObject *nextitem = PyList_GetItem ((PyObject *) container, i);
      int check =
	PyObject_IsInstance (nextitem, (PyObject *) & adftool_type_term);
      if (check <= 0)
	{
	  all_terms = 0;
	}
      else
	{
	  struct adftool_py_term *result =
	    (struct adftool_py_term *) nextitem;
	  results[i] = result->ptr;
	}
    }
  size_t n_results = 0;
  if (all_terms)
    {
      n_results =
	adftool_lookup_objects (self->ptr, subject->ptr, predicate, start,
				max, results);
    }
  free (results);
  return PyLong_FromSsize_t (n_results);
}

static PyObject *
lookup_integer (struct adftool_py_file *self, PyObject * args)
{
  struct adftool_py_term *subject;
  const char *predicate;
  Py_ssize_t start;
  PyListObject *container;
  int ok = PyArg_ParseTuple (args, "O!snO!", &adftool_type_term,
			     (PyObject **) & subject, &predicate, &start,
			     &PyList_Type, (PyObject **) & container);
  if (!ok)
    {
      return NULL;
    }
  if (start < 0)
    {
      return NULL;
    }
  Py_ssize_t py_max = PyList_Size ((PyObject *) container);
  assert (py_max >= 0);
  size_t max = py_max;
  long *results = malloc (max * sizeof (long));
  if (results == NULL)
    {
      return NULL;
    }
  size_t n_results =
    adftool_lookup_integer (self->ptr, subject->ptr, predicate, start, max,
			    results);
  for (size_t i = 0; i < max && i + start < n_results; i++)
    {
      PyList_SetItem ((PyObject *) container, i,
		      PyLong_FromLong (results[i]));
    }
  free (results);
  return PyLong_FromSsize_t (n_results);
}

static PyObject *
lookup_double (struct adftool_py_file *self, PyObject * args)
{
  struct adftool_py_term *subject;
  const char *predicate;
  Py_ssize_t start;
  PyListObject *container;
  int ok = PyArg_ParseTuple (args, "O!snO!", &adftool_type_term,
			     (PyObject **) & subject, &predicate, &start,
			     &PyList_Type, (PyObject **) & container);
  if (!ok)
    {
      return NULL;
    }
  if (start < 0)
    {
      return NULL;
    }
  Py_ssize_t py_max = PyList_Size ((PyObject *) container);
  assert (py_max >= 0);
  size_t max = py_max;
  double *results = malloc (max * sizeof (double));
  if (results == NULL)
    {
      return NULL;
    }
  size_t n_results =
    adftool_lookup_double (self->ptr, subject->ptr, predicate, start, max,
			   results);
  for (size_t i = 0; i < max && i + start < n_results; i++)
    {
      PyList_SetItem ((PyObject *) container, i,
		      PyFloat_FromDouble (results[i]));
    }
  free (results);
  return PyLong_FromSsize_t (n_results);
}

static PyObject *
lookup_date (struct adftool_py_file *self, PyObject * args)
{
  struct adftool_py_term *subject;
  const char *predicate;
  Py_ssize_t start;
  PyListObject *container;
  int ok = PyArg_ParseTuple (args, "O!snO!", &adftool_type_term,
			     (PyObject **) & subject, &predicate, &start,
			     &PyList_Type, (PyObject **) & container);
  if (!ok)
    {
      return NULL;
    }
  if (start < 0)
    {
      return NULL;
    }
  Py_ssize_t py_max = PyList_Size ((PyObject *) container);
  assert (py_max >= 0);
  size_t max = py_max;
  struct timespec *results_values = malloc (max * sizeof (struct timespec));
  struct timespec **results = malloc (max * sizeof (struct timespec *));
  if (results == NULL || results_values == NULL)
    {
      free (results);
      free (results_values);
      return NULL;
    }
  for (size_t i = 0; i < max; i++)
    {
      results[i] = &(results_values[i]);
    }
  size_t n_results =
    adftool_lookup_date (self->ptr, subject->ptr, predicate, start, max,
			 results);
  for (size_t i = 0; i < max && i + start < n_results; i++)
    {
      PyList_SetItem ((PyObject *) container, i,
		      Py_BuildValue ("(kk)", results_values[i].tv_sec,
				     results_values[i].tv_nsec));
    }
  free (results);
  free (results_values);
  return PyLong_FromSsize_t (n_results);
}

static PyObject *
lookup_string (struct adftool_py_file *self, PyObject * args)
{
  struct adftool_py_term *subject;
  const char *predicate;
  Py_ssize_t start;
  PyListObject *container;
  int ok = PyArg_ParseTuple (args, "O!snO!", &adftool_type_term,
			     (PyObject **) & subject, &predicate, &start,
			     &PyList_Type, (PyObject **) & container);
  if (!ok)
    {
      return NULL;
    }
  if (start < 0)
    {
      return NULL;
    }
  Py_ssize_t py_max = PyList_Size ((PyObject *) container);
  assert (py_max >= 0);
  size_t max = py_max;
  size_t storage_required;
  size_t n_results =
    adftool_lookup_string (self->ptr, subject->ptr, predicate,
			   &storage_required, 0, NULL, 0, 0, NULL, NULL, NULL,
			   NULL);
  const size_t storage_allocated = storage_required;
  char *storage = malloc (storage_allocated);
  size_t *langtag_length = malloc (max * sizeof (size_t));
  char **langtag = malloc (max * sizeof (char *));
  size_t *value_length = malloc (max * sizeof (size_t));
  char **value = malloc (max * sizeof (char *));
  if (storage == NULL || langtag_length == NULL || langtag == NULL
      || value_length == NULL || value == NULL)
    {
      free (value);
      free (value_length);
      free (langtag);
      free (langtag_length);
      free (storage);
      return NULL;
    }
  size_t check_storage_required;
  size_t check_n_results =
    adftool_lookup_string (self->ptr, subject->ptr, predicate,
			   &check_storage_required, storage_allocated,
			   storage, start, max, langtag_length, langtag,
			   value_length, value);
  assert (check_n_results == n_results);
  assert (check_storage_required == storage_required);
  for (size_t i = 0; i < max && i + start < n_results; i++)
    {
      PyObject *py_langtag = NULL;
      if (langtag[i] == NULL)
	{
	  Py_INCREF (Py_None);
	  py_langtag = Py_None;
	}
      else
	{
	  py_langtag =
	    PyBytes_FromStringAndSize (langtag[i], langtag_length[i]);
	}
      PyObject *py_value =
	PyBytes_FromStringAndSize (value[i], value_length[i]);
      PyList_SetItem ((PyObject *) container, i,
		      Py_BuildValue ("(OO)", py_value, py_langtag));
    }
  free (value);
  free (value_length);
  free (langtag);
  free (langtag_length);
  free (storage);
  return PyLong_FromSsize_t (n_results);
}

static PyObject *
lookup_subjects (struct adftool_py_file *self, PyObject * args)
{
  struct adftool_py_term *object;
  const char *predicate;
  Py_ssize_t start;
  PyListObject *container;
  int ok = PyArg_ParseTuple (args, "O!snO!", &adftool_type_term,
			     (PyObject **) & object, &predicate, &start,
			     &PyList_Type, (PyObject **) & container);
  if (!ok)
    {
      return NULL;
    }
  if (start < 0)
    {
      return NULL;
    }
  Py_ssize_t py_max = PyList_Size ((PyObject *) container);
  assert (py_max >= 0);
  size_t max = py_max;
  struct adftool_term **results =
    malloc (max * sizeof (struct adftool_term *));
  if (results == NULL)
    {
      return NULL;
    }
  int all_terms = 1;
  for (size_t i = 0; i < max; i++)
    {
      PyObject *nextitem = PyList_GetItem ((PyObject *) container, i);
      int check =
	PyObject_IsInstance (nextitem, (PyObject *) & adftool_type_term);
      if (check <= 0)
	{
	  all_terms = 0;
	}
      else
	{
	  struct adftool_py_term *result =
	    (struct adftool_py_term *) nextitem;
	  results[i] = result->ptr;
	}
    }
  size_t n_results = 0;
  if (all_terms)
    {
      n_results =
	adftool_lookup_subjects (self->ptr, object->ptr, predicate, start,
				 max, results);
    }
  free (results);
  return PyLong_FromSsize_t (n_results);
}

static PyObject *
delete (struct adftool_py_file *self, PyObject * args)
{
  struct adftool_py_statement *pattern;
  unsigned long deletion_date;
  int ok = PyArg_ParseTuple (args, "O!k", &adftool_type_statement,
			     (PyObject **) & pattern, &deletion_date);
  if (!ok)
    {
      return NULL;
    }
  int error = adftool_delete (self->ptr, pattern->ptr, deletion_date);
  if (error)
    {
      PyErr_SetString (adftool_io_error, _("Cannot modify the file."));
      return NULL;
    }
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
insert (struct adftool_py_file *self, PyObject * args)
{
  struct adftool_py_statement *statement;
  int ok = PyArg_ParseTuple (args, "O!", &adftool_type_statement,
			     (PyObject **) & statement);
  if (!ok)
    {
      return NULL;
    }
  int error = adftool_insert (self->ptr, statement->ptr);
  if (error)
    {
      PyErr_SetString (adftool_io_error, _("Cannot modify the file."));
      return NULL;
    }
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
find_channel_identifier (struct adftool_py_file *self, PyObject * args)
{
  Py_ssize_t channel_index;
  struct adftool_py_term *term;
  int ok = PyArg_ParseTuple (args, "nO!", &channel_index, &adftool_type_term,
			     (PyObject **) & term);
  if (!ok)
    {
      return NULL;
    }
  int error =
    adftool_find_channel_identifier (self->ptr, channel_index, term->ptr);
  return PyBool_FromLong (error == 0);
}

static PyObject *
get_channel_column (struct adftool_py_file *self, PyObject * args)
{
  struct adftool_py_term *term;
  int ok =
    PyArg_ParseTuple (args, "O!", &adftool_type_term, (PyObject **) & term);
  if (!ok)
    {
      return NULL;
    }
  size_t column;
  int error = adftool_get_channel_column (self->ptr, term->ptr, &column);
  if (error)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }
  return PyLong_FromSsize_t (column);
}

static PyObject *
add_channel_type (struct adftool_py_file *self, PyObject * args)
{
  struct adftool_py_term *channel;
  struct adftool_py_term *type;
  int ok = PyArg_ParseTuple (args, "O!O!", &adftool_type_term,
			     (PyObject **) & channel, &adftool_type_term,
			     (PyObject **) & type);
  if (!ok)
    {
      return NULL;
    }
  int error = adftool_add_channel_type (self->ptr, channel->ptr, type->ptr);
  if (error)
    {
      PyErr_SetString (adftool_io_error, _("Cannot modify the file."));
      return NULL;
    }
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
get_channel_types (struct adftool_py_file *self, PyObject * args)
{
  struct adftool_py_term *channel;
  Py_ssize_t start;
  PyListObject *container;
  int ok = PyArg_ParseTuple (args, "O!nO!", &adftool_type_term,
			     (PyObject **) & channel, &start,
			     &PyList_Type, (PyObject **) & container);
  if (!ok)
    {
      return NULL;
    }
  if (start < 0)
    {
      return NULL;
    }
  Py_ssize_t py_max = PyList_Size ((PyObject *) container);
  assert (py_max >= 0);
  size_t max = py_max;
  struct adftool_term **results =
    malloc (max * sizeof (struct adftool_term *));
  if (results == NULL)
    {
      return NULL;
    }
  int all_terms = 1;
  for (size_t i = 0; i < max; i++)
    {
      PyObject *nextitem = PyList_GetItem ((PyObject *) container, i);
      int check =
	PyObject_IsInstance (nextitem, (PyObject *) & adftool_type_term);
      if (check <= 0)
	{
	  all_terms = 0;
	}
      else
	{
	  struct adftool_py_term *result =
	    (struct adftool_py_term *) nextitem;
	  results[i] = result->ptr;
	}
    }
  size_t n_results = 0;
  if (all_terms)
    {
      n_results =
	adftool_get_channel_types (self->ptr, channel->ptr, start, max,
				   results);
    }
  free (results);
  return PyLong_FromSsize_t (n_results);
}

static PyObject *
find_channels_by_type (struct adftool_py_file *self, PyObject * args)
{
  struct adftool_py_term *type;
  Py_ssize_t start;
  PyListObject *container;
  int ok = PyArg_ParseTuple (args, "O!snO!", &adftool_type_term,
			     (PyObject **) & type, &start,
			     &PyList_Type, (PyObject **) & container);
  if (!ok)
    {
      return NULL;
    }
  if (start < 0)
    {
      return NULL;
    }
  Py_ssize_t py_max = PyList_Size ((PyObject *) container);
  assert (py_max >= 0);
  size_t max = py_max;
  struct adftool_term **results =
    malloc (max * sizeof (struct adftool_term *));
  if (results == NULL)
    {
      return NULL;
    }
  int all_terms = 1;
  for (size_t i = 0; i < max; i++)
    {
      PyObject *nextitem = PyList_GetItem ((PyObject *) container, i);
      int check =
	PyObject_IsInstance (nextitem, (PyObject *) & adftool_type_term);
      if (check <= 0)
	{
	  all_terms = 0;
	}
      else
	{
	  struct adftool_py_term *result =
	    (struct adftool_py_term *) nextitem;
	  results[i] = result->ptr;
	}
    }
  size_t n_results = 0;
  if (all_terms)
    {
      n_results =
	adftool_find_channels_by_type (self->ptr, type->ptr, start, max,
				       results);
    }
  free (results);
  return PyLong_FromSsize_t (n_results);
}

static PyObject *
eeg_get_data (struct adftool_py_file *self, PyObject * args)
{
  (void) args;
  /* This function gets all the data. */
  size_t n_times, n_channels, time_check, chan_check;
  int error =
    adftool_eeg_get_data (self->ptr, 0, 0, &n_times, 0, 0, &n_channels, NULL);
  if (error)
    {
      PyErr_SetString (adftool_io_error, _("No EEG data in the file."));
      return NULL;
    }
  double *data = malloc (n_times * n_channels * sizeof (double));
  if (data == NULL)
    {
      return NULL;
    }
  error =
    adftool_eeg_get_data (self->ptr, 0, n_times, &time_check, 0, n_channels,
			  &chan_check, data);
  if (error)
    {
      PyErr_SetString (adftool_io_error, _("No EEG data in the file."));
      free (data);
      return NULL;
    }
  PyObject *ret = PyList_New (n_times * n_channels);
  for (size_t i = 0; i < n_times * n_channels; i++)
    {
      error = PyList_SetItem (ret, i, PyFloat_FromDouble (data[i]));
      assert (error == 0);
    }
  free (data);
  PyObject *full = Py_BuildValue ("(nnO)", n_times, n_channels, ret);
  Py_DECREF (ret);
  return full;
}

static PyObject *
eeg_set_data (struct adftool_py_file *self, PyObject * args)
{
  Py_ssize_t n_times, n_channels;
  PyListObject *container;
  int ok = PyArg_ParseTuple (args, "nnO!", &n_times, &n_channels,
			     &PyList_Type, (PyObject **) & container);
  if (!ok)
    {
      return NULL;
    }
  if (n_times < 0 || n_channels < 0)
    {
      return NULL;
    }
  size_t max = PyList_Size ((PyObject *) container);
  double *c_data = malloc (n_times * n_channels * sizeof (double));
  if (c_data == NULL)
    {
      return NULL;
    }
  const size_t end = n_times * n_channels;
  for (size_t i = 0; i < end; i++)
    {
      if (i <= max)
	{
	  c_data[i] =
	    PyFloat_AsDouble (PyList_GetItem ((PyObject *) container, i));
	}
      else
	{
	  c_data[i] = 0;
	}
    }
  int error;
  if (PyErr_Occurred ())
    {
      error = 1;
    }
  else
    {
      error = adftool_eeg_set_data (self->ptr, n_times, n_channels, c_data);
    }
  free (c_data);
  if (PyErr_Occurred ())
    {
      return NULL;
    }
  if (error)
    {
      PyErr_SetString (adftool_io_error, _("Cannot modify the file."));
      return NULL;
    }
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
eeg_get_time (struct adftool_py_file *self, PyObject * args)
{
  (void) args;
  struct timespec time;
  double sampling_frequency;
  int error = adftool_eeg_get_time (self->ptr, 0, &time, &sampling_frequency);
  if (error)
    {
      PyErr_SetString (adftool_io_error, _("No EEG time in the file."));
      return NULL;
    }
  return Py_BuildValue ("((kk)d)", time.tv_sec, time.tv_nsec,
			sampling_frequency);
}

static PyObject *
eeg_set_time (struct adftool_py_file *self, PyObject * args)
{
  unsigned long tv_sec, tv_nsec;
  double sfreq;
  int ok = PyArg_ParseTuple (args, "(kk)d", &tv_sec, &tv_nsec,
			     &sfreq);
  if (!ok)
    {
      return NULL;
    }
  const struct timespec t = {.tv_sec = tv_sec,.tv_nsec = tv_nsec };
  int error = adftool_eeg_set_time (self->ptr, &t, sfreq);
  if (error)
    {
      PyErr_SetString (adftool_io_error, _("Cannot modify the file."));
      return NULL;
    }
  return Py_BuildValue ("((kk)d)", t.tv_sec, t.tv_nsec, sfreq);
}

static PyObject *
term_new (PyTypeObject * subtype, PyObject * args, PyObject * kwds)
{
  (void) args;
  (void) kwds;
  struct adftool_py_term *self =
    (struct adftool_py_term *) subtype->tp_alloc (subtype, 0);
  if (self != NULL)
    {
      self->ptr = adftool_term_alloc ();
      if (self->ptr == NULL)
	{
	  Py_DECREF (self);
	  return NULL;
	}
    }
  return (PyObject *) self;
}

static int
term_init (PyObject * self, PyObject * args, PyObject * kwds)
{
  (void) self;
  (void) args;
  (void) kwds;
  return 0;
}

static void
term_dealloc (PyObject * self)
{
  struct adftool_py_term *aself = (struct adftool_py_term *) self;
  adftool_term_free (aself->ptr);
}

static PyObject *
term_copy (struct adftool_py_term *self, PyObject * args)
{
  struct adftool_py_term *other;
  if (PyArg_ParseTuple (args, "O!", &adftool_type_term, (PyObject **) & other)
      == 0)
    {
      return NULL;
    }
  adftool_term_copy (self->ptr, other->ptr);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
term_set_named (struct adftool_py_term *self, PyObject * args)
{
  const char *id;
  if (PyArg_ParseTuple (args, "s", &id) == 0)
    {
      return NULL;
    }
  adftool_term_set_named (self->ptr, id);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
term_set_blank (struct adftool_py_term *self, PyObject * args)
{
  const char *id;
  if (PyArg_ParseTuple (args, "s", &id) == 0)
    {
      return NULL;
    }
  adftool_term_set_blank (self->ptr, id);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
term_set_literal (struct adftool_py_term *self, PyObject * args)
{
  const char *value;
  const char *type;
  const char *langtag;
  if (PyArg_ParseTuple (args, "szz", &value, &type, &langtag) == 0)
    {
      return NULL;
    }
  if (type != NULL && langtag != NULL)
    {
      PyErr_SetString (adftool_rdf_error,
		       _("A literal cannot have both "
			 "a type and a langtag. " "Please choose."));
      return NULL;
    }
  adftool_term_set_literal (self->ptr, value, type, langtag);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
term_set_integer (struct adftool_py_term *self, PyObject * args)
{
  long number;
  if (PyArg_ParseTuple (args, "l", &number) == 0)
    {
      return NULL;
    }
  mpz_t value;
  mpz_init_set_si (value, number);
  adftool_term_set_mpz (self->ptr, value);
  mpz_clear (value);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
term_set_double (struct adftool_py_term *self, PyObject * args)
{
  double number;
  if (PyArg_ParseTuple (args, "d", &number) == 0)
    {
      return NULL;
    }
  mpf_t value;
  mpf_init_set_d (value, number);
  adftool_term_set_mpf (self->ptr, value);
  mpf_clear (value);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
term_set_date (struct adftool_py_term *self, PyObject * args)
{
  unsigned long sec, nsec;
  if (PyArg_ParseTuple (args, "(kk)", &sec, &nsec) == 0)
    {
      return NULL;
    }
  const struct timespec t = {.tv_sec = sec,.tv_nsec = nsec };
  adftool_term_set_date (self->ptr, &t);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
term_is_named (struct adftool_py_term *self, PyObject * args)
{
  (void) args;
  return PyBool_FromLong (adftool_term_is_named (self->ptr));
}

static PyObject *
term_is_blank (struct adftool_py_term *self, PyObject * args)
{
  (void) args;
  return PyBool_FromLong (adftool_term_is_blank (self->ptr));
}

static PyObject *
term_is_literal (struct adftool_py_term *self, PyObject * args)
{
  (void) args;
  return PyBool_FromLong (adftool_term_is_literal (self->ptr));
}

static PyObject *
term_is_typed_literal (struct adftool_py_term *self, PyObject * args)
{
  (void) args;
  return PyBool_FromLong (adftool_term_is_typed_literal (self->ptr));
}

static PyObject *
term_is_langstring (struct adftool_py_term *self, PyObject * args)
{
  (void) args;
  return PyBool_FromLong (adftool_term_is_langstring (self->ptr));
}

static PyObject *
term_value (struct adftool_py_term *self, PyObject * args)
{
  Py_ssize_t start;
  Py_buffer destination;
  if (PyArg_ParseTuple (args, "nw*", &start, &destination) == 0)
    {
      return NULL;
    }
  if (start < 0)
    {
      start = 0;
    }
  assert (start >= 0);
  assert (!(destination.readonly));
  assert (destination.len >= 0);
  size_t ret =
    adftool_term_value (self->ptr, start, destination.len, destination.buf);
  return PyLong_FromSsize_t (ret);
}

static PyObject *
term_meta (struct adftool_py_term *self, PyObject * args)
{
  Py_ssize_t start;
  Py_buffer destination;
  if (PyArg_ParseTuple (args, "nw*", &start, &destination) == 0)
    {
      return NULL;
    }
  if (start < 0)
    {
      start = 0;
    }
  assert (start >= 0);
  assert (!(destination.readonly));
  assert (destination.len >= 0);
  size_t ret =
    adftool_term_meta (self->ptr, start, destination.len, destination.buf);
  return PyLong_FromSsize_t (ret);
}

static PyObject *
term_as_integer (struct adftool_py_term *self, PyObject * args)
{
  (void) args;
  mpz_t value;
  mpz_init (value);
  int error = adftool_term_as_mpz (self->ptr, value);
  long ret = 0;
  if (error == 0)
    {
      ret = mpz_get_si (value);
    }
  mpz_clear (value);
  if (error)
    {
      PyErr_SetString (adftool_rdf_error,
		       _("The term is not a number literal."));
      return NULL;
    }
  return PyLong_FromLong (ret);
}

static PyObject *
term_as_double (struct adftool_py_term *self, PyObject * args)
{
  (void) args;
  mpf_t value;
  mpf_init (value);
  int error = adftool_term_as_mpf (self->ptr, value);
  double ret = 0;
  if (error == 0)
    {
      ret = mpf_get_d (value);
    }
  mpf_clear (value);
  if (error)
    {
      PyErr_SetString (adftool_rdf_error,
		       _("The term is not a number literal."));
      return NULL;
    }
  return PyFloat_FromDouble (ret);
}

static PyObject *
term_as_date (struct adftool_py_term *self, PyObject * args)
{
  (void) args;
  struct timespec t;
  int error = adftool_term_as_date (self->ptr, &t);
  if (error)
    {
      PyErr_SetString (adftool_rdf_error,
		       _("The term is not a date literal."));
      return NULL;
    }
  return Py_BuildValue ("(kk)", t.tv_sec, t.tv_nsec);
}

static PyObject *
term_compare (struct adftool_py_term *self, PyObject * args)
{
  struct adftool_py_term *other;
  if (PyArg_ParseTuple (args, "O!", &adftool_type_term, (PyObject **) & other)
      == 0)
    {
      return NULL;
    }
  int ret = adftool_term_compare (self->ptr, other->ptr);
  return PyLong_FromLong (ret);
}

static PyObject *
term_parse_n3 (struct adftool_py_term *self, PyObject * args)
{
  const char *text;
  Py_ssize_t text_size;
  if (PyArg_ParseTuple (args, "s#", &text, &text_size) == 0)
    {
      return NULL;
    }
  if (text_size < 0)
    {
      text_size = 0;
    }
  size_t consumed;
  int error = adftool_term_parse_n3 (text, text_size, &consumed, self->ptr);
  if (error)
    {
      PyErr_SetString (adftool_rdf_error, _("The term is not in N3."));
      return NULL;
    }
  return PyLong_FromSsize_t (consumed);
}

static PyObject *
term_to_n3 (struct adftool_py_term *self, PyObject * args)
{
  Py_ssize_t start;
  Py_buffer destination;
  if (PyArg_ParseTuple (args, "nw*", &start, &destination) == 0)
    {
      return NULL;
    }
  if (start < 0)
    {
      start = 0;
    }
  assert (start >= 0);
  assert (!(destination.readonly));
  assert (destination.len >= 0);
  size_t ret =
    adftool_term_to_n3 (self->ptr, start, destination.len, destination.buf);
  return PyLong_FromSsize_t (ret);
}

static PyObject *
statement_new (PyTypeObject * subtype, PyObject * args, PyObject * kwds)
{
  (void) args;
  (void) kwds;
  struct adftool_py_statement *self =
    (struct adftool_py_statement *) subtype->tp_alloc (subtype, 0);
  if (self != NULL)
    {
      self->ptr = adftool_statement_alloc ();
      if (self->ptr == NULL)
	{
	  Py_DECREF (self);
	  return NULL;
	}
    }
  return (PyObject *) self;
}

static int
statement_init (PyObject * self, PyObject * args, PyObject * kwds)
{
  (void) self;
  (void) args;
  (void) kwds;
  return 0;
}

static void
statement_dealloc (PyObject * self)
{
  struct adftool_py_statement *aself = (struct adftool_py_statement *) self;
  adftool_statement_free (aself->ptr);
}

static PyObject *
statement_set_subject (struct adftool_py_statement *self, PyObject * args)
{
  struct adftool_py_term *term;
  if (PyArg_ParseTuple (args, "O!", &adftool_type_term, (PyObject **) & term)
      == 0)
    {
      return NULL;
    }
  adftool_statement_set (self->ptr, &(term->ptr), NULL, NULL, NULL, NULL);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
statement_unset_subject (struct adftool_py_statement *self, PyObject * args)
{
  (void) args;
  struct adftool_term *term = NULL;
  adftool_statement_set (self->ptr, &term, NULL, NULL, NULL, NULL);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
statement_set_predicate (struct adftool_py_statement *self, PyObject * args)
{
  struct adftool_py_term *term;
  if (PyArg_ParseTuple (args, "O!", &adftool_type_term, (PyObject **) & term)
      == 0)
    {
      return NULL;
    }
  adftool_statement_set (self->ptr, NULL, &(term->ptr), NULL, NULL, NULL);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
statement_unset_predicate (struct adftool_py_statement *self, PyObject * args)
{
  (void) args;
  struct adftool_term *term = NULL;
  adftool_statement_set (self->ptr, NULL, &term, NULL, NULL, NULL);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
statement_set_object (struct adftool_py_statement *self, PyObject * args)
{
  struct adftool_py_term *term;
  if (PyArg_ParseTuple (args, "O!", &adftool_type_term, (PyObject **) & term)
      == 0)
    {
      return NULL;
    }
  adftool_statement_set (self->ptr, NULL, NULL, &(term->ptr), NULL, NULL);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
statement_unset_object (struct adftool_py_statement *self, PyObject * args)
{
  (void) args;
  struct adftool_term *term = NULL;
  adftool_statement_set (self->ptr, NULL, NULL, &term, NULL, NULL);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
statement_set_graph (struct adftool_py_statement *self, PyObject * args)
{
  struct adftool_py_term *term;
  if (PyArg_ParseTuple (args, "O!", &adftool_type_term, (PyObject **) & term)
      == 0)
    {
      return NULL;
    }
  adftool_statement_set (self->ptr, NULL, NULL, NULL, &(term->ptr), NULL);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
statement_unset_graph (struct adftool_py_statement *self, PyObject * args)
{
  (void) args;
  struct adftool_term *term = NULL;
  adftool_statement_set (self->ptr, NULL, NULL, NULL, &term, NULL);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
statement_set_deletion_date (struct adftool_py_statement *self,
			     PyObject * args)
{
  unsigned long d;
  uint64_t date;
  if (PyArg_ParseTuple (args, "k", &d) == 0)
    {
      return NULL;
    }
  date = d;
  adftool_statement_set (self->ptr, NULL, NULL, NULL, NULL, &date);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
statement_unset_deletion_date (struct adftool_py_statement *self,
			       PyObject * args)
{
  (void) args;
  uint64_t date = (uint64_t) (-1);
  adftool_statement_set (self->ptr, NULL, NULL, NULL, NULL, &date);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
build_for_get (const struct adftool_term *object)
{
  if (PyType_Ready (&adftool_type_term) < 0)
    {
      return NULL;
    }
  PyObject *constructor = (PyObject *) & adftool_type_term;
  if (object == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }
  struct adftool_py_term *t;
  PyObject *constructed =
    PyObject_CallObject (constructor, Py_BuildValue ("()"));
  t = (struct adftool_py_term *) constructed;
  if (t == NULL)
    {
      return NULL;
    }
  adftool_term_copy (t->ptr, object);
  Py_INCREF ((PyObject *) t);
  return (PyObject *) t;
}

static PyObject *
statement_get (struct adftool_py_statement *self, PyObject * args)
{
  (void) args;
  struct adftool_term *s;
  struct adftool_term *p;
  struct adftool_term *o;
  struct adftool_term *g;
  uint64_t date;
  adftool_statement_get (self->ptr, &s, &p, &o, &g, &date);
  PyObject *ps = build_for_get (s);
  PyObject *pp = build_for_get (p);
  PyObject *po = build_for_get (o);
  PyObject *pg = build_for_get (g);
  if (ps == NULL || pp == NULL || po == NULL || pg == NULL)
    {
      return NULL;
    }
  PyObject *ret;
  if (date == ((uint64_t) (-1)))
    {
      ret = Py_BuildValue ("(OOOOO)", ps, pp, po, pg, Py_None);
    }
  else
    {
      ret = Py_BuildValue ("(OOOOk)", ps, pp, po, pg, date);
    }
  Py_DECREF (ps);
  Py_DECREF (pp);
  Py_DECREF (po);
  Py_DECREF (pg);
  return ret;
}

static PyObject *
statement_compare (struct adftool_py_statement *self, PyObject * args)
{
  const char *order;
  struct adftool_py_statement *other;
  if (PyArg_ParseTuple
      (args, "O!s", &adftool_type_statement, (PyObject **) & other,
       &order) == 0)
    {
      return NULL;
    }
  int ret = adftool_statement_compare (self->ptr, other->ptr, order);
  return PyLong_FromLong (ret);
}

static PyObject *
fir_new (PyTypeObject * subtype, PyObject * args, PyObject * kwds)
{
  (void) kwds;
  Py_ssize_t order;
  if (PyArg_ParseTuple (args, "n", &order) == 0)
    {
      return NULL;
    }
  struct adftool_py_fir *self =
    (struct adftool_py_fir *) subtype->tp_alloc (subtype, 0);
  if (self != NULL)
    {
      self->ptr = adftool_fir_alloc (order);
      if (self->ptr == NULL)
	{
	  Py_DECREF (self);
	  return NULL;
	}
    }
  return (PyObject *) self;
}

static int
fir_init (PyObject * self, PyObject * args, PyObject * kwds)
{
  (void) self;
  (void) args;
  (void) kwds;
  return 0;
}

static void
fir_dealloc (PyObject * self)
{
  struct adftool_py_fir *aself = (struct adftool_py_fir *) self;
  adftool_fir_free (aself->ptr);
}

static PyObject *
fir_order (struct adftool_py_fir *self, PyObject * args)
{
  (void) args;
  size_t order = adftool_fir_order (self->ptr);
  return PyLong_FromSsize_t (order);
}

static PyObject *
fir_coefficients (struct adftool_py_fir *self, PyObject * args)
{
  (void) args;
  const size_t order = adftool_fir_order (self->ptr);
  double *output = malloc (order * sizeof (double));
  if (output == NULL)
    {
      return NULL;
    }
  adftool_fir_coefficients (self->ptr, output);
  PyObject *ret = PyList_New (order);
  for (size_t i = 0; i < order; i++)
    {
      int error = PyList_SetItem (ret, i, PyFloat_FromDouble (output[i]));
      assert (error == 0);
    }
  free (output);
  return ret;
}

static PyObject *
fir_design_bandpass (struct adftool_py_fir *self, PyObject * args)
{
  double sfreq, low, high, trans_low, trans_high;
  if (PyArg_ParseTuple
      (args, "ddddd", &sfreq, &low, &high, &trans_low, &trans_high) == 0)
    {
      return NULL;
    }
  adftool_fir_design_bandpass (self->ptr, sfreq, low, high, trans_low,
			       trans_high);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
fir_apply (struct adftool_py_fir *self, PyObject * args)
{
  PyListObject *py_input;
  int ok = PyArg_ParseTuple (args, "O!",
			     &PyList_Type, (PyObject **) & py_input);
  if (!ok)
    {
      return NULL;
    }
  size_t n = PyList_Size ((PyObject *) py_input);
  double *input = malloc (n * sizeof (double));
  double *output = malloc (n * sizeof (double));
  if (input == NULL || output == NULL)
    {
      free (input);
      free (output);
      return NULL;
    }
  for (size_t i = 0; i < n; i++)
    {
      input[i] = PyFloat_AsDouble (PyList_GetItem ((PyObject *) py_input, i));
      if (PyErr_Occurred ())
	{
	  free (input);
	  free (output);
	  return NULL;
	}
    }
  adftool_fir_apply (self->ptr, n, input, output);
  free (input);
  PyObject *ret = PyList_New (n);
  for (size_t i = 0; i < n; i++)
    {
      int error = PyList_SetItem (ret, i, PyFloat_FromDouble (output[i]));
      assert (error == 0);
    }
  free (output);
  return ret;
}

static PyObject *
fir_auto_bandwidth (struct adftool_py_fir *self, PyObject * args)
{
  assert (self == NULL);
  double sfreq, freq_low, freq_high, trans_low, trans_high;
  int ok = PyArg_ParseTuple (args, "ddd", &sfreq, &freq_low, &freq_high);
  if (!ok)
    {
      return NULL;
    }
  adftool_fir_auto_bandwidth (sfreq, freq_low, freq_high, &trans_low,
			      &trans_high);
  return Py_BuildValue ("(dd)", trans_low, trans_high);
}

static PyObject *
fir_auto_order (struct adftool_py_fir *self, PyObject * args)
{
  assert (self == NULL);
  double sfreq, bw;
  int ok = PyArg_ParseTuple (args, "dd", &sfreq, &bw);
  if (!ok)
    {
      return NULL;
    }
  const size_t order = adftool_fir_auto_order (sfreq, bw);
  return PyLong_FromSsize_t (order);
}
