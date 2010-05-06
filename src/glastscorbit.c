#include <longnam.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "timeSystem/glastscorbit.h"

/* Tolerance of 1 millisecond in checking time boundaries */
static const double time_tolerance = 1.e-3; /* in units of seconds */
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

/** \brief Helper expression for compare_interval to compare two time intervals.
           Note: This expression becomes true if interval "x" is earlier than interval "y",
           and false if otherwise.  Interval "x" is defined by a time range [x[0], x[1])
           for x[0] < x[1], or (x[1], x[0]] for other cases.
 */
#define is_less_than(x, y) (x[0] < y[0] && x[1] <= y[0] && x[0] <= y[1] && x[1] <= y[1])

/** \brief Comparison function to be passed to "bsearch" function.
    \param a Pointer that points to the first element of an interval to compare with the other.
    \param b Pointer that points to the first element of the other interval to compare.
 */
static int compare_interval(const void * a, const void * b)
{
  double *a_ptr = (double *)a;
  double *b_ptr = (double *)b;

  if (is_less_than(a_ptr, b_ptr)) return -1;
  else if (is_less_than(b_ptr, a_ptr)) return 1;
  else return 0;
}

/** \brief Compute vector inner product, and return it.
    \param vect_x Cartesian coordinates of the first vector of the inner product,
           where vect_x[0] is its x coordinate, vect_x[1] y, and vect_x[2] z.
           The size of the array must be at least 3.
    \param vect_y Cartesian coordinates of the second vector of the inner product,
           where vect_y[0] is its x coordinate, vect_y[1] y, and vect_y[2] z.
           The size of the array must be at least 3.
 */
static double inner_product(double vect_x[], double vect_y[])
{
  return vect_x[0]*vect_y[0] + vect_x[1]*vect_y[1] + vect_x[2]*vect_y[2];
}

/** \brief Compute vector outer product.
    \param vect_x Cartesian coordinates of the first vector of the outer product,
           where vect_x[0] is its x coordinate, vect_x[1] y, and vect_x[2] z.
           The size of the array must be at least 3.
    \param vect_y Cartesian coordinates of the second vector of the outer product,
           where vect_y[0] is its x coordinate, vect_y[1] y, and vect_y[2] z.
           The size of the array must be at least 3.
    \param vect_z Cartesian coordinates of the resultant outer product,
           where vect_z[0] is its x coordinate, vect_z[1] y, and vect_z[2] z.
           The size of the array must be at least 3.
 */
static void outer_product(double vect_x[], double vect_y[], double vect_z[])
{
  vect_z[0] = vect_x[1]*vect_y[2] - vect_x[2]*vect_y[1];
  vect_z[1] = vect_x[2]*vect_y[0] - vect_x[0]*vect_y[2];
  vect_z[2] = vect_x[0]*vect_y[1] - vect_x[1]*vect_y[0];
}

/* List of all opened spacecraft data files */
static GlastScData ** ScDataTable = NULL;    /* Array of pointers to open spacecraft files */
static GlastScData ** ScDataTableEnd = NULL; /* One past the last element of the above array */
static const int NMAXFILES = 300;            /* The maximum number of spacecraft files to open */
                                             /* Note: NMAXFILES is same as in fitio2.h */

/** \brief Return the I/O status code in a spacecraft file pointer.
    \param scfile Spacecraft file pointer whose I/O status code is to be returned.
 */
int glastscorbit_getstatus(GlastScFile * scfile) {
  if (scfile) return scfile->status;
  else return NULL_INPUT_PTR;
}

/** \brief Clear the I/O status code in a spacecraft file pointer.
    \param scfile Spacecraft file pointer whose I/O status code is to be cleared.
 */
void glastscorbit_clearerr(GlastScFile * scfile) {
  if (scfile) scfile->status = 0;
}

/** \brief Helper function to detach spacecraft data from a given spacecraft file pointer.
           If no other spacecraft file pointer needs the spacecraft data any longer,
           the function also cleans up the contents of the spacecraft data.
           The function returns 0 if successful, and a non-zero error code if otherwise.
           The returned error code can be decoded as a FITS error code.
    \param scfile Spacecraft file pointer whose contents is to be cleaned.
 */
