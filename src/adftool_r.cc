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

#include <adftool.h>
#include <Rcpp.h>
#include <cmath>
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

RCPP_EXPOSED_CLASS (adftool_r::term)
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
  /* Rcpp stuff: */
  Rcpp::XPtr<Rcpp::Module> mod_xp (&adftool, false);
  ::setCurrentScope (0);
  return mod_xp;
}
