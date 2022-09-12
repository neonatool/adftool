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

/* Write term as N3 to stdout, unless, it is an empty named node and
   ignore_empty_name. (used to ignore the default graph) */
static void printf_term (const struct adftool_term *term,
			 int ignore_empty_name, int print_space);

static void lookup_results_by_page (const struct adftool_file *file,
				    const struct adftool_statement *pattern,
				    size_t page_size,
				    struct adftool_statement **results,
				    const struct timespec *current_time);

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
  static int find_channel_identifier = 0;
  static size_t channel_column = 0;
  struct adftool_term *channel_identifier = adftool_term_alloc ();
  static int get_channel_metadata = 0;
  static int add_channel_type = 0;
  struct adftool_term *channel_type = adftool_term_alloc ();
  static int list_channels_of_type = 0;
  static int get_eeg_metadata = 0;
  static int set_eeg_date = 0;
  struct timespec eeg_date = { 0, 0 };
  double eeg_sampling_frequency = 0;
  char *example_date_format;
  if (channel_identifier == NULL || channel_type == NULL)
    {
      abort ();
    }
  if (1)
    {
      struct timespec now;
      if (timespec_get (&now, TIME_UTC) != TIME_UTC)
	{
	  now.tv_sec = now.tv_nsec = 0;
	}
      struct adftool_term *literal_now = adftool_term_alloc ();
      if (literal_now == NULL)
	{
	  abort ();
	}
      adftool_term_set_date (literal_now, &now);
      size_t n = adftool_term_value (literal_now, 0, 0, NULL);
      example_date_format = malloc (n + 1);
      if (example_date_format == NULL)
	{
	  abort ();
	}
      size_t check =
	adftool_term_value (literal_now, 0, n + 1, example_date_format);
      if (check != n)
	{
	  abort ();
	}
      adftool_term_free (literal_now);
    }
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
    {NP_ ("Command-line|Option|", "find-channel-identifier"),
     required_argument, NULL, 256},
    {NP_ ("Command-line|Option|", "channel-metadata"),
     required_argument, NULL, 258},
    {NP_ ("Command-line|Option|", "add-channel-type"),
     required_argument, NULL, 259},
    {NP_ ("Command-line|Option|", "channels-of-type"),
     required_argument, NULL, 260},
    {NP_ ("Command-line|Option|", "eeg-metadata"),
     no_argument, &get_eeg_metadata, 261},
    {NP_ ("Command-line|Option|", "set-eeg-date"),
     required_argument, NULL, 262},
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
	case 256:
	  /* --find-channel-identifier=COLUMN */
	  {
	    char *end;
	    channel_column = strtol (optarg, &end, 0);
	    while (*end == ' ' || *end == '\r' || *end == '\t'
		   || *end == '\n')
	      {
		end++;
	      }
	    if (end == optarg || strcmp (end, "") != 0)
	      {
		fprintf (stderr, _("The argument to \"%s\" "
				   "must be a number.\n"),
			 long_options[option_index].name);
		exit (1);
	      }
	    find_channel_identifier = 1;
	  }
	  break;
	case 258:
	  {
	    /* --channel-metadata=IDENTIFIER */
	    size_t consumed;
	    int parse_error =
	      adftool_term_parse_n3 (optarg, strlen (optarg), &consumed,
				     channel_identifier);
	    if (parse_error == 0)
	      {
		while (consumed < strlen (optarg)
		       && (optarg[consumed] == ' '
			   || optarg[consumed] == '\r'
			   || optarg[consumed] == '\t'
			   || optarg[consumed] == '\n'))
		  {
		    consumed++;
		  }
	      }
	    if (parse_error != 0 || consumed != strlen (optarg))
	      {
		fprintf (stderr, _("The argument to \"%s\" "
				   "must be a N3 identifier.\n"),
			 long_options[option_index].name);
		exit (1);
	      }
	    get_channel_metadata = 1;
	  }
	  break;
	case 259:
	  {
	    /* --add-channel-type=IDENTIFIER=TYPE */
	    size_t consumed;
	    int parse_error =
	      adftool_term_parse_n3 (optarg, strlen (optarg), &consumed,
				     channel_identifier);
	    if (parse_error)
	      {
		fprintf (stderr, _("The argument to \"%s\" "
				   "must be in the form of "
				   "IDENTIFIER=TYPE.\n"),
			 long_options[option_index].name);
		exit (1);
	      }
	    while (consumed < strlen (optarg)
		   && (optarg[consumed] == ' '
		       || optarg[consumed] == '\r'
		       || optarg[consumed] == '\t'
		       || optarg[consumed] == '\n'))
	      {
		consumed++;
	      }
	    if (consumed >= strlen (optarg) || optarg[consumed] != '=')
	      {
		fprintf (stderr, _("The argument to \"%s\" "
				   "must be in the form of "
				   "IDENTIFIER=TYPE.\n"),
			 long_options[option_index].name);
		exit (1);
	      }
	    char *rest = optarg + consumed + strlen ("=");
	    parse_error =
	      adftool_term_parse_n3 (rest, strlen (rest), &consumed,
				     channel_type);
	    if (parse_error)
	      {
		fprintf (stderr, _("The argument to \"%s\" "
				   "must be in the form of "
				   "IDENTIFIER=TYPE.\n"),
			 long_options[option_index].name);
		exit (1);
	      }
	    while (consumed < strlen (rest)
		   && (rest[consumed] == ' '
		       || rest[consumed] == '\r'
		       || rest[consumed] == '\t' || rest[consumed] == '\n'))
	      {
		consumed++;
	      }
	    if (consumed != strlen (rest))
	      {
		fprintf (stderr, _("The argument to \"%s\" "
				   "must be in the form of "
				   "IDENTIFIER=TYPE.\n"),
			 long_options[option_index].name);
		exit (1);
	      }
	    add_channel_type = 1;
	  }
	  break;
	case 260:
	  {
	    /* --channels-of-type=TYPE */
	    size_t consumed;
	    int parse_error =
	      adftool_term_parse_n3 (optarg, strlen (optarg), &consumed,
				     channel_type);
	    if (parse_error)
	      {
		fprintf (stderr, _("The argument to \"%s\" "
				   "must be a N3 identifier.\n"),
			 long_options[option_index].name);
		exit (1);
	      }
	    while (consumed < strlen (optarg)
		   && (optarg[consumed] == ' '
		       || optarg[consumed] == '\r'
		       || optarg[consumed] == '\t'
		       || optarg[consumed] == '\n'))
	      {
		consumed++;
	      }
	    if (consumed < strlen (optarg))
	      {
		fprintf (stderr, _("The argument to \"%s\" "
				   "must be a N3 identifier.\n"),
			 long_options[option_index].name);
		exit (1);
	      }
	    list_channels_of_type = 1;
	  }
	  break;
	case 262:
	  /* --set-eeg-date=DATE,SAMPLING_FREQUENCY */
	  {
	    char *separator = strchr (optarg, ',');
	    if (separator == NULL)
	      {
		fprintf (stderr, _("The argument to \"%s\" "
				   "must be in the form of "
				   "DATE,SAMPLING_FREQUENCY.\n"),
			 long_options[option_index].name);
		exit (1);
	      }
	    char *left = malloc (separator - optarg + 1);
	    char *right = malloc (strlen (separator + 1) + 1);
	    if (left == NULL || right == NULL)
	      {
		abort ();
	      }
	    memcpy (left, optarg, separator - optarg);
	    left[separator - optarg] = '\0';
	    strcpy (right, separator + 1);
	    struct adftool_term *literal = adftool_term_alloc ();
	    if (literal == NULL)
	      {
		abort ();
	      }
	    static const char *type =
	      "http://www.w3.org/2001/XMLSchema#dateTime";
	    adftool_term_set_literal (literal, left, type, NULL);
	    if (adftool_term_as_date (literal, &eeg_date) != 0)
	      {
		fprintf (stderr, _("The DATE argument to \"%s\" "
				   "must be a date according "
				   "to XSD.\n"),
			 long_options[option_index].name);
		exit (1);
	      }
	    adftool_term_free (literal);
	    char *number_end;
	    eeg_sampling_frequency = strtod (right, &number_end);
	    while (*number_end == ' ' || *number_end == '\r'
		   || *number_end == '\t' || *number_end == '\n')
	      {
		number_end++;
	      }
	    if (number_end == right || strcmp (number_end, "") != 0)
	      {
		fprintf (stderr, _("The SAMPLING_FREQUENCY argument "
				   "to \"%s\" "
				   "must be a number.\n"),
			 long_options[option_index].name);
		exit (1);
	      }
	    free (right);
	    free (left);
	    set_eeg_date = 1;
	  }
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
	  printf (_("There are other operation modes for adftool:\n"));
	  printf (_("  --%s: read the raw EEG sensor data (in Tab-Separated "
		    "Value [TSV] format);\n"),
		  P_ ("Command-line|Option|", "get-eeg-data"));
	  printf (_("  --%s: set the raw EEG sensor data (in TSV format) "
		    "from the standard input;\n"),
		  P_ ("Command-line|Option|", "set-eeg-data"));
	  printf (_("  --%s: read the EEG metadata;\n"),
		  P_ ("Command-line|Option|", "eeg-metadata"));
	  printf (_("  --%s=DATE,SAMPLING_FREQUENCY: set the EEG date "
		    "and sampling frequency (DATE is in the format of "
		    "%s, and SAMPLING_FREQUENCY in the locale numeric "
		    "format, %f);\n"),
		  P_ ("Command-line|Option|", "set-eeg-date"),
		  example_date_format, 256.0);
	  printf (_("  --%s=COLUMN: find the channel identifier for the "
		    "raw sensor data in COLUMN (an integer);\n"),
		  P_ ("Command-line|Option|", "find-channel-identifier"));
	  printf (_("  --%s=IDENTIFIER: read the channel metadata "
		    "for IDENTIFIER (in N3);\n"),
		  P_ ("Command-line|Option|", "channel-metadata"));
	  printf (_("  --%s=IDENTIFIER=TYPE: add TYPE to the list of "
		    "types for IDENTIFIER (both in N3);\n"),
		  P_ ("Command-line|Option|", "add-channel-type"));
	  printf (_("  --%s=TYPE: print the list of channels of type "
		    "TYPE (in N3).\n"),
		  P_ ("Command-line|Option|", "channels-of-type"));
	  printf ("\n");
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
  if (!lookup && !insert && !remove && !get_eeg_data && !set_eeg_data
      && !find_channel_identifier
      && !get_channel_metadata && !add_channel_type && !list_channels_of_type
      && !get_eeg_metadata && !set_eeg_date)
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
  if (find_channel_identifier +
      get_channel_metadata + add_channel_type + list_channels_of_type > 1)
    {
      fprintf (stderr,
	       _("Conflicting operations: please pass either --%s, "
		 "--%s, --%s "
		 "or --%s.\n"),
	       P_ ("Command-line|Option|", "find-channel-identifier"),
	       P_ ("Command-line|Option|", "channel-metadata"),
	       P_ ("Command-line|Option|", "add-channel-type"),
	       P_ ("Command-line|Option|", "channels-of-type"));
      exit (0);
    }
  while (optind < argc)
    {
      int write = insert || remove || set_eeg_data
	|| add_channel_type || set_eeg_date;
      const char *filename = argv[optind++];
      if (adftool_file_open (file, filename, write) != 0)
	{
	  fprintf (stderr, _("The file \"%s\" could not be opened.\n"),
		   filename);
	  exit (1);
	}
      if (lookup)
	{
	  size_t page_size = 256;
	  struct adftool_statement **buffer =
	    malloc (page_size * sizeof (struct adftool_statement *));
	  if (buffer == NULL)
	    {
	      abort ();
	    }
	  for (size_t i = 0; i < page_size; i++)
	    {
	      buffer[i] = adftool_statement_alloc ();
	      if (buffer[i] == NULL)
		{
		  abort ();
		}
	    }
	  struct timespec current_time;
	  if (timespec_get (&current_time, TIME_UTC) != TIME_UTC)
	    {
	      fprintf (stderr, _("Error: could not get the current time.\n"));
	    }
	  lookup_results_by_page (file, pattern, page_size, buffer,
				  &current_time);
	  for (size_t i = 0; i < page_size; i++)
	    {
	      adftool_statement_free (buffer[i]);
	    }
	  free (buffer);
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
	  free (line);
	  int error =
	    adftool_eeg_set_data (file, current_line, n_columns, buffer);
	  if (error != 0)
	    {
	      fprintf (stderr, _("Error: cannot set the EEG data.\n"));
	      exit (1);
	    }
	  free (buffer);
	}
      if (find_channel_identifier)
	{
	  struct adftool_term *term = adftool_term_alloc ();
	  if (term == NULL)
	    {
	      abort ();
	    }
	  int error =
	    adftool_find_channel_identifier (file, channel_column, term);
	  if (error)
	    {
	      fprintf (stderr,
		       _("Error: no channel identifier for column %lu.\n"),
		       channel_column);
	      exit (1);
	    }
	  printf_term (term, 0, 0);
	  printf ("\n");
	  free (term);
	}
      if (get_channel_metadata)
	{
	  size_t column;
	  int error = 0;
	  size_t required =
	    adftool_term_to_n3 (channel_identifier, 0, 0, NULL);
	  char *identifier = malloc (required + 1);
	  if (identifier == NULL)
	    {
	      abort ();
	    }
	  adftool_term_to_n3 (channel_identifier, 0, required + 1,
			      identifier);
	  printf (_("Metadata of channel %s:\n"), identifier);
	  error =
	    adftool_get_channel_column (file, channel_identifier, &column);
	  if (error == 0)
	    {
	      printf (_("  - its data are in column %lu;\n"), column);
	    }
	  else
	    {
	      printf (_("  - its data cannot be found;\n"));
	    }
	  size_t n_types =
	    adftool_get_channel_types (file, channel_identifier, 0, 0, NULL);
	  struct adftool_term **types =
	    malloc (n_types * sizeof (struct adftool_term *));
	  if (types == NULL)
	    {
	      abort ();
	    }
	  for (size_t i = 0; i < n_types; i++)
	    {
	      types[i] = adftool_term_alloc ();
	      if (types[i] == NULL)
		{
		  abort ();
		}
	    }
	  adftool_get_channel_types (file, channel_identifier, 0, n_types,
				     types);
	  if (n_types == 0)
	    {
	      printf (_("  - it has no types.\n"));
	    }
	  else
	    {
	      printf (ngettext ("  - it has %lu type:\n",
				"  - it has %lu types:\n", n_types), n_types);
	      for (size_t i = 0; i < n_types; i++)
		{
		  size_t n3_length =
		    adftool_term_to_n3 (types[i], 0, 0, NULL);
		  char *n3 = malloc (n3_length + 1);
		  if (n3 == NULL)
		    {
		      abort ();
		    }
		  if (adftool_term_to_n3 (types[i], 0, n3_length + 1, n3) !=
		      n3_length)
		    {
		      abort ();
		    }
		  printf (_("    %s\n"), n3);
		  free (n3);
		}
	    }
	  for (size_t i = 0; i < n_types; i++)
	    {
	      adftool_term_free (types[i]);
	    }
	  free (types);
	  free (identifier);
	}
      if (add_channel_type)
	{
	  int error =
	    adftool_add_channel_type (file, channel_identifier, channel_type);
	  if (error)
	    {
	      fprintf (stderr, _("Error: could not add a new type.\n"));
	      exit (1);
	    }
	}
      if (list_channels_of_type)
	{
	  size_t n_channels =
	    adftool_find_channels_by_type (file, channel_type, 0, 0, NULL);
	  struct adftool_term **channels =
	    malloc (n_channels * sizeof (struct adftool_term *));
	  if (channels == NULL)
	    {
	      abort ();
	    }
	  for (size_t i = 0; i < n_channels; i++)
	    {
	      channels[i] = adftool_term_alloc ();
	      if (channels[i] == NULL)
		{
		  abort ();
		}
	    }
	  adftool_find_channels_by_type (file, channel_type, 0, n_channels,
					 channels);
	  for (size_t i = 0; i < n_channels; i++)
	    {
	      size_t n3_length = adftool_term_to_n3 (channels[i], 0, 0, NULL);
	      char *n3 = malloc (n3_length + 1);
	      if (n3 == NULL)
		{
		  abort ();
		}
	      if (adftool_term_to_n3 (channels[i], 0, n3_length + 1, n3) !=
		  n3_length)
		{
		  abort ();
		}
	      printf (_("%s\n"), n3);
	      free (n3);
	    }
	  for (size_t i = 0; i < n_channels; i++)
	    {
	      adftool_term_free (channels[i]);
	    }
	  free (channels);
	}
      if (get_eeg_metadata)
	{
	  struct timespec start_date;
	  double sfreq;
	  if (adftool_eeg_get_time (file, 0, &start_date, &sfreq) != 0)
	    {
	      printf (_("The EEG does not have "
			"a start date or a sampling frequency.\n"));
	    }
	  else
	    {
	      struct adftool_term *literal_date = adftool_term_alloc ();
	      if (literal_date == NULL)
		{
		  abort ();
		}
	      adftool_term_set_date (literal_date, &start_date);
	      size_t n = adftool_term_value (literal_date, 0, 0, NULL);
	      char *d = malloc (n + 1);
	      if (d == NULL)
		{
		  abort ();
		}
	      size_t ck = adftool_term_value (literal_date, 0, n + 1, d);
	      if (ck != n)
		{
		  abort ();
		}
	      printf (_("The EEG started at: %s, "
			"with a sampling frequency of %f Hz.\n"), d, sfreq);
	      free (d);
	      adftool_term_free (literal_date);
	    }
	}
      if (set_eeg_date)
	{
	  if (adftool_eeg_set_time (file, &eeg_date, eeg_sampling_frequency)
	      != 0)
	    {
	      fprintf (stderr, _("Could not set the EEG time.\n"));
	      exit (1);
	    }
	}
      adftool_file_close (file);
    }
  free (example_date_format);
  adftool_term_free (channel_type);
  adftool_term_free (channel_identifier);
  adftool_file_free (file);
  adftool_term_free (term);
  adftool_statement_free (pattern);
  return 0;
}