static int detach_scdata(GlastScFile * scfile)
{
  GlastScData * scdata = NULL;
  int close_status = 0;

  /* Do nothing if no spacecraft file information is available. */
  /* Note: These cases are considered successful, in order to allow
     a caller to close a spacecraft file before opening it. */
  if (NULL == scfile || NULL == scfile->data) return 0;

  /* Copy the pointer to the spacecraft data for better readability. */
  scdata = *(scfile->data);

  if (scdata) {
    /* Decrement the file open counter. */
    scdata->open_count--;

    /* Close spacecraft file and free all the allocated memory spaces
       if this is the last closure of this file. */
    if (scdata->open_count <= 0) {
      /* Close the opened file. */
      if (NULL != scdata->fits_ptr) {
        fits_close_file(scdata->fits_ptr, &close_status);
        scdata->fits_ptr = NULL;
      }
      scdata->num_rows = 0;
      scdata->colnum_scposn = 0;

      /* Free the allocated memory space for "START" column. */
      free(scdata->sctime_array);
      scdata->sctime_array = NULL;
      scdata->sctime_array_size = 0;

      /* Free the allocated memory space for names. */
      free(scdata->filename);
      scdata->filename = NULL;
      free(scdata->extname);
      scdata->extname = NULL;

      /* Free memory space for this spacecraft data. */
      free(scdata);

      /* Clear the entry for this spacecraft data in the spacecraft data table. */
      *(scfile->data) = NULL;
    }
  }

  /* Strip access to the spacecraft data. */
  scfile->data = NULL;

  /* Return closing status. */
  return close_status;
}

/** \brief Close a spacecraft file and clean up initialized items.
           The function returns 0 if successful, and a non-zero error code if otherwise.
           The returned error code can be decoded as a FITS error code.
    \param scfile Spacecraft file pointer whose contents is to be cleaned.
 */
int glastscorbit_close(GlastScFile * scfile)
{
  int close_status = 0;

  /* Remove a connection to the spacecraft data. */
  close_status = detach_scdata(scfile);

  /* Destroy the spacecraft file pointer. */
  free(scfile);

  /* Return closing status. */
  return close_status;
}

/** \brief Open the given spacecraft file and initialize global variables.
           The function returns a pointer to a structure that acts as an
           identifier of the opened file. Error status is stored in a returned
           structure, and can be obtained by calling glastscorbit_getstatus
           with the returned pointer. The returned error code can be decoded
           as a FITS error code. The function returns a null pointer if
           and only if memory allocation for a structure to return fails.
           In all other cases, the function returns a valid pointer.
    \param filename Character string representing the name of a file to open.
    \param extname Character string representing the name of a FITS extension
           that contains spacecraft position information.
 */
