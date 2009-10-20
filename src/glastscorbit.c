#include "math.h"
#include "bary.h"

/* tolerance of 1 millisecond in checking time boundaries */
static double time_tolerance = 1.e-3; /* in units of seconds */

/* compute vector inner product */
static double inner_product(double vect_x[], double vect_y[])
{
  return vect_x[0]*vect_y[0] + vect_x[1]*vect_y[1] + vect_x[2]*vect_y[2];
}

/* compute vector outer product */
static void outer_product(double vect_x[], double vect_y[], double vect_z[])
{
  vect_z[0] = vect_x[1]*vect_y[2] - vect_x[2]*vect_y[1];
  vect_z[1] = vect_x[2]*vect_y[0] - vect_x[0]*vect_y[2];
  vect_z[2] = vect_x[0]*vect_y[1] - vect_x[1]*vect_y[0];
}

/* read spacecraft positions from file and returns interpolated position */
double *glastscorbit (char *filename, double t, int *oerror)
{
  static double intposn[3];
  static fitsfile *OE = 0;
  static char savefile[256] = " ";
  static long num_rows = 0;
  static int index = 0;
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
      fits_movabs_hdu (OE, 2, 0, oerror) ;
      fits_get_num_rows(OE, &num_rows, oerror);
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

  for (; index < num_rows; ++index) {
    /* Read "START" column, which is column # 1 */
    fits_read_col(OE, TDOUBLE, 1, index + 1, 1, 1, 0, &sctime2, 0, oerror);

    /* Return if an error occurs. */
    if ( *oerror ) return intposn ;

    /* Find the first time which is > the time being corrected. */
    if (t < sctime2) break;
  }

  /* Check bounds, with some level of tolerance. */
  if (index > 0 && index < num_rows) {
    /* In this case, the given time is between the first and the final rows,
       so read "START" column, which is column # 1, in the previous row. */
    fits_read_col(OE, TDOUBLE, 1, index, 1, 1, 0, &sctime1, 0, oerror);

  } else if (0 == index && t > sctime2 - time_tolerance) {
    /* In this case, the given time is earlier than, but close enough
       to, the time in the first row (which is currently stored in
       sctime2) within the given tolerance.  So, set index to 1 so
       that scposn1 will be the spacecraft position from the first
       row, copy sctime2 to sctime1 so that sctime1 will be the time
       from the first row, and read "START" column from the second row
       to set to sctime2. */
    index = 1;
    sctime1 = sctime2;
    fits_read_col(OE, TDOUBLE, 1, index + 1, 1, 1, 0, &sctime2, 0, oerror);

  } else if (num_rows == index && t < sctime2 + time_tolerance) {
    /* In this case, the given time is later than, but close enough
       to, the time in the final row (which is currently stored in
       sctime2) within the given tolerance.  So, set index back to
       num_rows - 1 so that scposn1 will be the spacecraft position
       from the penultimate row, and read "START" column from the
       penultimate row to set to sctime1. */
    index = num_rows - 1;
    fits_read_col(OE, TDOUBLE, 1, index, 1, 1, 0, &sctime1, 0, oerror);

  } else {
    /* In this case, the given time is out of bounds. */
    index = 0;
    *oerror = -2;
    return intposn;
  }

  /* Read "SC_POSITION" column, which is column # 3, in the previous and the current rows. */
  fits_read_col(OE, TDOUBLE, 3, index, 1, 3, 0, scposn1, 0, oerror);
  fits_read_col(OE, TDOUBLE, 3, index + 1, 1, 3, 0, scposn2, 0, oerror);

  /* Return if an error occurs while reading "START" and "SC_POSITION" columns. */
  if ( *oerror ) return intposn ;

  /* Interpolate. */
  fract = (t - sctime1) / (sctime2 - sctime1);
  {
    double length1, length2, length12, intlength;
    double vector12[3], vectprod_out[3];
    
    /* linear interpolation for vector length */
    length1 = sqrt(inner_product(scposn1, scposn1));
    length2 = sqrt(inner_product(scposn2, scposn2));
    intlength = length1 + fract*(length2 - length1);

    /* compute a base vector on the orbital plane (vector12) */
    outer_product(scposn1, scposn2, vectprod_out);
    outer_product(vectprod_out, scposn1, vector12);
    length12 = sqrt(inner_product(vector12, vector12));
		    
    /* check vectors scposn1 and scposn2 */
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

    } else { /* both has a non-zero length, and they are not parallel */
      double inttheta, factor_cos, factor_sin;
      /* linear interpolation for orbital phase */
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

  /* linear interpolation: obsolete
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
