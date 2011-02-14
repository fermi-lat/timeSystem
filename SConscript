# -*- python -*-
# $Id: SConscript,v 1.35 2010/05/06 03:01:41 hirayama Exp $
# Authors: James Peachey <James.Peachey-1@nasa.gov>
# Version: timeSystem-06-05-00

Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

timeSystemLib = libEnv.StaticLibrary('timeSystem', listFiles(['src/*.cxx', 'src/*.c']))

progEnv.Tool('timeSystemLib')
gtbaryBin = progEnv.Program('gtbary', listFiles(['src/gtbary/*.cxx']))
test_timeSystemBin = progEnv.Program('test_timeSystem', listFiles(['src/test/*.cxx'])) 

progEnv.Tool('registerTargets', package = 'timeSystem',
             staticLibraryCxts = [[timeSystemLib, libEnv]],
             includes = listFiles(['timeSystem/*.h']),
             binaryCxts = [[gtbaryBin, progEnv]],
             testAppCxts = [[test_timeSystemBin, progEnv]],
             pfiles = listFiles(['pfiles/*.par']),
             data = listFiles(['data/*'], recursive = True))