GlastScFile * glastscorbit_open(char *filename, char *extname)
{
  GlastScFile * scfile = NULL;
  GlastScData * scdata = NULL;
  GlastScData ** scitor = NULL;
  int colnum_start = 0;

  /* Create an object to return, and initialize the contents. */
  scfile = malloc(sizeof(GlastScFile));
  if (NULL == scfile) return scfile;
  scfile->data = NULL;
  scfile->status = 0;

  /* Check the pointer arguments. */
  if (NULL == filename || NULL == extname) {
    scfile->status = FILE_NOT_OPENED;
    return scfile;
  }

  /* Check the spacecraft data table. */
  if (NULL == ScDataTable) {
    /* Initialize the pointer table for opened spacecraft files. */
    ScDataTable = malloc(sizeof(GlastScFile *) * NMAXFILES);
    if (NULL == ScDataTable) {
      scfile->status = MEMORY_ALLOCATION;
      return scfile;
    }
    ScDataTableEnd = ScDataTable + NMAXFILES;
    for (scitor = ScDataTable; scitor != ScDataTableEnd; ++scitor) *scitor = NULL;
    scfile->data = ScDataTable;

  } else {
    /* Find either an already opened file with the same name, or an open space in the pointer table. */
    for (scitor = ScDataTable; scitor != ScDataTableEnd; ++scitor) {
      if (*scitor) {
        /* Use this table entry if this opened file has the same filename and the same extension name. */
        if (!strcmp(filename, (*scitor)->filename) && !strcmp(extname, (*scitor)->extname)) {
          (*scitor)->open_count++;
          scfile->data = scitor;
          return scfile;
        }
      } else {
        /* Use this table entry in case the file is not already open. */
        if (NULL == scfile->data) scfile->data = scitor;
      }
    }

    /* Return an error if failed to find an entry in the spacecraft data table. */
    /* Note: This condition is met if the requested file is not already open, AND
       no open space is available in the pointer table, hence TOO_MANY_FILES. */
    if (NULL == scfile->data) {
      scfile->status = TOO_MANY_FILES;
      return scfile;
    }
  }

  /* Allocate memory space for spacecraft file information, and initialize the members. */
  scdata = malloc(sizeof(GlastScData));
  if (NULL == scdata) {
    scfile->status = MEMORY_ALLOCATION;
    return scfile;
  }
  *(scfile->data) = scdata;
  scdata->fits_ptr = NULL;
  scdata->num_rows = 0;
  scdata->colnum_scposn = 0;
  scdata->sctime_array = NULL;
  scdata->sctime_array_size = 0;
  scdata->filename = NULL;
  scdata->extname = NULL;
  scdata->open_count = 1;

  /* Allocate memory space for filename and extension name, and keep their copies. */
  scdata->filename = malloc(sizeof(char) * (strlen(filename) + 1));
  scdata->extname = malloc(sizeof(char) * (strlen(extname) + 1));
  if (scdata->filename && scdata->extname) {
    strcpy(scdata->filename, filename);
    strcpy(scdata->extname, extname);
  } else {
    scfile->status = MEMORY_ALLOCATION;
  }

  /* Open the given file, move to the spacecraft data, and read table information. */
  fits_open_file(&(scdata->fits_ptr), filename, 0, &(scfile->status));
  fits_movnam_hdu(scdata->fits_ptr, ANY_HDU, extname, 0, &(scfile->status));
  fits_get_num_rows(scdata->fits_ptr, &(scdata->num_rows), &(scfile->status));
  fits_get_colnum(scdata->fits_ptr, CASEINSEN, "START", &colnum_start, &(scfile->status));
  fits_get_colnum(scdata->fits_ptr, CASEINSEN, "SC_POSITION", &(scdata->colnum_scposn), &(scfile->status));

  /* Require two rows at the minimum, for interpolation to work. */
  if (0 == scfile->status && scdata->num_rows < 2) {
    /* Return BAD_ROW_NUM because it would read the second row
       which does not exist in this case. */
    scfile->status = BAD_ROW_NUM;
  }

  /* Allocate memory space to cache "START" column. */
  if (0 == scfile->status) {
    scdata->sctime_array = malloc(sizeof(double) * scdata->num_rows);
    if (NULL == scdata->sctime_array) {
      scdata->sctime_array_size = 0;
      scfile->status = MEMORY_ALLOCATION;
    } else {
      scdata->sctime_array_size = scdata->num_rows;
    }
  }

  /* Read "START" column. */
  fits_read_col(scdata->fits_ptr, TDOUBLE, colnum_start, 1, 1, scdata->num_rows, 0, scdata->sctime_array, 0, &(scfile->status));

  /* Finally check errors in opening file. If an error occurred, close spacecraft file
     and free all the allocated memory spaces. Ignore an error in closing file, and
     preserve the error in opening it. */
  if (scfile->status) detach_scdata(scfile);

  /* Return the spacecraft file information. */
  return scfile;
}

/** \brief Read spacecraft positions from file and computes interpolated position.
           The resultant spacecraft position is set to the argument of the function.
           The function returns 0 if successful, and a non-zero error code if otherwise.
           The returned error code can be decoded as a FITS error code, except a given
           time is not covered by the given spacecraft file, in which case the function
           returns TIME_OUT_BOUNDS defined in glastscorbit.h. If an error occurs in file
           I/O operation(s), then the function also sets the error code from the file I/O
           to the given spacecraft file structure. The function does not perform any
           computation when the given spacecraft file already has an I/O error. To clear
           the error code, call glastscorbit_clearerr with the structure.
    \param scfile Spacecraft file pointer whose contents is to be cleaned.
    \param t Time in Mission Elapsed Time (MET) at which the spacecraft position is
           to be computed.
    \param intposn Array to which interpolated spacecraft position at the given time
           is to be set, where intposn[0] is the x coordinate of the equatorial
           coordinate system with the origin at the center of the Earth, intposn[1] y,
           and intposn[2] z. The size of the array must be at least 3.
 */
