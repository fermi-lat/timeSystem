char *xccrcsid = "RCS: $Id: xCC.c,v 1.1.1.1 2005/08/03 20:42:28 peachey Exp $" ;
#include <stdio.h>
#include <stdlib.h>
/* JP 8 march 2005 commented out the following line because it is not
   strictly necessary on Unix, and this header is not present on Windows.
#include <unistd.h>
*/
#include <string.h>
#include "bary.h"

int xCC (double t, double *tZero, double *tcorr)
{

  FILE *tdc ;

  double subday, m0, m1, m2, end, t0=0.0 ;
  double modt=0.0, corr = -999999.0 ;
  int error = 0 ;

  if ( !( tdc = openAFile (XTECC) ) ) {
    error = -1 ;
    corr = 0 ;
  }

  fprintf(stderr,"correcting time = %lg\n",t);
  while ( !error && ( fscanf (tdc, "%lg %lg %lg %lg", &m0, &m1, &m2, &end) ) ) {
    fprintf(stderr,"modt is %lg, read m0=%lg m1=%lg m2=%lg end=%lg\n",modt,m0,m1,m2,end);
    if ( end < 0.0 ) {
      if ( m0 < 0.0 ) {
	error = 2 ;
	break ;
      }
      else {
	subday = m0 ;
	modt = t / 86400.0 - subday ;
	t0 = m1 ;
	fprintf(stderr,"computed subday = %lg, modt = %lg, t0 = %lg\n",subday,modt,t0);
      }
    }
    else {
      if ( modt < end ) {
	corr = m0 + m1 * modt + m2 * modt * modt ;
	fprintf(stderr,"computed corr = %lg microsecs (t0 = %lg secs)\n",corr,t0);
	break ;
      }
    }
  }

  if ( error || ( corr == -999999.0 ) ) {
    corr = 0.0 ;
    error-- ;
  }
/*  else {
 *    printf ("TimeZero: %f seconds\n", t0) ;
 *    printf ("PCA: %d microseconds; HEXTE %d microseconds\n",
 *	    (long) corr - 16, (long) corr) ;
 *  }
 */

  *tZero = t0 ;
  *tcorr = corr ;
  fclose (tdc) ;

  return error ;
}
