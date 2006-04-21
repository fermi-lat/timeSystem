/*------------------------------------------------------------------
 *
 *  File:        scorbit.c
 *  Programmer:  Arnold Rots, SAO/ASC
 *  Date:        2 September 1998
 *
 *  A set of functions that provides orbit positions for the
 *  the bary package.
 *
 *  baryinit will call scorbitinit.  After that, spacecraft positions
 *  will be returned by a call to scorbit or xscorbit:
 *    double *scorbit (char *filename, MJDTime *time, int *oerror)
 *    double *xscorbit (char *filename, double time, int *oerror)
 *
 *  To add support for another mission:
 *    Add the mission to the switch statement in clockCorr.
 *    Add a function to calculate its clock correction.
 *    Don't forget to provide support for it in bary.h and clock.c!
 *
 *-----------------------------------------------------------------*/

#include "bary.h"

void scorbitinit (enum Observatory obs)
/*------------------------------------------------------------------
 *
 *  A function that to initialize the spacecraft orbit package.
 *  It sets the mission parameter.
 *
 *  Input:
 *    mission      Observatory Mission, observatory, telescope
 *
 *-----------------------------------------------------------------*/
{
  mission = obs ;
  return ;
}

/*-----------------------------------------------------------------------
 *
 *  scorbit reads the orbit ephemeris from file OE and interpolates the
 *  spacecraft position at time t; the pointer to an array with these
 *  coordinates is returned.
 *  If t falls outside the range covered by the file, a non-zero value
 *  is returned in oerror.
 *
 *  This is the generic MJDTime version that passes control to the version
 *  appropriate for the value of mission.
 *
 *----------------------------------------------------------------------*/

double *scorbit (char *filename, MJDTime *time, int *oerror)
{
  static double scposn0[3] = {0.0, 0.0, 0.0} ;

  switch ( mission ) {
  case Geocenter:
    return scposn0 ;
  case XTE:
  case AXAF:   /* Same format as XTE */
    return xtescorbit (filename, mjd2met(time), oerror) ;
  case SWIFT:
    return swiftscorbit (filename, mjd2met(time), oerror) ;
  case GLAST:
    return glastscorbit (filename, mjd2met(time), oerror) ;
  case Unknown:
    *oerror = -11 ;
    return NULL ;
  default:
    *oerror = -12 ;
    return NULL ;
  }
}

/*-----------------------------------------------------------------------
 *
 *  xscorbit reads the orbit ephemeris from file OE and interpolates the
 *  spacecraft position at time t; the pointer to an array with these
 *  coordinates is returned.
 *  If t falls outside the range covered by the file, a non-zero value
 *  is returned in oerror.
 *
 *  This is the generic MET version that passes control to the version
 *  appropriate for the value of mission.
 *
 *----------------------------------------------------------------------*/

double *xscorbit (char *filename, double time, int *oerror)
{
  static double scposn0[3] = {0.0, 0.0, 0.0} ;

  switch ( mission ) {
  case Geocenter:
    return scposn0 ;
  case XTE:
  case AXAF:   /* Same format as XTE */
    return xtescorbit (filename, time, oerror) ;
  case SWIFT:
    return swiftscorbit (filename, time, oerror) ;
  case GLAST:
    return glastscorbit (filename, time, oerror) ;
  case Unknown:
    *oerror = -11 ;
    return NULL ;
  default:
    *oerror = -12 ;
    return NULL ;
  }
}

/*-----------------------------------------------------------------------
 *
 *  xtescorbit reads the orbit ephemeris from file OE and interpolates the
 *  spacecraft position at time t; the pointer to an array with these
 *  coordinates is returned.
 *  If t falls outside the range covered by the file, a non-zero value
 *  is returned in oerror.
 *
 *  XTE and AXAF version
 *
 *----------------------------------------------------------------------*/

