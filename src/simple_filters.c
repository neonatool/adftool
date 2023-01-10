#include <config.h>

#include <adftool.h>
#include <hdf5.h>

#include <stdio.h>
#include <stdlib.h>
#include "gettext.h"
#include "relocatable.h"
#include "progname.h"
#include <locale.h>
#include <assert.h>

#define _(String) gettext(String)
#define N_(String) (String)

static double absdiff (double f1, double f2);
static int appeq (double f1, double f2);

#define APPEQ(f1, f2) appeq (f1, f2)

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  const size_t filter_order = 113;
  const double sfreq = 256;
  struct adftool_fir *filter = adftool_fir_alloc (filter_order);
  if (filter == NULL)
    {
      abort ();
    }
  adftool_fir_design_bandpass (filter, sfreq, 0, 30, 0, 7.5);
  assert (adftool_fir_order (filter) == filter_order);
  double coef[113];
  assert (sizeof (coef) == filter_order * sizeof (double));
  adftool_fir_coefficients (filter, coef);
  assert (APPEQ (coef[0], 3.05061457e-04));
  assert (APPEQ (coef[1], 4.66692952e-04));
  assert (APPEQ (coef[2], 3.32211517e-04));
  assert (APPEQ (coef[55], 0.23412415420338445));
  assert (APPEQ (coef[56], 0.2633994815898794));
  assert (APPEQ (coef[57], 0.23412415420338445));
  assert (APPEQ (coef[112], coef[0]));
  adftool_fir_free (filter);
  /* Second example: high pass */
  filter = adftool_fir_alloc (filter_order);
  if (filter == NULL)
    {
      abort ();
    }
  /* Cut-off, 30, minus half the bandwidth, 7.5 */
  adftool_fir_design_bandpass (filter, sfreq, 30, 128, 7.5, 0);
  assert (adftool_fir_order (filter) == filter_order);
  assert (sizeof (coef) == filter_order * sizeof (double));
  adftool_fir_coefficients (filter, coef);
  assert (APPEQ (coef[0], 0.0004543420808785086));
  assert (APPEQ (coef[1], 0.0003594407595337251));
  assert (APPEQ (coef[2], 0.00011293967292371414));
  assert (APPEQ (coef[55], -0.19111170684105802));
  assert (APPEQ (coef[56], 0.794848991806567));
  assert (APPEQ (coef[57], -0.19111170684105802));
  assert (APPEQ (coef[112], coef[0]));
  adftool_fir_free (filter);
  return 0;
}

static double
absdiff (double f1, double f2)
{
  double ret = f1 - f2;
  if (ret < 0)
    {
      ret = -ret;
    }
  return ret;
}

static int
appeq (double f1, double f2)
{
  const double d = absdiff (f1, f2);
  const double r1 = d / absdiff (f1, 0);
  const double r2 = d / absdiff (f2, 0);
  if (d >= 1e-3 || r1 >= 1e-2 || r2 >= 1e-2)
    {
      return 0;
    }
  return 1;
}
