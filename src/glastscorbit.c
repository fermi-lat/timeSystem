#include "math.h"
#include "bary.h"

/* Tolerance of 1 millisecond in checking time boundaries */
static double time_tolerance = 1.e-3; /* in units of seconds */
/* Notes on the time tolerance
/*   Masaharu Hirayama, GSSC
/*   October 20th, 2009
/*
/* Rationale for 1-millisecond tolerance:
/* o The time duration of 1 millisecond is more than two orders of
/*   magnitude longer than the time resolution of Fermi time stamps (3-10
/*   microseconds). Therefore, if two time stamps are separate by more
/*   than 1 millisecond, one can safely assume that they represent two
/*   different points in time.
/* o The Fermi spacecraft travels approximately 25 nano-light-seconds in
/*   1 millisecond, and that can create a difference in a photon arrival
/*   time of 25 nanoseconds at most, which is more than 2 orders of
/*   magnitude shorter than the time resolution of Fermi time stamp (3-10
/*   microseconds). Therefore, it is highly unlikely that extrapolation
/*   of spacecraft positions at a boundary of FT2 coverage for 1
/*   millisecond will create a significant difference in geocentric or
/*   barycentric times.
/*
/* Related fact:
/* o The time duration of 25 nanoseconds is even shorter than (but of the
/*   same order of magnitude of) the computation precision guaranteed by
/*   the barycentering function ctatv.c (100 ns). This means that
/*   1-millisecond tolerance is as acceptable as use of (the current
/*   version of) ctatv.c.
 */

/* Compute vector inner product. */
static double inner_product(double vect_x[], double vect_y[])
{
  return vect_x[0]*vect_y[0] + vect_x[1]*vect_y[1] + vect_x[2]*vect_y[2];
}

/* Compute vector outer product. */
static void outer_product(double vect_x[], double vect_y[], double vect_z[])
{
  vect_z[0] = vect_x[1]*vect_y[2] - vect_x[2]*vect_y[1];
  vect_z[1] = vect_x[2]*vect_y[0] - vect_x[0]*vect_y[2];
  vect_z[2] = vect_x[0]*vect_y[1] - vect_x[1]*vect_y[0];
}



