#include <config.h>

#if defined _WIN32 && !defined __CYGWIN__
# define LIBADFTOOL_R_DLL_MADNESS 1
#else
# define LIBADFTOOL_R_DLL_MADNESS 0
#endif

#if BUILDING_LIBADFTOOL_R && HAVE_VISIBILITY
# define LIBADFTOOL_R_DLL_EXPORTED __attribute__((__visibility__("default")))
#elif BUILDING_LIBADFTOOL_R && LIBADFTOOL_R_DLL_MADNESS
# define LIBADFTOOL_R_DLL_EXPORTED __declspec(dllexport)
#elif LIBADFTOOL_R_DLL_MADNESS
# define LIBADFTOOL_R_DLL_EXPORTED __declspec(dllimport)
#else
# define LIBADFTOOL_R_DLL_EXPORTED
#endif

#define LIBADFTOOL_R_API \
  LIBADFTOOL_R_DLL_EXPORTED

/* On Windows, the "write" C function exists as "_write", so gnulib
   redefines "write" as "_write". However, Rcpp uses a few things from
   iostream, for instance std::ostream::write. This is a non-inline
   function, and suddently the C++ linker imports
   std::ostream::_write, which does not exist. Boom. */
#define GNULIB_NAMESPACE adftool_r_compat

#include <attribute.h>
#include <adftool.h>
#include <Rcpp.h>
#include <cmath>
#include <optional>
#include <array>
#include "gettext.h"
#include "relocatable.h"

#define _(String) dgettext (PACKAGE, (String))
#define N_(String) (String)

