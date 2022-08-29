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

static void print_uri (const char *value);

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
  static struct option long_options[] = {
    {NP_ ("Command-line|Option|", "lookup"), no_argument, &lookup,
     1},
    {NP_ ("Command-line|Option|", "add"), no_argument, &insert,
     1},
    {NP_ ("Command-line|Option|", "remove"), no_argument, &remove,
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
	getopt_long (argc, argv, "s:p:o:g:d:hv", long_options, &option_index);
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
	  printf (_("Usage: adftool [OPTION…] FILE\n\n"
		    "Read or update FILE.\n\n"
		    "You can set a pattern in N-Triples format:\n"
		    "  -s NT, --%s=NT;\n"
		    "  -p NT, --%s=NT;\n"
		    "  -o NT, --%s=NT;\n"
		    "  -g NT, --%s=NT;\n"
		    "\n"
		    "You use this pattern to:\n"
		    "  --%s: print the data matching the pattern;\n"
		    "  --%s: add the statement (only graph is optional);\n"
		    "  --%s: remove all statements matching the pattern.\n"
		    "\n"
		    "There are other options:\n"
		    "  -d DATE, --%s=DATE: use DATE instead of "
		    "the current date when deleting statements.\n"
		    "  -h, --%s: print this message and exit.\n"
		    "  -V, --%s: print the package version and exit.\n"
		    "\n"),
		  P_ ("Command-line|Option|", "subject"),
		  P_ ("Command-line|Option|", "predicate"),
		  P_ ("Command-line|Option|", "object"),
		  P_ ("Command-line|Option|", "graph"),
		  P_ ("Command-line|Option|", "lookup"),
		  P_ ("Command-line|Option|", "add"),
		  P_ ("Command-line|Option|", "remove"),
		  P_ ("Command-line|Option|", "deletion-date"),
		  P_ ("Command-line|Option|", "help"),
		  P_ ("Command-line|Option|", "version"));
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
  if (!lookup && !insert && !remove)
    {
      fprintf (stderr, _("Nothing to do.\n"));
      exit (0);
    }
  if (lookup + insert + remove > 1)
    {
      fprintf (stderr,
	       _
	       ("Conflicting operations: please pass either --lookup, --insert or --remove.\n"));
      exit (0);
    }
  while (optind < argc)
    {
      int write = insert || remove;
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
			      char *value = NULL;
			      char *meta = NULL;
			      size_t n_value, n_value_check;
			      size_t n_meta, n_meta_check;
			      n_value =
				adftool_term_value (terms[i], 0, 0, NULL);
			      n_meta =
				adftool_term_meta (terms[i], 0, 0, NULL);
			      value = malloc (n_value + 1);
			      meta = malloc (n_meta + 1);
			      if (value == NULL || meta == NULL)
				{
				  fprintf (stderr, _("\
Cannot allocate memory to hold the results.\n"));
				  exit (1);
				}
			      n_value_check =
				adftool_term_value (terms[i], 0, n_value + 1,
						    value);
			      n_meta_check =
				adftool_term_meta (terms[i], 0, n_meta + 1,
						   meta);
			      assert (n_value_check == n_value);
			      assert (n_meta_check == n_meta);
			      value[n_value] = '\0';
			      meta[n_meta] = '\0';
			      if (adftool_term_is_blank (terms[i]))
				{
				  printf ("_:%s ", value);
				}
			      else if (adftool_term_is_named (terms[i]))
				{
				  if (strcmp (meta, "") != 0)
				    {
				      fprintf (stderr,
					       _
					       ("I don’t support namespaced data yet.\n"));
				      exit (1);
				    }
				  print_uri (value);
				}
			      else
				{
				  printf ("\"");
				  for (size_t i = 0; i < n_value; i++)
				    {
				      switch (value[i])
					{
					case '\\':
					  printf ("\\\\");
					  break;
					case '"':
					  printf ("\\\"");
					  break;
					case '\r':
					  printf ("\\r");
					  break;
					case '\n':
					  printf ("\\n");
					  break;
					default:
					  printf ("%c", value[i]);
					}
				    }
				  printf ("\"");
				  if (adftool_term_is_typed_literal
				      (terms[i]))
				    {
				      printf ("^^");
				      print_uri (meta);
				    }
				  else
				    if (adftool_term_is_langstring (terms[i]))
				    {
				      printf ("@");
				      for (size_t i = 0; i < n_meta; i++)
					{
					  if ((meta[i] >= 'A'
					       && meta[i] <= 'Z')
					      || (meta[i] >= 'a'
						  && meta[i] <= 'z')
					      || (meta[i] >= '0'
						  && meta[i] <= '9')
					      || meta[i] == '-')
					    {
					      printf ("%c", meta[i]);
					    }
					}
				    }
				  else
				    {
				      abort ();
				    }
				}
			      free (meta);
			      free (value);
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
      adftool_file_close (file);
    }
  adftool_file_free (file);
  adftool_term_free (term);
  adftool_statement_free (pattern);
  return 0;
}

static void
print_uri (const char *value)
{
  printf ("<");
  for (size_t i = 0; i < strlen (value); i++)
    {
      if (value[i] == '>')
	{
	  printf ("%%3E");
	}
      else
	{
	  printf ("%c", value[i]);
	}
    }
  printf (">");
}
