#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "gettext.h"
#include "relocatable.h"
#include "progname.h"
#include <locale.h>
#include <adftool.h>
#include <getopt.h>
#include <time.h>
#include <assert.h>

#define _(String) gettext(String)
#define N_(String) (String)
#define NP_(Context, String) (String)

#if defined ENABLE_NLS && ENABLE_NLS
#define adftool_pgettext pgettext_expr
#define P_(Context, String) pgettext(Context, String)
#else
#define adftool_pgettext(Context, String) (String)
#define P_(Context, String) (String)
#endif

int
main (int argc, char *argv[])
{
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  struct adftool_statement *pattern = adftool_statement_alloc ();
  struct adftool_term *term = adftool_term_alloc ();
  struct adftool_file *file = adftool_file_alloc ();
  uint64_t deletion_date = ((uint64_t) time (NULL)) * 1000;
  if (pattern == NULL || term == NULL || file == NULL)
    {
      fprintf (stderr, _("Not enough memory.\n"));
      exit (1);
    }
  size_t consumed;
  static int lookup = 0;
  static int insert = 0;
  static int remove = 0;
  static int get_eeg_data = 0;
  static int set_eeg_data = 0;
  static struct option long_options[] = {
    {NP_ ("Command-line|Option|", "lookup"), no_argument, &lookup,
     1},
    {NP_ ("Command-line|Option|", "add"), no_argument, &insert,
     1},
    {NP_ ("Command-line|Option|", "remove"), no_argument, &remove,
     1},
    {NP_ ("Command-line|Option|", "get-eeg-data"), no_argument, &get_eeg_data,
     1},
    {NP_ ("Command-line|Option|", "set-eeg-data"), no_argument, &set_eeg_data,
     1},
    {NP_ ("Command-line|Option|", "subject"), required_argument,
     0, 's'},
    {NP_ ("Command-line|Option|", "predicate"),
     required_argument, 0, 'p'},
    {NP_ ("Command-line|Option|", "object"), required_argument,
     0, 'o'},
    {NP_ ("Command-line|Option|", "graph"), required_argument,
     0, 'g'},
    {NP_ ("Command-line|Option|", "deletion-date"), required_argument, 0,
     'd'},
    {NP_ ("Command-line|Option|", "help"), no_argument, 0, 'h'},
    {NP_ ("Command-line|Option|", "version"), no_argument, 0, 'V'},
    {0, 0, 0, 0}
  };
  for (size_t i = 0; i + 1 < sizeof (long_options) / sizeof (long_options[0]);
       i++)
    {
      long_options[i].name =
	adftool_pgettext ("Command-line|Option|", long_options[i].name);
    }
  while (1)
    {
      int option_index = 0;
      int c =
	getopt_long (argc, argv, "s:p:o:g:d:hV", long_options, &option_index);
      if (c == -1)
	{
	  break;
	}
      switch (c)
	{
	case 0:
	  /* The flag has been set. */
	  break;
	case 's':
	case 'p':
	case 'o':
	case 'g':
	  if (adftool_term_parse_n3 (optarg, strlen (optarg), &consumed, term)
	      != 0)
	    {
	      fprintf (stderr,
		       _("The argument to \"%s\" "
			 "cannot be parsed as a N-Triples term: \"%s\"\n"),
		       long_options[option_index].name, optarg);
	      exit (1);
	    }
	  while (consumed < strlen (optarg)
		 && (optarg[consumed] == ' '
		     || optarg[consumed] == '\t'
		     || optarg[consumed] == '\n' || optarg[consumed] == '\r'))
	    {
	      consumed++;
	    }
	  if (consumed < strlen (optarg))
	    {
	      fprintf (stderr,
		       _("Warning: the argument to \"%s\" "
			 "can be parsed as a N-Triples term, "
			 "but the trailing \"%s\" will be discarded.\n"),
		       long_options[option_index].name, optarg + consumed);
	    }
	  switch (c)
	    {
	    case 's':
	      adftool_statement_set (pattern, &term, NULL, NULL, NULL, NULL);
	      break;
	    case 'p':
	      adftool_statement_set (pattern, NULL, &term, NULL, NULL, NULL);
	      break;
	    case 'o':
	      adftool_statement_set (pattern, NULL, NULL, &term, NULL, NULL);
	      break;
	    case 'g':
	      adftool_statement_set (pattern, NULL, NULL, NULL, &term, NULL);
	      break;
	    }
	  break;
	case 'd':
	  {
	    struct tm deletion_time;
	    memset (&deletion_time, 0, sizeof (deletion_time));
	    strptime (optarg, "%Y-%m-%dT%H:%M:%S%:z", &deletion_time);
	    time_t deletion_t = mktime (&deletion_time);
	    uint64_t deletion_s = (uint64_t) deletion_t;
	    deletion_date = deletion_s * 1000;
	  }
	  break;
	case 'h':
	  printf (_("Usage: [ENVIRONMENT…] adftool [OPTION…] FILE\n\n"
		    "Read or update FILE.\n\n"));
	  printf (_("You can set a pattern in N-Triples format:\n"
		    "  -s NT, --%s=NT;\n"
		    "  -p NT, --%s=NT;\n"
		    "  -o NT, --%s=NT;\n"
		    "  -g NT, --%s=NT.\n"
		    "\n"
		    "You use this pattern to:\n"
		    "  --%s: print the data matching the pattern;\n"
		    "  --%s: add the statement (only graph is optional);\n"
		    "  --%s: remove all statements matching the pattern.\n"
		    "\n"),
		  P_ ("Command-line|Option|", "subject"),
		  P_ ("Command-line|Option|", "predicate"),
		  P_ ("Command-line|Option|", "object"),
		  P_ ("Command-line|Option|", "graph"),
		  P_ ("Command-line|Option|", "lookup"),
		  P_ ("Command-line|Option|", "add"),
		  P_ ("Command-line|Option|", "remove"));
	  printf (_("There are other operation modes for adftool:\n"
		    "  --%s: read the raw EEG sensor data (in Tab-Separated "
		    "Value [TSV] format);\n"
		    "  --%s: set the raw EEG sensor data (in TSV format) "
		    "from the standard input.\n"
		    "\n"), P_ ("Command-line|Option|", "get-eeg-data"),
		  P_ ("Command-line|Option|", "set-eeg-data"));
	  printf (_("There are other options:\n"
		    "  -d DATE, --%s=DATE: use DATE instead of "
		    "the current date when deleting statements.\n"
		    "  -h, --%s: print this message and exit.\n"
		    "  -V, --%s: print the package version and exit.\n"
		    "\n"),
		  P_ ("Command-line|Option|", "deletion-date"),
		  P_ ("Command-line|Option|", "help"),
		  P_ ("Command-line|Option|", "version"));
	  printf (_("The following environment variables can change the "
		    "behavior of the program:\n"
		    "  LANG: change the localization. Set it as \"LANG=C\" "
		    "in the environment or \"LANG=en_US.UTF-8\" to disable "
		    "localization.\n"
		    "  LC_NUMERIC: change the expected number format to "
		    "input and output numbers. Set it as \"LC_NUMERIC=C\" "
		    "or \"LC_NUMERIC=en_US.UTF-8\" to use the English notation.\n"
		    "\n"));
	  printf (_("Please note that TSV exchange of raw EEG data uses "
		    "the current numeric locale format. For instance, "
		    "here is an approximation of π: \"%f\". You can control "
		    "the numeric format by setting the LC_NUMERIC environment "
		    "variable.\n"
		    "\n"
		    "For instance, if the EEG data has been generated within the "
		    "English locale, you would set it with the following "
		    "command-line:\n"
		    "  LC_NUMERIC=en_US.UTF-8 adftool --%s file.adf\n"
		    "And then specify the data in this format:\n"
		    "  0.01\t3.42e-4\t-1.18e2\n"
		    "  3.1416\t…\t…\n"
		    "\n"),
		  3.1416, P_ ("Command-line|Option|", "set-eeg-data"));
	  static const char *env_names[] = {
	    "LANG",
	    "LC_NUMERIC"
	  };
	  static const size_t n_env =
	    (sizeof (env_names) / sizeof (env_names[0]));
	  printf (_("Here is a summary of the current values "
		    "of the main environment variables:\n"));
	  for (size_t i = 0; i < n_env; i++)
	    {
	      char *value = getenv (env_names[i]);
	      if (value)
		{
		  printf (_("  %s: \"%s\"\n"), env_names[i], value);
		}
	      else
		{
		  printf (_("  %s is unset\n"), env_names[i]);
		}
	    }
	  exit (0);
	case 'V':
	  printf (_("%s\n"
		    "\n" "Copyright status is unclear.\n"), PACKAGE_STRING);
	  exit (0);
	case '?':
	  break;
	default:
	  abort ();
	}
    }
  if (optind == argc)
    {
      fprintf (stderr, _("No file to process.\n"));
      exit (0);
    }
  if (!lookup && !insert && !remove && !get_eeg_data && !set_eeg_data)
    {
      fprintf (stderr, _("Nothing to do.\n"));
      exit (0);
    }
  if (lookup + insert + remove + get_eeg_data + set_eeg_data > 1)
    {
      fprintf (stderr,
	       _("Conflicting operations: please pass either --%s, --%s, "
		 "--%s, --%s or --%s.\n"), P_ ("Command-line|Option|",
					       "lookup"),
	       P_ ("Command-line|Option|", "insert"),
	       P_ ("Command-line|Option|", "remove"),
	       P_ ("Command-line|Option|", "get-eeg-data"),
	       P_ ("Command-line|Option|", "set-eeg-data"));
      exit (0);
    }
  while (optind < argc)
    {
      int write = insert || remove || set_eeg_data;
      const char *filename = argv[optind++];
      if (adftool_file_open (file, filename, write) != 0)
	{
	  fprintf (stderr, _("The file \"%s\" could not be opened.\n"),
		   filename);
	  exit (1);
	}
      if (lookup)
	{
	  struct adftool_results *results = adftool_results_alloc ();
	  if (results == NULL)
	    {
	      fprintf (stderr,
		       _("Cannot allocate memory to hold the results.\n"));
	      exit (1);
	    }
	  uint64_t current_time = ((uint64_t) time (NULL)) * 1000;
	  if (adftool_lookup (file, pattern, results) != 0)
	    {
	      fprintf (stderr, _("Cannot list the statements.\n"));
	      exit (1);
	    }
	  size_t n_results = adftool_results_count (results);
	  for (size_t i = 0; i < n_results; i++)
	    {
	      const struct adftool_statement *statement =
		adftool_results_get (results, i);
	      uint64_t deletion_date;
	      adftool_statement_get (statement, NULL, NULL, NULL, NULL,
				     &deletion_date);
	      const int has_deletion_date =
		(deletion_date != ((uint64_t) (-1)));
	      if (has_deletion_date && deletion_date <= current_time)
		{
		  /* Skip this statement. */
		}
	      else
		{
		  struct adftool_term *terms[4];
		  adftool_statement_get (statement, &(terms[0]), &(terms[1]),
					 &(terms[2]), &(terms[3]), NULL);
		  if (terms[0] != NULL && terms[1] != NULL
		      && terms[2] != NULL)
		    {
		      for (size_t i = 0; i < 4; i++)
			{
			  if (terms[i] != NULL)
			    {
			      char first_bytes[32];
			      const size_t easy_n = sizeof (first_bytes) - 1;
			      size_t length =
				adftool_term_to_n3 (terms[i], 0, easy_n,
						    first_bytes);
			      first_bytes[easy_n] = '\0';
			      if (i == 3 && strcmp (first_bytes, "<>") == 0)
				{
				  /* Do not print the default graph. */
				  break;
				}
			      printf ("%s", first_bytes);
			      if (length >= sizeof (first_bytes))
				{
				  char *rest = malloc (length - easy_n + 1);
				  if (rest == NULL)
				    {
				      fprintf (stderr, _("\
Cannot allocate memory to hold the results.\n"));
				      exit (1);
				    }
				  adftool_term_to_n3 (terms[i], easy_n,
						      length - easy_n + 1,
						      rest);
				  assert (rest[length - easy_n] == '\0');
				  printf ("%s", rest);
				  free (rest);
				}
			      printf (" ");
			    }
			}
		      printf (".\n");
		    }
		}
	    }
	  adftool_results_free (results);
	}
      if (insert)
	{
	  if (adftool_insert (file, pattern) != 0)
	    {
	      fprintf (stderr, _("Could not insert the data.\n"));
	    }
	}
      if (remove)
	{
	  if (adftool_delete (file, pattern, deletion_date) != 0)
	    {
	      fprintf (stderr, _("Could not delete the data.\n"));
	    }
	}
      if (get_eeg_data)
	{
	  size_t n_lines, n_columns;
	  int error =
	    adftool_eeg_get_data (file, 0, 0, &n_lines, 0, 0, &n_columns,
				  NULL);
	  if (error != 0)
	    {
	      fprintf (stderr, _("Could not read the EEG data.\n"));
	      exit (1);
	    }
	  double *data = calloc (n_lines * n_columns, sizeof (double));
	  if (data == NULL)
	    {
	      abort ();
	    }
	  size_t ck_n_lines, ck_n_columns;
	  if (adftool_eeg_get_data
	      (file, 0, n_lines, &ck_n_lines, 0, n_columns, &ck_n_columns,
	       data) != 0)
	    {
	      abort ();
	    }
	  assert (n_lines == ck_n_lines);
	  assert (n_columns == ck_n_columns);
	  for (size_t i = 0; i < n_lines; i++)
	    {
	      for (size_t j = 0; j < n_columns; j++)
		{
		  if (j != 0)
		    {
		      printf ("\t");
		    }
		  printf ("%.8f", data[i * n_columns + j]);
		}
	      printf ("\n");
	    }
	  free (data);
	}
      if (set_eeg_data)
	{
	  size_t n_columns = 0;
	  size_t max = 1;
	  double *buffer = malloc (max * sizeof (double));
	  if (buffer == NULL)
	    {
	      abort ();
	    }
	  size_t current_line = 0;
	  size_t current_column = 0;
	  char *line = NULL;
	  size_t current_line_length = 0;
	  ssize_t n_read;
	  size_t i = 0;
	  while ((n_read = getline (&line, &current_line_length, stdin)) >= 0)
	    {
	      current_column = 0;
	      char *beginning;
	      char *end = line;
	      while (*end == ' ' || *end == '\r' || *end == '\n')
		{
		  end++;
		}
	      do
		{
		  beginning = end;
		  const double next_value = strtod (beginning, &end);
		  while (*end == ' ' || *end == '\r' || *end == '\n')
		    {
		      end++;
		    }
		  if (end != beginning)
		    {
		      if (i >= max)
			{
			  max *= 2;
			  buffer = realloc (buffer, max * sizeof (double));
			  if (buffer == NULL)
			    {
			      abort ();
			    }
			}
		      buffer[i] = next_value;
		      i++;
		      current_column++;
		    }
		}
	      while (end != beginning);
	      if (strcmp (end, "") != 0)
		{
		  fprintf (stderr, _("Error: \
input line %lu contains \"%s\", which cannot be parsed \
as a number.\n"), current_line + 1, end);
		  if (*end == '.' && current_line == 0)
		    {
		      fprintf (stderr, _("You may need to set \
LC_NUMERIC. Please run adftool --%s to read more about numeric \
data formats.\n"), P_ ("Command-line|Option|", "help"));
		    }
		  exit (1);
		}
	      if (current_line == 0)
		{
		  n_columns = current_column;
		}
	      else if (current_column != n_columns)
		{
		  fprintf (stderr,
			   _("Error: input line %lu has %lu values, "
			     "but there are %lu columns.\n"),
			   current_line + 1, current_column, n_columns);
		  exit (1);
		}
	      free (line);
	      line = NULL;
	      current_line_length = 0;
	      current_line++;
	    }
	  int error =
	    adftool_eeg_set_data (file, current_line, n_columns, buffer);
	  if (error != 0)
	    {
	      fprintf (stderr, _("Error: cannot set the EEG data.\n"));
	      exit (1);
	    }
	  free (buffer);
	}
      adftool_file_close (file);
    }
  adftool_file_free (file);
  adftool_term_free (term);
  adftool_statement_free (pattern);
  return 0;
}