namespace adftool_r
{
  class term
  {
  private:
    adftool::term t;
  public:
    term (): t ()
    {
    }
    term (adftool::term &&v) noexcept: t (v)
    {
    }
    term (const term &v): t (v.t)
    {
    }
    term (term &&v) noexcept: t (std::move (v.t))
    {
    }
    term (SEXPREC *&): t ()
    {
    }
    term & operator= (const term & other) noexcept
    {
      this->copy (other);
      return *this;
    }
    term & operator= (term && other) noexcept
    {
      t = std::move (other.t);
      return *this;
    }
    void copy (const term & other) noexcept
    {
      t.copy (other.t);
    }
    void set_blank (const std::string & id) noexcept
    {
      t.set_blank (id);
    }
    void set_named (const std::string & id) noexcept
    {
      t.set_named (id);
    }
    void set_string (const std::string & value) noexcept
    {
      t.set_string (value);
    }
    void set_typed_literal (const std::string & value,
			    const std::string & type) noexcept
    {
      t.set_typed_literal (value, type);
    }
    void set_langstring (const std::string & value,
			 const std::string & langtag) noexcept
    {
      t.set_langstring (value, langtag);
    }
    void set_integer (long value) noexcept
    {
      t.set_integer (value);
    }
    void set_double (double value) noexcept
    {
      t.set_double (value);
    }
    void set_date (Rcpp::Datetime date) noexcept
    {
      using clock = std::chrono::high_resolution_clock;
      using seconds = std::chrono::seconds;
      using nanoseconds = std::chrono::nanoseconds;
      const double total_floor_seconds_double = floor (date.getFractionalTimestamp ());
      const seconds::duration::rep total_floor_seconds = total_floor_seconds_double;
      const Rcpp::Datetime floor = Rcpp::Datetime (total_floor_seconds);
      double ns_double = (date - floor) * 1e9;
      const nanoseconds::duration::rep ns = ns_double;
      const auto seconds_since_epoch = seconds (total_floor_seconds);
      const auto remaining = nanoseconds (ns);
      const clock::duration since_epoch =
	std::chrono::duration_cast<clock::duration> (seconds_since_epoch)
	+ std::chrono::duration_cast<clock::duration> (remaining);
      const std::chrono::time_point<clock> cxx_date =
	std::chrono::time_point<clock> (since_epoch);
      t.set_date (cxx_date);
    }
    bool is_blank (void) const noexcept
    {
      return t.is_blank ();
    }
    bool is_named (void) const noexcept
    {
      return t.is_named ();
    }
    bool is_literal (void) const noexcept
    {
      return t.is_literal ();
    }
    bool is_typed_literal (void) const noexcept
    {
      return t.is_typed_literal ();
    }
    bool is_langstring (void) const noexcept
    {
      return t.is_langstring ();
    }
    std::string value (void) const
    {
      return t.value_alloc ();
    }
    std::string meta (void) const
    {
      return t.meta_alloc ();
    }
    Rcpp::IntegerVector as_integer (void) const
    {
      const std::optional<long> value = t.as_integer ();
      if (value.has_value ())
	{
	  Rcpp::IntegerVector ret (1);
	  ret[0] = value.value ();
	  return ret;
	}
      Rcpp::IntegerVector ret (0);
      return ret;
    }
    Rcpp::NumericVector as_double (void) const
    {
      const std::optional<double> value = t.as_double ();
      if (value.has_value ())
	{
	  Rcpp::NumericVector ret (1);
	  ret[0] = value.value ();
	  return ret;
	}
      Rcpp::NumericVector ret (0);
      return ret;
    }
    Rcpp::DatetimeVector as_date (void) const
    {
      using clock = std::chrono::high_resolution_clock;
      using seconds = std::chrono::seconds;
      using nanoseconds = std::chrono::nanoseconds;
      const std::optional<std::chrono::time_point<clock>> date_value = t.as_date ();
      if (date_value.has_value ())
	{
	  const clock::duration since_epoch = date_value.value ().time_since_epoch ();
	  const seconds date_seconds =
	    std::chrono::duration_cast<seconds> (since_epoch);
	  const clock::duration since_epoch_floor =
	    std::chrono::duration_cast<clock::duration> (date_seconds);
	  const clock::duration remaining = since_epoch - since_epoch_floor;
	  const nanoseconds date_nanoseconds =
	    std::chrono::duration_cast<nanoseconds> (remaining);
	  const Rcpp::Datetime rough = Rcpp::Datetime (date_seconds.count ());
	  const Rcpp::Datetime fine =
	    rough + Rcpp::Datetime (date_nanoseconds.count () * 1e-9);
	  Rcpp::DatetimeVector ret(1);
	  ret[0] = fine;
	  return ret;
	}
      Rcpp::DatetimeVector ret (0);
      return ret;
    }
    int compare (const term & other) const
    {
      return t.compare (other.t);
    }
    Rcpp::List parse_n3 (const std::string & text)
    {
      Rcpp::List ret;
      static const std::string success_key = "success";
      static const std::string rest_key = "rest";
      const std::pair<bool, std::string::const_iterator> parse_result =
	t.parse_n3 (text.begin (), text.end ());
      ret[success_key] = parse_result.first;
      ret[rest_key] = std::string (parse_result.second, text.end ());
      return ret;
    }
    std::string to_n3 (void) const
    {
      return t.to_n3_alloc ();
    }
    adftool::term cxx_value (void) const
    {
      adftool::term ret;
      ret.copy (this->t);
      return ret;
    }
  };
}

