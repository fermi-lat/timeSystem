import glob,os

Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

timeSystemLib = libEnv.StaticLibrary('timeSystem', listFiles(['src/*.cxx', 'src/*.c']))

progEnv.Tool('timeSystemLib')
gtbaryBin = progEnv.Program('gtbary', listFiles(['src/gtbary/*.cxx']))
test_timeSystemBin = progEnv.Program('test_timeSystem', listFiles(['src/test/*.cxx'])) 

progEnv.Tool('registerObjects', package = 'timeSystem', libraries = [timeSystemLib], includes = listFiles(['timeSystem/*.h']), binaries = [gtbaryBin], testApps = [test_timeSystemBin], pfiles = listFiles(['pfiles/*.par']))