/* Read spacecraft positions from file and returns interpolated position. */
double *glastscorbit_getpos (char *filename, char *extname, double t, int *oerror)
{
  static double intposn[3];
  static fitsfile *OE = 0;
  static char savefile[256] = " ";
  static long num_rows = 0;
  static int colnum_start = 0;
  static int colnum_scposn = 0;
  static double *sctime_array = NULL;
  static long sctime_array_size = 0;
  static long index = 0;
  static double sctime1 = 0.;
  double sctime2 = 0.;
  double scposn1[3] = { 0., 0., 0. };
  double scposn2[3] = { 0., 0., 0. };
  double fract = 0.;
  int ii = 0;
/*
 *  Initialize ----------------------------------------------------------
 */
  *oerror = 0 ;
  for (ii = 0; ii < 3; ++ii)
    intposn[ii] = 0.0;

  /* Open file and prepare for reading the spacecraft position. */
  if ( strcmp (savefile, filename) ) {
    index = 0;
    num_rows = 0 ;
    sctime1 = 0.0;
    if ( *savefile != ' ' )
      fits_close_file (OE, oerror) ;
    strcpy (savefile, " ") ;
    if ( fits_open_file (&OE, filename, 0, oerror) )
      fprintf(stderr, "glastscorbit: Cannot open file %s\n", filename) ;
    else {
      fits_movnam_hdu(OE, ANY_HDU, extname, 0, oerror);

      /* Read table information. */
      fits_get_num_rows(OE, &num_rows, oerror);
      fits_get_colnum(OE, CASEINSEN, "START", &colnum_start, oerror);
      fits_get_colnum(OE, CASEINSEN, "SC_POSITION", &colnum_scposn, oerror);

      /* Allocate memory space to cache "START" column. */
      if (0 == *oerror && sctime_array_size < num_rows) {
        free(sctime_array);
        sctime_array = malloc(sizeof(double) * num_rows);
        if (NULL == sctime_array) {
          sctime_array_size = 0;
          *oerror = MEMORY_ALLOCATION;
        } else {
          sctime_array_size = num_rows;
        }
      }

      /* Read "START" column. */
      fits_read_col(OE, TDOUBLE, colnum_start, 1, 1, num_rows, 0, sctime_array, 0, oerror);

      /* Check for error(s) */
      if ( *oerror )
	fits_close_file (OE, oerror) ;
      else
	strcpy (savefile, filename) ;
    }
    if ( *oerror )
      return intposn ;
  }

  /* For times which are not monotonically increasing, start over at
     the beginning. */
  if (t < sctime1) index = 0;

  /* Find the first time which is > the time being corrected. */
  for (; index < num_rows; ++index) {
    sctime2 = sctime_array[index];
    if (t < sctime2) break;
  }

  /* Check bounds, with some level of tolerance. */
  if (index > 0 && index < num_rows) {
    /* In this case, the given time is between the first and the final rows,
       so get "START" column value in the previous row. */
    sctime1 = sctime_array[index-1];

  } else if (0 == index && t > sctime2 - time_tolerance) {
    /* In this case, the given time is earlier than, but close enough
       to, the time in the first row (which is currently stored in
       sctime2) within the given tolerance.  So, set index to 1 so
       that scposn1 will be the spacecraft position from the first
       row, copy sctime2 to sctime1 so that sctime1 will be the time
       from the first row, and set "START" column value of the second
       row sctime2. */
    index = 1;
    sctime1 = sctime2;
    sctime2 = sctime_array[index];

  } else if (num_rows == index && t < sctime2 + time_tolerance) {
    /* In this case, the given time is later than, but close enough
       to, the time in the final row (which is currently stored in
       sctime2) within the given tolerance.  So, set index back to
       num_rows - 1 so that scposn1 will be the spacecraft position
       from the penultimate row, and set "START" column value of the
       penultimate row to sctime1. */
    index = num_rows - 1;
    sctime1 = sctime_array[index-1];

  } else {
    /* In this case, the given time is out of bounds. */
    index = 0;
    *oerror = -2;
    return intposn;
  }

  /* Read "SC_POSITION" column in the previous and the current rows. */
  fits_read_col(OE, TDOUBLE, colnum_scposn, index,     1, 3, 0, scposn1, 0, oerror);
  fits_read_col(OE, TDOUBLE, colnum_scposn, index + 1, 1, 3, 0, scposn2, 0, oerror);

  /* Return if an error occurs while reading "SC_POSITION" columns. */
  if ( *oerror ) return intposn ;

  /* Interpolate. */
  fract = (t - sctime1) / (sctime2 - sctime1);
  {
    double length1, length2, length12, intlength;
    double vector12[3], vectprod_out[3];
    
    /* Linear interpolation for vector length. */
    length1 = sqrt(inner_product(scposn1, scposn1));
    length2 = sqrt(inner_product(scposn2, scposn2));
    intlength = length1 + fract*(length2 - length1);

    /* Compute a base vector on the orbital plane (vector12). */
    outer_product(scposn1, scposn2, vectprod_out);
    outer_product(vectprod_out, scposn1, vector12);
    length12 = sqrt(inner_product(vector12, vector12));

    /* Check vectors scposn1 and scposn2. */
    if ((length1 == 0.0) && (length2 == 0.0)) {
      /* both vectors are null */
      for (ii = 0; ii < 3; ++ii) intposn[ii] = 0.0;

    } else if (length1 == 0.0) {
      /* scposn1 is null, but scposn2 is not */
      for (ii = 0; ii < 3; ++ii) {
	intposn[ii] = scposn2[ii] / length2 * intlength;
      }

    } else if ((length2 == 0.0) || (length12 == 0.0)) {
      /* left:  scposn2 is null, but scposn1 is not */
      /* right: either vector is not null, but they are parallel */
      for (ii = 0; ii < 3; ++ii) {
	intposn[ii] = scposn1[ii] / length1 * intlength;
      }

    } else { /* Both has a non-zero length, and they are not parallel. */
      double inttheta, factor_cos, factor_sin;
      /* Linear interpolation for orbital phase. */
      inttheta = fract * acos(inner_product(scposn1, scposn2)
			      / length1 / length2);
      factor_cos = cos(inttheta);
      factor_sin = sin(inttheta);
      for (ii = 0; ii < 3; ++ii) {
	intposn[ii] = intlength * (scposn1[ii] / length1 * factor_cos 
				   + vector12[ii] / length12 * factor_sin);
      }
    }
  }

  /* Linear interpolation: obsolete
  fract = (t - sctime1) / (sctime2 - sctime1);
  for (ii = 0; ii < 3; ++ii) {
    intposn[ii] = scposn1[ii] + fract * (scposn2[ii] - scposn1[ii]);
  }
  */

/*
 *  Return position -----------------------------------------------------
 */
  return intposn ;
}

/* Wrapper function to read spacecraft positions from file and returns interpolated position,
/* for backward compatibility.
 */
double *glastscorbit (char *filename, double t, int *oerror)
{
  return glastscorbit_getpos(filename, "SC_DATA", t, oerror);
}