namespace adftool_r
{
  class statement
  {
  private:
    adftool::statement t;
  public:
    statement (): t ()
    {
    }
    statement (adftool::statement &&v) noexcept: t (v)
    {
    }
    statement (const statement &v): t (v.t)
    {
    }
    statement (statement &&v) noexcept: t (std::move (v.t))
    {
    }
    statement (SEXPREC *&): t ()
    {
    }
    void copy (statement & other) noexcept
    {
      t.copy (other.t);
    }
    void set (const Rcpp::List &what)
    {
      std::array<std::optional<std::optional<adftool::term>>, 4> cxx_terms;
      const std::array<std::string, 4> keys = { "subject", "predicate", "object", "graph" };
#define set_ptr(i)							\
      if (what.containsElementNamed(keys[i].c_str ()))			\
	{								\
	  Rcpp::RObject value = what[keys[i]];				\
	  if (Rcpp::is<Rcpp::LogicalVector> (value))			\
	    {								\
	      /* Most likely FALSE */					\
	      std::optional<adftool::term> no_term;			\
	      std::get<i> (cxx_terms) = std::move (no_term);		\
	    }								\
	  else								\
	    {								\
	      term t = Rcpp::as<term> (what[keys[i]]);			\
	      std::optional<adftool::term> ot = t.cxx_value ();		\
	      std::get<i> (cxx_terms) = std::move (ot);			\
	    }								\
	}
      set_ptr (0);
      set_ptr (1);
      set_ptr (2);
      set_ptr (3);
#undef set_ptr
      const std::optional<uint64_t> no_date;
      const std::string date_key = "deletion_date";
      std::optional<std::optional<uint64_t>> date;
      if (what.containsElementNamed(date_key.c_str ()))
	{
	  Rcpp::RObject value = what[date_key];
	  if (Rcpp::is<Rcpp::LogicalVector> (value))
	    {
	      // Suppose FALSE
	      date = no_date;
	    }
	  else
	    {
	      Rcpp::NumericVector date_arg =
		Rcpp::as<Rcpp::NumericVector> (what[date_key]);
	      uint64_t actual_date = date_arg[0];
	      std::optional<uint64_t> actual_date_ptr = actual_date;
	      date = actual_date_ptr;
	    }
	}
      using optional_term_setter = std::optional<std::optional<adftool::term>>;
      using optional_date_setter = std::optional<std::optional<uint64_t>>;
      std::tuple<optional_term_setter,
		 optional_term_setter,
		 optional_term_setter,
		 optional_term_setter,
		 optional_date_setter> args;
      std::get<0> (args) = std::move (cxx_terms[0]);
      std::get<1> (args) = std::move (cxx_terms[1]);
      std::get<2> (args) = std::move (cxx_terms[2]);
      std::get<3> (args) = std::move (cxx_terms[3]);
      std::get<4> (args) = date;
      t.set (args);
    }
    std::map<std::string, term> get_terms (void) const
    {
      auto args = t.get ();
      std::array<adftool::term, 4> cxx_values;
      std::array<std::string, 4> names = { "subject", "predicate", "object", "graph" };
#define take(i) \
      cxx_values[i] = std::move (std::get<i> (args));
      take (0);
      take (1);
      take (2);
      take (3);
#undef take
      std::map<std::string, term> ret;
      for (size_t i = 0; i < 4; i++)
	{
	  term rcpp_value = std::move (cxx_values[i]);
	  ret[names[i]] = std::move (rcpp_value);
	}
      return ret;
    }
    Rcpp::NumericVector get_deletion_date (void) const
    {
      auto args = t.get ();
      if (std::get<4> (args).has_value ())
	{
	  Rcpp::NumericVector ret (1);
	  ret[0] = std::get<4> (args).value ();
	  return ret;
	}
      Rcpp::NumericVector ret (0);
      return ret;
    }
    int compare (const statement & other, const std::string order) const
    {
      return t.compare (other.t, order);
    }
    const adftool::statement &cxx_data (void) const
    {
      return t;
    }
  };
}

namespace adftool_r
{
  class fir
  {
  private:
    adftool::fir t;
  public:
    fir (double sfreq, double freq_low, double freq_high): t (sfreq, freq_low, freq_high)
    {
    }
    fir (double sfreq, double freq_low, double freq_high, double trans_low, double trans_high): t (sfreq, freq_low, freq_high, trans_low, trans_high)
    {
    }
    fir (fir && v) noexcept: t (std::move (v.t))
    {
    }
    fir & operator= (fir && v) noexcept
    {
      t = std::move (v.t);
      return *this;
    }
    std::vector<double> coefficients (void) const
    {
      return t.coefficients ();
    }
    std::vector<double> apply (const std::vector<double> &input) const
    {
      return t.apply (input);
    }
  };
}