int glastscorbit_calcpos(GlastScFile * scfile, double t, double intposn[3])
{
  GlastScData * scdata = NULL;
  double evtime_array[2];
  double *sctime_ptr = NULL;
  long scrow1 = 0;
  long scrow2 = 0;
  double sctime1 = 0.;
  double sctime2 = 0.;
  double scposn1[3] = { 0., 0., 0. };
  double scposn2[3] = { 0., 0., 0. };
  double fract = 0.;
  int ii = 0;

  /* Clear the previously stored values. */
  for (ii = 0; ii < 3; ++ii) intposn[ii] = 0.0;

  /* Do nothing if no spacecraft file information is available. */
  /* Note: Do NOT override scfile->status with this status, because
     this error is not from an I/O operation by this function. */
  if (NULL == scfile) return NULL_INPUT_PTR;
  if (NULL == scfile->data || NULL == *(scfile->data)) return BAD_FILEPTR;

  /* Return an error status if an error has occurred on this file. */
  /* Note: Do NOT override scfile->status with this status, because
     this error is not from an I/O operation by this function. */
  if (scfile->status) return BAD_FILEPTR;

  /* Copy the pointer to the spacecraft data for better readability. */
  scdata = *(scfile->data);

  /* Find two neighboring rows from which the spacecraft position at
     the given time will be computed. */
  if (fabs(t - scdata->sctime_array[0]) <= time_tolerance) {
    /* In this case, the given time is close enough to the time in the
       first row within the given tolerance.  So, use the first two
       rows for the computation. */
    scrow1 = 1;
    scrow2 = 2;
    sctime1 = scdata->sctime_array[0];
    sctime2 = scdata->sctime_array[1];

  } else if (fabs(t - scdata->sctime_array[scdata->num_rows-1]) <= time_tolerance) {
    /* In this case, the given time is close enough to the time in the
       final row within the given tolerance.  So, use the penultimate
       row and the final row for the computation. */
    scrow1 = scdata->num_rows - 1;
    scrow2 = scdata->num_rows;
    sctime1 = scdata->sctime_array[scdata->num_rows - 2];
    sctime2 = scdata->sctime_array[scdata->num_rows - 1];

  } else {
    evtime_array[0] = evtime_array[1] = t;
    sctime_ptr = (double *)bsearch(evtime_array, scdata->sctime_array, scdata->num_rows - 1, sizeof(double), compare_interval);
    if (NULL == sctime_ptr) {
      /* In this case, the given time is out of bounds. */
      /* Note: Do NOT override scfile->status with this status, because
         this error is not from an I/O operation by this function. */
      return TIME_OUT_BOUNDS; /* Defined in glastscorbit.h. */
    }

    /* In this case, the given time is between the first and the
       final rows, so use the row returned by bsearch and the next
       row for the computation. */
    scrow1 = sctime_ptr - scdata->sctime_array + 1;
    scrow2 = scrow1 + 1;
    sctime1 = sctime_ptr[0];
    sctime2 = sctime_ptr[1];
  }

  /* Read "SC_POSITION" column in the two rows found above. */
  fits_read_col(scdata->fits_ptr, TDOUBLE, scdata->colnum_scposn, scrow1, 1, 3, 0, scposn1, 0, &(scfile->status));
  fits_read_col(scdata->fits_ptr, TDOUBLE, scdata->colnum_scposn, scrow2, 1, 3, 0, scposn2, 0, &(scfile->status));

  /* Return if an error occurs while reading "SC_POSITION" columns. */
  if (scfile->status) return scfile->status;

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

  /* Return the computed spacecraft position. */
  return scfile->status;
}

/** \brief Read spacecraft positions from file and return interpolated position.
           Note: This function is obsolete and unnecessary, but kept as is only
           for backward compatibility. Use glastscorbit_open/calcpos/close instead.
    \param filename Character string representing the name of a spacecraft file.
           Note: The name of extension is purposely hard-coded as "SC_DATA" for
           complete backward compatibility.
    \param t Time in Mission Elapsed Time (MET) at which the spacecraft position is
           to be computed.
    \param oerror Pointer of an integer to which an error code is set by the function.
           The returned error code can be decoded as a FITS error code, except a given
           time is not covered by the given spacecraft file, in which case the function
           returns TIME_OUT_BOUNDS defined in glastscorbit.h. The function ignores
           an integer value pointed by this pointer at the time of a call to this function,
           and resets it to zero (0) at the beginning of the function.
 */
double * glastscorbit(char *filename, double t, int *oerror)
{
  static double intposn[3] = {0., 0., 0.};
  static char savefile[256] = " ";
  static GlastScFile * scfile = NULL;
  int ii = 0;

  /* Override error status to preserve the original behavior. */
  *oerror = 0;
  glastscorbit_clearerr(scfile);

  /* Clear the previously stored values. */
  for (ii = 0; ii < 3; ++ii) intposn[ii] = 0.0;

  /* Check whether a new file is given. */
  if (strcmp(savefile, filename)) {
    /* Close the previously opened spacecraft file, if any. */
    glastscorbit_close(scfile);

    /* Open file and prepare for reading the spacecraft position. */
    scfile = glastscorbit_open(filename, "SC_DATA");

    /* Copy file opening status. */
    *oerror = scfile->status;

    /* Refresh the saved file name. */
    if (0 == *oerror) {
      strcpy(savefile, filename);
    } else {
      strcpy(savefile, " ");
      return intposn;
    }
  }

  /* Compute and return the spacecraft position at the given time. */
  *oerror = glastscorbit_calcpos(scfile, t, intposn);
  return intposn;
}