double *xtescorbit (char *filename, double t, int *oerror)
{
  static double sctime[NOEROWS] ;
  static double scposn[3][NOEROWS] ;
  static double scvel[3][NOEROWS] ;
  int htype, any ;
  static double intposn[3] ;
  static fitsfile *OE ;
  static char savefile[256]= " " ;
  int i, j ;
  double scacc, dt, dt2 ;
  static int k ;
  char line[256] ;
  static int startrow ;
  static int numrows ;
  static int nbrows ;
  int startinx, relinx ;
  static double deltat, tstart, tstop ;
/*
 *  Initialize ----------------------------------------------------------
 */
  *oerror = 0 ;
  for (j=0; j<3; j++)
    intposn[j] = 0.0 ;

  if ( strcmp (savefile, filename) ) {
    startrow = 0 ;
    nbrows = 0 ;
    if ( *savefile != ' ' )
      fits_close_file (OE, oerror) ;
    strcpy (savefile, " ") ;
    if ( fits_open_file (&OE, filename, 0, oerror) )
      fprintf(stderr, "xscorbit: Cannot open file %s\n", filename) ;
    else {
      fits_movabs_hdu (OE, 2, &htype, oerror) ;
      if ( !(*oerror) ) {
	fits_read_key (OE, TDOUBLE, "DELTAT", &deltat, line, oerror) ;
	if ( *oerror ) {
	  *oerror = 0 ;
	  fits_read_key (OE, TDOUBLE, "TIMEDEL", &deltat, line, oerror) ;
	}
      }
      fits_read_key (OE, TDOUBLE, "TSTART", &tstart, line, oerror) ;
      fits_read_key (OE, TDOUBLE, "TSTOP", &tstop, line, oerror) ;
      fits_read_key (OE, TLONG, "NAXIS2", &numrows, line, oerror) ;
      if ( *oerror )
	fits_close_file (OE, oerror) ;
      else
	strcpy (savefile, filename) ;
    }
    if ( *oerror )
      return intposn ;
  }

/*
 *  See whether orbit file is appropriate --------------------------------
 */
  startinx = (t - tstart) / deltat + 0.5 ;
  if ( ( startinx <= 0 ) || ( startinx > numrows-2 ) ) {
    fprintf(stderr, "xscorbit: Time %g not covered by file %s\n", t, filename) ;
    *oerror = -2 ;
    return intposn ;
  }

/*
 *  Read records from orbit file -----------------------------------------
 */
  relinx = startinx - startrow ;
  if ( !nbrows || !startrow || ( relinx > nbrows-3 ) || (relinx < 0 ) ) {
    startrow = startinx ;
    nbrows = ((startrow + NOEROWS) <= numrows) ? NOEROWS : (numrows - startrow) ;
    fits_read_col (OE, TDOUBLE, 1, startrow, 1, nbrows, 0,
		   sctime, &any, oerror) ;
    for (i=0; i<3; i++) {
      fits_read_col (OE, TDOUBLE, i+2, startrow, 1, nbrows, 0,
		     &scposn[i][0], &any, oerror) ;
      fits_read_col (OE, TDOUBLE, i+5, startrow, 1, nbrows, 0,
		     &scvel[i][0], &any, oerror) ;
    }
    if ( *oerror ) {
      fprintf (stderr, "xscorbit: error while reading rows starting at %d\n",
	       startrow) ;
      return intposn ;
    }
    relinx = startinx - startrow ;
  }

/*
 *  Interpolate position ------------------------------------------------
 */
    dt = t - sctime[relinx+1] ;
    dt2 = 0.5 * dt * dt ;
    for (j=0; j<3; j++) {
      scacc = (scvel[j][relinx+2] - scvel[j][relinx]) /
	(sctime[relinx+2] - sctime[relinx]) ;
      intposn[j] = scposn[j][relinx+1] + dt * scvel[j][relinx+1] + dt2 * scacc ;
    }

/*
 *  Return position -----------------------------------------------------
 */
  return intposn ;
}

/*-----------------------------------------------------------------------
 *
 *  swiftscorbit reads the orbit ephemeris from file OE and interpolates the
 *  spacecraft position at time t; the pointer to an array with these
 *  coordinates is returned.
 *  If t falls outside the range covered by the file, a non-zero value
 *  is returned in oerror.
 *
 *  XTE orbit files always have TIMEZERO = 0.0 and so timezero is ignored
 *
 *  Swift orbit files (prefilter output) have position and velocity written
 *  as vector columns with units of km and km/s, respectively. We convert the
 *  position vector to meters just before returning it (after interpolation).
 *
 *  Swift version by M.Tripicco 13May2004
 *
 *----------------------------------------------------------------------*/