namespace adftool_r
{
  class file
  {
  private:
    adftool::file t;
  public:
    file (std::string filename, bool writeable): t (filename, writeable)
    {
    }
    file (const std::vector<uint8_t> & data): t (data)
    {
    }
    file (file && v) noexcept: t (std::move (v.t))
    {
    }
    void close (void)
    {
      t.close ();
    }
    file & operator= (file && v) noexcept
    {
      t = std::move (v.t);
      return *this;
    }
    std::vector<uint8_t> get_data (void) const
    {
      return t.get_data ();
    }
    Rcpp::List lookup (const statement &pattern) const
    {
      std::optional<std::vector<adftool::statement>> cxx_opt_results = t.lookup (pattern.cxx_data ());
      if (cxx_opt_results.has_value ())
	{
	  std::vector<adftool::statement> cxx_results = cxx_opt_results.value ();
	  Rcpp::List ret = Rcpp::List (cxx_results.size ());
	  for (size_t i = 0; i < cxx_results.size (); i++)
	    {
	      adftool_r::statement rcpp_result = std::move (cxx_results[i]);
	      ret[i] = Rcpp::wrap (rcpp_result);
	    }
	  return ret;
	}
      else
	{
	  return Rcpp::List ();
	}
    }
    Rcpp::List lookup_objects (const term &subject, const std::string &predicate) const
    {
      std::vector<adftool::term> cxx_results = t.lookup_objects (subject.cxx_value (), predicate);
      Rcpp::List ret = Rcpp::List (cxx_results.size ());
      for (size_t i = 0; i < cxx_results.size (); i++)
	{
	  adftool_r::term rcpp_term = std::move (cxx_results[i]);
	  ret[i] = Rcpp::wrap (rcpp_term);
	}
      return ret;
    }
    Rcpp::List lookup_subjects (const term &object, const std::string &predicate) const
    {
      std::vector<adftool::term> cxx_results = t.lookup_subjects (object.cxx_value (), predicate);
      Rcpp::List ret = Rcpp::List (cxx_results.size ());
      for (size_t i = 0; i < cxx_results.size (); i++)
	{
	  adftool_r::term rcpp_term = std::move (cxx_results[i]);
	  ret[i] = Rcpp::wrap (rcpp_term);
	}
      return ret;
    }
    std::vector<long> lookup_integer (const term &subject, std::string predicate) const
    {
      return t.lookup_integer (subject.cxx_value (), predicate);
    }
    std::vector<double> lookup_double (const term &subject, std::string predicate) const
    {
      return t.lookup_double (subject.cxx_value (), predicate);
    }
    Rcpp::DatetimeVector lookup_date (const term &subject, std::string predicate) const
    {
      using clock = std::chrono::high_resolution_clock;
      using seconds = std::chrono::seconds;
      using nanoseconds = std::chrono::nanoseconds;
      const std::vector<std::chrono::time_point<clock>> date_values =
	t.lookup_date (subject.cxx_value (), predicate);
      Rcpp::DatetimeVector ret = Rcpp::DatetimeVector (date_values.size ());
      for (size_t i = 0; i < date_values.size (); i++)
	{
	  const std::chrono::time_point<clock> date_value = date_values[i];
	  const clock::duration since_epoch = date_value.time_since_epoch ();
	  const seconds date_seconds =
	    std::chrono::duration_cast<seconds> (since_epoch);
	  const clock::duration since_epoch_floor =
	    std::chrono::duration_cast<clock::duration> (date_seconds);
	  const clock::duration remaining = since_epoch - since_epoch_floor;
	  const nanoseconds date_nanoseconds =
	    std::chrono::duration_cast<nanoseconds> (remaining);
	  const Rcpp::Datetime rough = Rcpp::Datetime (date_seconds.count ());
	  const Rcpp::Datetime fine =
	    rough + Rcpp::Datetime (date_nanoseconds.count () * 1e-9);
	  ret[i] = fine;
	}
      return ret;
    }
    Rcpp::List lookup_string (const term &subject, std::string predicate) const
    {
      std::vector<std::pair<std::string, std::optional<std::string>>> records =
	t.lookup_string (subject.cxx_value (), predicate);
      Rcpp::List ret;
      for (auto i = records.begin (); i != records.end (); i++)
	{
	  std::string langtag = "";
	  if (i->second.has_value ())
	    {
	      langtag = i->second.value ();
	    }
	  Rcpp::StringVector others = Rcpp::StringVector ();
	  if (ret.containsElementNamed (langtag.c_str ()))
	    {
	      others = ret[langtag];
	    }
	  others.push_back (i->first);
	  ret[langtag] = others;
	}
      return ret;
    }
    Rcpp::LogicalVector delete_statement (const statement &pattern, uint64_t deletion_date)
    {
      Rcpp::LogicalVector ret = Rcpp::LogicalVector (1);
      ret[0] = t.delete_statement (pattern.cxx_data (), deletion_date);
      return ret;
    }
    Rcpp::LogicalVector insert_statement (const statement &statement)
    {
      Rcpp::LogicalVector ret = Rcpp::LogicalVector (1);
      ret[0] = t.insert_statement (statement.cxx_data ());
      return ret;
    }
    Rcpp::LogicalVector set_eeg_data (size_t n, size_t p, std::vector<double> data)
    {
      Rcpp::LogicalVector ret = Rcpp::LogicalVector (1);
      ret[0] = t.set_eeg_data (n, p, data);
      return ret;
    }
    Rcpp::List get_eeg_data (size_t start, size_t length, size_t channel) const
    {
      Rcpp::List ret = Rcpp::List ();
      const std::string n_key = "n";
      const std::string p_key = "p";
      const std::string data_key = "data";
      const auto data = t.get_eeg_data (start, length, channel);
      if (data.has_value ())
	{
	  ret[n_key] = std::get<0> (data.value ());
	  ret[p_key] = std::get<1> (data.value ());
	  std::vector<double> values = std::get<2> (data.value ());
	  Rcpp::NumericVector r_values = Rcpp::NumericVector (values.size ());
	  for (auto i = values.begin (); i != values.end (); i++)
	    {
	      r_values[i - values.begin ()] = *i;
	    }
	  ret[data_key] = r_values;
	}
      return ret;
    }
  };
}

