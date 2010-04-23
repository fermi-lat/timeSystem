/** \file glastscorbit.h
    \brief Declarations for the C functions defined in glastscorbit.c.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_glastscorbit_h
#define timeSystem_glastscorbit_h

#include <fitsio.h>

#define TIME_OUT_BOUNDS -2

/* Structure to hold the spacecraft file information. */
typedef struct {
  fitsfile * fits_ptr;
  long num_rows;
  int colnum_scposn;
  double * sctime_array;
  long sctime_array_size;
  char * filename;
  char * extname;
  int open_count;
} GlastScData;

typedef struct {
  GlastScData ** data;
  int status;
} GlastScFile;

// Declare function prototypes for GLAST spacecraft file access.
GlastScFile * glastscorbit_open(char *, char *);
int glastscorbit_calcpos(GlastScFile *, double, double []);
int glastscorbit_close(GlastScFile *);
double * glastscorbit(char *, double, int *);
int glastscorbit_getstatus(GlastScFile *);
void glastscorbit_clearerr(GlastScFile *);

#endif
