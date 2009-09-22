# -*- python -*-
# $Id: SConscript,v 1.25 2009/09/19 00:31:34 hirayama Exp $
# Authors: James Peachey <James.Peachey-1@nasa.gov>
# Version: timeSystem-06-04-03

Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

libEnv.Tool('timeSystemLib', depsOnly = 1)
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