RCPP_EXPOSED_CLASS_NODECL (adftool_r::term)
RCPP_EXPOSED_CLASS_NODECL (adftool_r::statement)
RCPP_EXPOSED_CLASS_NODECL (adftool_r::fir)
RCPP_EXPOSED_CLASS_NODECL (adftool_r::file)

extern "C" LIBADFTOOL_R_DLL_EXPORTED SEXP
_rcpp_module_boot_adftool ()
{
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  static Rcpp::Module adftool ("adftool");
  ::setCurrentScope (&adftool);

  /* Initialization code here */
  Rcpp::class_<adftool_r::term> ("term")
    .constructor(_("Construct a new, empty term."))
    .method ("copy", &adftool_r::term::copy, _("Copy the other term."))
    .method ("set_blank", &adftool_r::term::set_blank, _("Initialize the term as a blank term."))
    .method ("set_named", &adftool_r::term::set_named, _("Initialize the term as an IRIREF term."))
    .method ("set_string", &adftool_r::term::set_string, _("Initialize the term as a literal untagged string."))
    .method ("set_typed_literal", &adftool_r::term::set_typed_literal, _("Initialize the term as a typed literal string."))
    .method ("set_langstring", &adftool_r::term::set_langstring, _("Initialize the term as a lang-tagged string."))
    .method ("set_integer", &adftool_r::term::set_integer, _("Initialize the term as a literal integer."))
    .method ("set_double", &adftool_r::term::set_double, _("Initialize the term as a literal double."))
    .method ("set_date", &adftool_r::term::set_date, _("Initialize the term as a literal date with the number of seconds since the epoch, and the number of nanoseconds."))
    .method ("is_blank", &adftool_r::term::is_blank, _("Check whether the term is a blank node."))
    .method ("is_named", &adftool_r::term::is_blank, _("Check whether the term is a named node."))
    .method ("is_literal", &adftool_r::term::is_literal, _("Check whether the term is a literal node."))
    .method ("is_typed_literal", &adftool_r::term::is_typed_literal, _("Check whether the term is a literal node with a type (as opposed to a langtag)."))
    .method ("is_langstring", &adftool_r::term::is_langstring, _("Check whether the term is a literal node with a language tag."))
    .method ("value", &adftool_r::term::value, _("Return the value of the term."))
    .method ("meta", &adftool_r::term::meta, _("Return the type or langtag."))
    .method ("as_integer", &adftool_r::term::as_integer, _("Try and convert the term as an integer. Return a 0-length array if it cannot be converted, or a 1-integer-long array if it can."))
    .method ("as_double", &adftool_r::term::as_double, _("Try and convert the term as a double. Return a 0-length array if it cannot be converted, or a 1-double-long array if it can."))
    .method ("as_date", &adftool_r::term::as_date, _("Try and convert the term as a date. Return an empty list if the conversion failed, or a list with keys \"tv_sec\" and \"tv_nsec\" otherwise."))
    .method ("compare", &adftool_r::term::compare, _("Compare the term order with the other term. Return a negative number if the main term comes before the other term, a positive number if it comes after, and 0 if both terms are equal."))
    .method ("parse_n3", &adftool_r::term::parse_n3, _("Try and parse the string as a N3 term. If successful, return a list with \"success\" set to TRUE and \"rest\" set to the suffix that has not been used by the parser. Otherwise, return a list with \"success\" set to FALSE."))
    .method ("to_n3", &adftool_r::term::to_n3, _("Return as a string the N3 encoding of the term."));

  Rcpp::class_<adftool_r::statement> ("statement")
    .constructor(_("Construct a new, empty pattern."))
    .method ("copy", &adftool_r::statement::copy, _("Copy the other pattern."))
    .method ("set", &adftool_r::statement::set, _("Initialize parts of the pattern from the argument, a list. For each of the keys, subject, predicate, object and graph, if the list has a value, set the corresponding term. The value can be FALSE to unset the term, or a valid term object to replace it. If the list has a value for the deletion_date key, then also set the deletion date. If the deletion date is FALSE, then undelete the statement. Otherwise, set its deletion date."))
    .method ("get_terms", &adftool_r::statement::get_terms, _("Return all the terms as a named vector."))
    .method ("get_deletion_date", &adftool_r::statement::get_deletion_date, _("Return the deletion date as numeric vector. If there is no deletion date, return an empty numeric vector."))
    .method ("compare", &adftool_r::statement::compare, _("Compare the pattern order with the other pattern. Return a negative number if the main pattern comes before the other pattern, a positive number if it comes after, and 0 if both patterns overlap. The order is a lexicographic order, but the order of the terms to compare can be parametrized. For instance, \"GSPO\" compares the graphs, then subjects, then predicates, then objects."));

  Rcpp::class_<adftool_r::fir> ("fir")
    .constructor<double, double, double> (_("Construct a band-pass filter, acting on a signal of the given sampling frequency, and letting through frequencies from low to high."))
    .constructor<double, double, double, double, double> (_("Construct a band-pass filter, acting on a signal of the given sampling frequency, and letting through frequencies from low to high, by specifying the low and high transition bandwidths."))
    .method ("coefficients", &adftool_r::fir::coefficients, _("Return the coefficients of the filter."))
    .method ("apply", &adftool_r::fir::apply, _("Apply the filter on new data."));

  Rcpp::class_<adftool_r::file> ("file")
    .constructor<std::string, bool> (_("Open a file on disk."))
    .constructor<std::vector<uint8_t>> (_("Open an anonymous file on disk with initial data."))
    .method ("close", &adftool_r::file::close, _("Close the file early."))
    .method ("get_data", &adftool_r::file::get_data, _("Return the file contents as bytes."))
    .method ("lookup", &adftool_r::file::lookup, _("Search for a pattern in file, return all the matches."))
    .method ("lookup_objects", &adftool_r::file::lookup_objects, _("Search for all objects in the file that are related to the subject by predicate, return all the matches."))
    .method ("lookup_subjects", &adftool_r::file::lookup_subjects, _("Search for all subjects in the file that are related to the objects by predicate, return all the matches."))
    .method ("lookup_integer", &adftool_r::file::lookup_integer, _("Return all the integer properties of the subject in file for the predicate."))
    .method ("lookup_double", &adftool_r::file::lookup_double, _("Return all the numeric properties of the subject in file for the predicate."))
    .method ("lookup_date", &adftool_r::file::lookup_date, _("Return all the date properties of the subject in file for the predicate."))
    .method ("lookup_string", &adftool_r::file::lookup_string, _("Return all the possibly lang-tagged property values of the subject in file for the predicate. Return them as a list of string vectors, named by the langtags. The non-tagged strings have an empty name."))
    .method ("delete", &adftool_r::file::delete_statement, _("Try and delete all statements that match a pattern in file. Return wether it succeeded."))
    .method ("insert", &adftool_r::file::insert_statement, _("Try and insert a new statement in file. Return wether it succeeded."))
    .method ("set_eeg_data", &adftool_r::file::set_eeg_data, _("Try and set all the EEG data at once. Return wether it succeeded. The data must be row-oriented."))
    .method ("get_eeg_data", &adftool_r::file::get_eeg_data, _("Read a temporal slice of a channel. If the request is larger than what is available, fill the rest with NAN values. The return value is a list, with key n bound to the number of time points available, p bound to the total number of channels, and data bound to the requested data."));
  /* Rcpp stuff: */
  Rcpp::XPtr<Rcpp::Module> mod_xp (&adftool, false);
  ::setCurrentScope (0);
  return mod_xp;
}
