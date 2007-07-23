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

    The executable is fairly self-explanatory. Upon startup, the
    user will be prompted for the input file whose times will be
    corrected, the orbit file to use for the correction, the
    output file (which may be the same as the input file) and
    the RA and DEC of the source (pulsar) location from which to
    correct the photons.

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
evfile [file]
    Name of input event file, FT1 format or equivalent.

scfile [file]
    Name of input spacecraft data file, FT2 format or equivalent.

outfile [file]
    Name of output file. If blank (or the same as evfile), the
    barycentric correction will be performed in situ in the input
    file (which must therefore be writable).

ra [double]
    RA of point source for which to perform the barycentric
    correction.

dec [double]
    DEC of point source for which to perform the barycentric
    correction.
\endverbatim

*/
