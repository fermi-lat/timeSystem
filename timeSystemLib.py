#$Id$
def generate(env, **kw):
    if not kw.get('depsOnly',0):
        env.Tool('addLibrary', library = ['timeSystem'])
    env.Tool('st_facilitiesLib')
    env.Tool('st_streamLib')
    env.Tool('tipLib')
    env.Tool('st_appLib')
    env.Tool('addLibrary', library = env['cfitsioLibs'])

def exists(env):
    return 1
