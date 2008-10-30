# -*- python -*-
# $Id: SConscript,v 1.15 2008/10/28 21:31:06 glastrm Exp $
# Authors: James Peachey <James.Peachey-1@nasa.gov>
# Version: timeSystem-06-02-02

Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

libEnv.Tool('timeSystemLib', depsOnly = 1)
timeSystemLib = libEnv.StaticLibrary('timeSystem', listFiles(['src/*.cxx', 'src/*.c']))

progEnv.Tool('timeSystemLib')
gtbaryBin = progEnv.Program('gtbary', listFiles(['src/gtbary/*.cxx']))
test_timeSystemBin = progEnv.Program('test_timeSystem', listFiles(['src/test/*.cxx'])) 

progEnv.Tool('registerObjects', package = 'timeSystem', libraries = [timeSystemLib], includes = listFiles(['timeSystem/*.h']), binaries = [gtbaryBin],
             testApps = [test_timeSystemBin], pfiles = listFiles(['pfiles/*.par']), data = listFiles(['data/*'], recursive = True))