double *swiftscorbit (char *filename, double t, int *oerror)
{
  static double sctime[NOEROWS] ;
  static double scposn[3][NOEROWS] ;
  static double scvel[3][NOEROWS] ;
  static double vectmp[3*NOEROWS]; /* used to deal with vector cols */
  int htype, any ;
  static double intposn[3] ;
  static fitsfile *OE ;
  static char savefile[256]= " " ;
  int i, j ;
  double scacc, dt, dt2 ;
  static int k ;
  char line[256] ;
  static int startrow ;
  static int numrows ;
  static int nbrows ;
  int startinx, relinx ;
  static double deltat, tstart, tstop ;
/*
 *  Initialize ----------------------------------------------------------
 */
  *oerror = 0 ;
  for (j=0; j<3; j++)
    intposn[j] = 0.0 ;
  
  if ( strcmp (savefile, filename) ) {
    startrow = 0 ;
    nbrows = 0 ;
    if ( *savefile != ' ' )
      fits_close_file (OE, oerror) ;
    strcpy (savefile, " ") ;
    if ( fits_open_file (&OE, filename, 0, oerror) )
      fprintf(stderr, "swiftscorbit: Cannot open file %s\n", filename) ;
    else {
      fits_movabs_hdu (OE, 2, &htype, oerror) ;
      if ( !(*oerror) ) {
	fits_read_key (OE, TDOUBLE, "DELTAT", &deltat, line, oerror) ;
	if ( *oerror ) {
	  *oerror = 0 ;
	  fits_read_key (OE, TDOUBLE, "TIMEDEL", &deltat, line, oerror) ;
	}
      }
      fits_read_key (OE, TDOUBLE, "TSTART", &tstart, line, oerror) ;
      fits_read_key (OE, TDOUBLE, "TSTOP", &tstop, line, oerror) ;
      fits_read_key (OE, TLONG, "NAXIS2", &numrows, line, oerror) ;
      if ( *oerror )
	fits_close_file (OE, oerror) ;
      else
	strcpy (savefile, filename) ;
    }
    if ( *oerror ){
      fprintf(stderr,"swiftscorbit: keyword missing..returning intposn\n");
      return intposn ;
    }
  }

/*
 *  See whether orbit file is appropriate --------------------------------
 */
  startinx = (t - tstart) / deltat + 0.5 ;
  if ( ( startinx <= 0 ) || ( startinx > numrows-2 ) ) {
    fprintf(stderr, "swiftscorbit: Time %g not covered by file %s\n", t, filename) ;
    *oerror = -2 ;
    return intposn ;
  }
  
/*
 *  Read records from orbit file -----------------------------------------
 */
  relinx = startinx - startrow ;
  if ( !nbrows || !startrow || ( relinx > nbrows-3 ) || (relinx < 0 ) ) {
    startrow = startinx ;
    nbrows = ((startrow + NOEROWS) <= numrows) ? NOEROWS : (numrows - startrow) ;
    fits_read_col (OE, TDOUBLE, 1, startrow, 1, nbrows, 0,
		   sctime, &any, oerror) ;
    for (i=0; i<3; i++) {
      fits_read_col (OE, TDOUBLE, 2, startrow, i+1, 3*nbrows, 0,
		     &vectmp[0], &any, oerror) ; /* stored as vector columns */
      for (j=0; j<nbrows; j++) scposn[i][j] = vectmp[j*3];
      fits_read_col (OE, TDOUBLE, 3, startrow, i+1, 3*nbrows, 0,
		     &vectmp[0], &any, oerror) ; /* stored as vector columns */
      for (j=0; j<nbrows; j++) scvel[i][j] = vectmp[j*3];
    }
    if ( *oerror ) {
      fprintf (stderr, "swiftscorbit: error while reading rows starting at %d\n",startrow) ;
      return intposn ;
    }
    relinx = startinx - startrow ;
  }

/*
 *  Interpolate position ------------------------------------------------
 */
  dt = t - sctime[relinx+1] ;
  dt2 = 0.5 * dt * dt ;
  for (j=0; j<3; j++) {
    scacc = (scvel[j][relinx+2] - scvel[j][relinx]) /
      (sctime[relinx+2] - sctime[relinx]) ;
    intposn[j] = scposn[j][relinx+1] + dt * scvel[j][relinx+1] + dt2 * scacc ;
    intposn[j] *= 1000.0; /* convert position vector to meters from km */
  }
  
/*
 *  Return position -----------------------------------------------------
 */
  return intposn ;
}
