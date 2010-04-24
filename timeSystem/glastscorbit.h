/** \file glastscorbit.h
    \brief Declarations for the C functions defined in glastscorbit.c.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_glastscorbit_h
#define timeSystem_glastscorbit_h

#include <fitsio.h>

/* Error code to return if a given time is not covered by this spacecraft file */
/* Note: The value must be different from any of the existing FITS error codes. */
#define TIME_OUT_BOUNDS -2

/* Structure to hold information of an opened spacecraft file */
typedef struct {
  fitsfile * fits_ptr;    /* Pointer to an opened spacecraft file */
  long num_rows;          /* The number of rows in the file */
  int colnum_scposn;      /* Column number of "SC_POSITION" column */
  double * sctime_array;  /* Copy of the "START" column contents */
  long sctime_array_size; /* Size of the above array */
  char * filename;        /* Name of the opened spacecraft file */
  char * extname;         /* Name of the spacecraft data extension */
  int open_count;         /* The number of requests to open this file */
} GlastScData;

/* Structure to be given to the requester to open a spacecraft file */
/* Note: A pointer to this structure is used as an identifier of an
   opened spacecraft file. */
typedef struct {
  GlastScData ** data; /* Pointer to an internal spacecraft data table */
  int status;          /* File I/O status (0 if normal) */
} GlastScFile;

/* Function prototypes for GLAST spacecraft file access */
GlastScFile * glastscorbit_open(char *, char *);
int glastscorbit_calcpos(GlastScFile *, double, double []);
int glastscorbit_close(GlastScFile *);
double * glastscorbit(char *, double, int *);
int glastscorbit_getstatus(GlastScFile *);
void glastscorbit_clearerr(GlastScFile *);

#endif