static void
printf_term (const struct adftool_term *term, int ignore_empty_name,
	     int print_space)
{
  char first_bytes[32];
  const size_t easy_n = sizeof (first_bytes) - 1;
  size_t length = adftool_term_to_n3 (term, 0, easy_n, first_bytes);
  first_bytes[easy_n] = '\0';
  if (ignore_empty_name && strcmp (first_bytes, "<>") == 0)
    {
      /* Do not print the default graph. */
      return;
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
      adftool_term_to_n3 (term, easy_n, length - easy_n + 1, rest);
      assert (rest[length - easy_n] == '\0');
      printf ("%s", rest);
      free (rest);
    }
  if (print_space)
    {
      printf (" ");
    }
}

static size_t
do_one_page (const struct adftool_file *file,
	     const struct adftool_statement *pattern, size_t start,
	     size_t length, struct adftool_statement **page,
	     uint64_t current_time)
{
  size_t n;
  if (adftool_lookup (file, pattern, start, length, &n, page) != 0)
    {
      fprintf (stderr, _("Cannot list the statements.\n"));
      exit (1);
    }
  size_t i;
  for (i = start; i < n && i - start < length; i++)
    {
      const struct adftool_statement *statement = page[i];
      uint64_t deletion_date;
      adftool_statement_get (statement, NULL, NULL, NULL, NULL,
			     &deletion_date);
      const int has_deletion_date = (deletion_date != ((uint64_t) (-1)));
      if (has_deletion_date && deletion_date <= current_time)
	{
	  /* Skip this statement. */
	}
      else
	{
	  struct adftool_term *terms[4];
	  adftool_statement_get (statement, &(terms[0]), &(terms[1]),
				 &(terms[2]), &(terms[3]), NULL);
	  if (terms[0] != NULL && terms[1] != NULL && terms[2] != NULL)
	    {
	      for (size_t i = 0; i < 4; i++)
		{
		  if (terms[i] != NULL)
		    {
		      printf_term (terms[i], i == 3, 1);
		    }
		}
	      printf (".\n");
	    }
	}
    }
  return i;
}

static void
lookup_results_by_page (const struct adftool_file *file,
			const struct adftool_statement *pattern,
			size_t page_size, struct adftool_statement **results,
			const struct timespec *current_time)
{
  struct tm tm_ref = {
    .tm_year = 70,		/* 1970 */
    .tm_mon = 0,		/* january */
    .tm_mday = 1,		/* 1st */
    .tm_hour = 0,
    .tm_min = 0,		/* midnight */
    .tm_sec = 0
  };
  const time_t time_ref = timegm (&tm_ref);
  const time_t elapsed_s = current_time->tv_sec - time_ref;
  const uint64_t elapsed_ms =
    elapsed_s * 1000 + (current_time->tv_nsec / 1000000);
  size_t start = 0;
  size_t n_done;
  while (1)
    {
      n_done =
	do_one_page (file, pattern, start, page_size, results, elapsed_ms);
      if (n_done == start)
	{
	  /* Everything done. */
	  return;
	}
      start = n_done;
    }
}
