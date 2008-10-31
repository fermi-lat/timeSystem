/**
    \mainpage timeSystem package

    \author Masaharu Hirayama hirayama@jca.umbc.edu
            James Peachey James.Peachey-1@nasa.gov

    \section intro Introduction
    The timeSystem package contains a rational and extensible set of
    abstractions for representing times in various time systems,
    and for transparently converting to/from all time systems on-the-fly
    as needed, while preserving sufficient precision.

    In addition, the timeSystem package has a single executable, gtbary,
    which performs a barycentering time correction to fits files
    using GLAST orbit files. The heart of this executable is taken
    from axBary by Arnold Rots at SAO. Minimal modifications were
    made to axBary to work with GLAST orbit files. This is also
    equivalent to the barycorr tool in HEADAS.

    The executable is fairly self-explanatory. Upon startup, the user
    will be prompted for the input file whose times will be corrected,
    the orbit file to use for the correction, the output file (which
    may be the same as the input file) and the Right Ascension (RA)
    and Declination (Dec) of the source (pulsar) location from which
    to correct the photons.

    Three additional ancillary files are needed by gtbary/axBary
    in order to perform the correction: JPLEPH.405, leapsec.fits
    and tai-utc.dat. The program looks for these files in the
    directory given by the TIMING_DIR environment variable. For
    the moment, these files are included in the data subdirectory
    of the timeSystem package.

    \section error-handling Error Handling
    In the event that an error occurs during the correction, the
    input file will be left in its original state, even if the
    correction is being performed in place. When debugging is enabled
    (by typing debug=yes on the command line), a temporary file
    containing the state of the correction when the error occurred will
    be left for investigative purposes. The temporary file name is
    identical to the output file name with an added suffix of .tmp.

    \section parameters Application Parameters

    \subsection key Key To Parameter Descriptions
\verbatim
Automatic parameters:
par_name [ = value ] type

Hidden parameters:
(par_name = value ) type

Where "par_name" is the name of the parameter, "value" is the
default value, and "type" is the type of the parameter. The
type is enclosed in square brackets.

Examples:
infile [file]
    Describes an automatic (queried) file-type parameter with
    no default value.

(plot = yes) [bool]
    Describes a hidden bool-type parameter named plot, whose
    default value is yes (true).
\endverbatim

    \subsection general gtbary Parameters
\verbatim
evfile [file name]
    Name of input event file, FT1 format or equivalent.

scfile [file name]
    Name of input spacecraft data file, FT2 format or equivalent.

outfile [file name]
    Name of output file. If the same as evfile, the arrival time
    correction will be performed in situ in the input file (which must
    therefore be writable).

ra [double]
    Right Ascension of point source in degrees for which to perform
    the arrival time correction.

dec [double]
    Declination of point source in degrees for which to perform the
    arrival time correction.

(tcorrect = BARY) [enumerated string (GEO|BARY)]
    Arrival time correction to apply. Choose BARY to apply the
    barycentric correction, and GEO the geocentric correction.

(solareph = JPL DE405) [enumerated string (JPL DE200|JPL DE405)]
    Solar system ephemeris for the barycentric correction.

(angtol = 1.e-8) [double]
    Angular tolerance in degrees in comparison of two source
    positions, one for which the barycentric correction is performed,
    and another given by RA_NOM and DEC_NOM header keyword of an event
    file to which the barycentric correction has already been applied.
    This parameter only has effect if the barycentric correction has
    been applied to an input event data file.  If the two source
    positions are separate from each other by this amount or less,
    then they will be considered to be the same position.  Otherwise
    an error will be generated.  The sign of the parameter value is
    ignored.  For example, setting angtol=-1.0 results in giving
    angular tolerance of 1 degree.

(timefield = TIME) [string]
    Name of the field containing the time values for temporal
    analysis.

(sctable = SC_DATA) [string]
    Name of the FITS table containing the spacecraft data.

(leapsecfile = DEFAULT) [file name]
    Name of the file containing the name of the leap second table, in
    OGIP-compliant leap second table format. If leapsecfile is the
    string DEFAULT, the default leap-second file (leapsec.fits), which
    is distributed with the extFiles package, will be used.
\endverbatim

*/
