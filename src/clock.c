/* $Id: clock.c,v 1.1.1.1 2006/04/21 20:56:15 peachey Exp $ */
/*------------------------------------------------------------------
 *
 *  File:        clock.c
 *  Programmer:  Arnold Rots, SAO/ASC
 *  Date:        2 September 1998
 *
 *  A set of functions that provides clock corrections for the
 *  the bary package.
 *
 *  baryinit will call clockinit.  After that, clock corrections
 *  (including TIMEZERO) will be returned by a call to clockCorr:
 *    double clockCorr (double time, double timezero, double *timeparms,
 *                      char *instrument)
 *  The first element of timeparms is TIERABSO; if the returned
 *  value is > 0, it is the correct value to use.
 *
 *  Note that instrumental delays are included in the correction!
 *
 *  To add support for another mission:
 *    Add the mission to the switch statement in clockCorr.
 *    Add a function to calculate its clock correction.
 *    Don't forget to provide support for it in bary.h and scorbit.c!
 *
 *-----------------------------------------------------------------*/

#include "bary.h"

void clockinit (enum Observatory obs)
/*------------------------------------------------------------------
 *
 *  A function that to initialize the clock correction package.
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

double clockCorr (double time, double timezero, double *timeparms,
		  char *instrument)
/*------------------------------------------------------------------
 *
 *  A function that determines the clock correction depending on mission.
 *  It returns the clock correction, including TIMEZERO.
 *
 *  Input:
 *    time         double   MET time
 *    timezero     double   Clock correction in header
 *    timeparms    double*  Clock parameters, the first of which is TIERABSO
 *    instrument   char*    INSTRUME (for instrument-dependent delays)
 *
 *  Output:
 *    timeparms[0] double*  Absolute clock error (<0 if unchanged)
 *
 *-----------------------------------------------------------------*/
{
  double xteClockCorr (double, double, double*, char*) ;
  double axafClockCorr (double, double, double*, char*) ;
  double swiftClockCorr (double, double, double*, char*) ;

/*
 *  Clock corrections ---------------------------------------------------
 */
  switch (mission) {
  case XTE:
    return xteClockCorr (time, timezero, timeparms, instrument) ;
  case AXAF:
    return axafClockCorr (time, timezero, timeparms, instrument) ;
  case SWIFT:
    return swiftClockCorr (time, timezero, timeparms, instrument) ;
  default:
    *timeparms = -1.0 ;
    return timezero ;
  }
/* Masaharu Hirayama 1 November 2008: Commented out the following line
   because 1) it does not return anything when this function should
   return a number of 'double' type, and 2) the flow of execution
   never reaches this statement.
  return ;
*/
}

double xteClockCorr (double time, double timezero, double *timeparms,
		     char *instrument)
/*------------------------------------------------------------------
 *
 *  A function that determines the clock correction for XTE.
 *  It returns the clock correction, including TIMEZERO.
 *
 *  Input:
 *    time         double   MET time
 *    timezero     double   Clock correction in header
 *    timeparms    double*  Clock parameters, the first of which is TIERABSO
 *    instrument   char*    INSTRUME (for instrument-dependent delays)
 *
 *  Output:
 *    timeparms[0] double*  Absolute clock error (<0 if unchanged)
 *
 *-----------------------------------------------------------------*/
{
#define PCACORR -0.000016
#define HEXTECORR -0.000001
  double t, tCorr ;

  int xCC (double, double *, double *) ;

  if ( xCC (time, &t, &tCorr) ) {
    /*  Can't find a clock correction  */
    t = timezero ;
    *timeparms = -1.0 ;
    fprintf (stderr, "===>  Please note that phaseHist could not locate an applicable\n") ;
    fprintf (stderr, "      entry in the XTE fine clock correction file.\n") ;
    fprintf (stderr, "      You may want to get a fresh copy of:\n") ;
    fprintf (stderr, "        ftp://legacy.gsfc.nasa.gov/xte/calib_data/clock/tdc.dat") ;
    fprintf (stderr, "      and deposit it in $TIMING_DIR/tdc.dat.\n") ;
  }
  else {
    /*  Truly raw SCC without TIMEZERO  */
    if ( *timeparms > 0.5 ) {
      t += 0.000001 * tCorr ;
      *timeparms = 0.000005 ;
    }
    /*  TIMEZERO supposedly is correct to 100 us  */
    else if ( *timeparms > 0.00005 ) {
      t = timezero + 0.000001 * tCorr ;
      *timeparms = 0.000005 ;
    }
    /*  This file is already fully corrected  */
    else {
      t = timezero ;
      *timeparms = -1.0 ;
      return t ;
    }
  }
  /*  Add the instrumental delays  */
  if ( !strncmp (instrument, "PCA", 3) )
    t += PCACORR ;
  else if ( !strncmp (instrument, "HEXTE", 5) )
    t += HEXTECORR ;
  /*  Return total correction  */
  return t ;
}

double axafClockCorr (double time, double timezero, double *timeparms,
		     char *instrument)
/*------------------------------------------------------------------
 *
 *  axafClockCorr is currently still stubbed ...
 *  A function that determines the clock correction for AXAF.
 *  It returns the clock correction, including TIMEZERO.
 *
 *  Input:
 *    time         double   MET time
 *    timezero     double   Clock correction in header
 *    timeparms    double*  Clock parameters, the first of which is TIERABSO
 *    instrument   char*    INSTRUME (for instrument-dependent delays)
 *
 *  Output:
 *    timeparms[0] double*  Absolute clock error (<0 if unchanged)
 *
 *-----------------------------------------------------------------*/
{
#define HRCCORR -0.0000015
#define ACISCORR -0.0

  double t ;

  t = timezero ;
  *timeparms = -1.0 ;
  /*  Add the instrumental delays  */
  if ( !strncmp (instrument, "HRC", 3) )
    t += HRCCORR ;
  else if ( !strncmp (instrument, "ACIS", 4) )
    t += ACISCORR ;
  /*  Return total correction  */
  return t ;
}

double swiftClockCorr (double time, double timezero, double *timeparms,
		     char *instrument)
/*------------------------------------------------------------------
 *
 *  swiftClockCorr is currently still stubbed ...
 *  A function that determines the clock correction for Swift.
 *  It returns the clock correction, including TIMEZERO.
 *
 *  Input:
 *    time         double   MET time
 *    timezero     double   Clock correction in header
 *    timeparms    double*  Clock parameters, the first of which is TIERABSO
 *    instrument   char*    INSTRUME (for instrument-dependent delays)
 *
 *  Output:
 *    timeparms[0] double*  Absolute clock error (<0 if unchanged)
 *
 *-----------------------------------------------------------------*/
{
#define XRTCORR -0.0
#define UVOTCORR -0.0
#define BATCORR -0.0

  double t ;

  t = timezero ;
  *timeparms = -1.0 ;

  /*  Add the instrumental delays  */
  if ( !strncmp (instrument, "BAT", 3) )
    t += BATCORR ;
  else if ( !strncmp (instrument, "UVOT", 4) )
    t += UVOTCORR ;
  else if ( !strncmp (instrument, "XRT", 3) )
    t += XRTCORR ;

  /*  Return total correction  */
  return t ;
}
