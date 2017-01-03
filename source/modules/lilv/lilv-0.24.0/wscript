#!/usr/bin/env python
import os
import shutil
import subprocess
import sys
import waflib.Options as Options
import waflib.extras.autowaf as autowaf
import waflib.Build as Build
import waflib.Logs as Logs

# Library and package version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
LILV_VERSION       = '0.24.0'
LILV_MAJOR_VERSION = '0'

# Mandatory waf variables
APPNAME = 'lilv'        # Package name for waf dist
VERSION = LILV_VERSION  # Package version for waf dist
top     = '.'           # Source directory
out     = 'build'       # Build directory

test_plugins = [
    'bad_syntax',
    'failed_instantiation',
    'failed_lib_descriptor',
    'lib_descriptor',
    'missing_descriptor',
    'missing_name',
    'missing_plugin',
    'missing_port',
    'missing_port_name',
    'new_version',
    'old_version'
]

def options(opt):
    opt.load('compiler_c')
    opt.load('compiler_cxx')
    opt.load('python')
    autowaf.set_options(opt, test=True)
    opt.add_option('--no-utils', action='store_true', dest='no_utils',
                   help='Do not build command line utilities')
    opt.add_option('--bindings', action='store_true', dest='bindings',
                   help='Build python bindings')
    opt.add_option('--dyn-manifest', action='store_true', dest='dyn_manifest',
                   help='Build support for dynamic manifests')
    opt.add_option('--no-bash-completion', action='store_true',
                   dest='no_bash_completion',
                   help='Do not install bash completion script in CONFIGDIR')
    opt.add_option('--static', action='store_true', dest='static',
                   help='Build static library')
    opt.add_option('--no-shared', action='store_true', dest='no_shared',
                   help='Do not build shared library')
    opt.add_option('--static-progs', action='store_true', dest='static_progs',
                   help='Build programs as static binaries')
    opt.add_option('--default-lv2-path', type='string', default='',
                   dest='default_lv2_path',
                   help='Default LV2 path to use if LV2_PATH is unset')

def configure(conf):
    conf.load('compiler_c')

    if Options.options.bindings:
        try:
            conf.load('python')
            conf.load('compiler_cxx')
            conf.check_python_headers()
            autowaf.define(conf, 'LILV_PYTHON', 1);
        except:
            Logs.warn('Failed to configure Python (%s)\n' % sys.exc_info()[1])

    autowaf.configure(conf)
    autowaf.set_c99_mode(conf)
    autowaf.display_header('Lilv Configuration')

    conf.env.BASH_COMPLETION = not Options.options.no_bash_completion
    conf.env.BUILD_UTILS     = not Options.options.no_utils
    conf.env.BUILD_SHARED    = not Options.options.no_shared
    conf.env.STATIC_PROGS    = Options.options.static_progs
    conf.env.BUILD_STATIC    = (Options.options.static or
                                Options.options.static_progs)

    if not conf.env.BUILD_SHARED and not conf.env.BUILD_STATIC:
        conf.fatal('Neither a shared nor a static build requested')

    autowaf.check_pkg(conf, 'lv2', uselib_store='LV2',
                      atleast_version='1.14.0', mandatory=True)
    autowaf.check_pkg(conf, 'serd-0', uselib_store='SERD',
                      atleast_version='0.14.0', mandatory=True)
    autowaf.check_pkg(conf, 'sord-0', uselib_store='SORD',
                      atleast_version='0.13.0', mandatory=True)
    autowaf.check_pkg(conf, 'sratom-0', uselib_store='SRATOM',
                      atleast_version='0.4.0', mandatory=True)
    autowaf.check_pkg(conf, 'sndfile', uselib_store='SNDFILE',
                      atleast_version='1.0.0', mandatory=False)

    defines = ['_POSIX_C_SOURCE=200809L', '_BSD_SOURCE', '_DEFAULT_SOURCE']
    if conf.env.DEST_OS == 'darwin':
        defines += ['_DARWIN_C_SOURCE']

    conf.check_cc(function_name='flock',
                  header_name='sys/file.h',
                  defines=defines,
                  define_name='HAVE_FLOCK',
                  mandatory=False)

    conf.check_cc(function_name='fileno',
                  header_name='stdio.h',
                  defines=defines,
                  define_name='HAVE_FILENO',
                  mandatory=False)

    conf.check_cc(function_name='clock_gettime',
                  header_name=['sys/time.h','time.h'],
                  defines=['_POSIX_C_SOURCE=200809L'],
                  define_name='HAVE_CLOCK_GETTIME',
                  uselib_store='CLOCK_GETTIME',
                  lib=['rt'],
                  mandatory=False)

    conf.check_cc(define_name   = 'HAVE_LIBDL',
                  lib           = 'dl',
                  mandatory     = False)

    autowaf.define(conf, 'LILV_VERSION', LILV_VERSION)
    if Options.options.dyn_manifest:
        autowaf.define(conf, 'LILV_DYN_MANIFEST', 1)

    lilv_path_sep = ':'
    lilv_dir_sep  = '/'
    if conf.env.DEST_OS == 'win32':
        lilv_path_sep = ';'
        lilv_dir_sep = '\\\\'

    autowaf.define(conf, 'LILV_PATH_SEP', lilv_path_sep)
    autowaf.define(conf, 'LILV_DIR_SEP',  lilv_dir_sep)

    # Set default LV2 path
    lv2_path = Options.options.default_lv2_path
    if lv2_path == '':
        if conf.env.DEST_OS == 'darwin':
            lv2_path = lilv_path_sep.join(['~/Library/Audio/Plug-Ins/LV2',
                                           '~/.lv2',
                                           '/usr/local/lib/lv2',
                                           '/usr/lib/lv2',
                                           '/Library/Audio/Plug-Ins/LV2'])
        elif conf.env.DEST_OS == 'haiku':
            lv2_path = lilv_path_sep.join(['~/.lv2',
                                           '/boot/common/add-ons/lv2'])
        elif conf.env.DEST_OS == 'win32':
            lv2_path = lilv_path_sep.join(['%APPDATA%\\\\LV2',
                                           '%COMMONPROGRAMFILES%\\\\LV2'])
        else:
            libdirname = os.path.basename(conf.env.LIBDIR)
            lv2_path = lilv_path_sep.join(['~/.lv2',
                                           '/usr/%s/lv2' % libdirname,
                                           '/usr/local/%s/lv2' % libdirname])
    autowaf.define(conf, 'LILV_DEFAULT_LV2_PATH', lv2_path)

    autowaf.set_lib_env(conf, 'lilv', LILV_VERSION)
    conf.write_config_header('lilv_config.h', remove=False)

    autowaf.display_msg(conf, 'Default LV2_PATH',
                        conf.env.LILV_DEFAULT_LV2_PATH)
    autowaf.display_msg(conf, 'Utilities',
                        bool(conf.env.BUILD_UTILS))
    autowaf.display_msg(conf, 'Unit tests',
                        bool(conf.env.BUILD_TESTS))
    autowaf.display_msg(conf, 'Dynamic manifest support',
                        bool(conf.env.LILV_DYN_MANIFEST))
    autowaf.display_msg(conf, 'Python bindings',
                        conf.is_defined('LILV_PYTHON'))

    conf.undefine('LILV_DEFAULT_LV2_PATH')  # Cmd line errors with VC++
    print('')

def build_util(bld, name, defines, libs=''):
    obj = bld(features     = 'c cprogram',
              source       = name + '.c',
              includes     = ['.', './src', './utils'],
              use          = 'liblilv',
              target       = name,
              defines      = defines,
              install_path = '${BINDIR}')
    autowaf.use_lib(bld, obj, 'SERD SORD SRATOM LV2 ' + libs)
    if not bld.env.BUILD_SHARED or bld.env.STATIC_PROGS:
        obj.use = 'liblilv_static'
    if bld.env.STATIC_PROGS:
        if not bld.env.MSVC_COMPILER:
            obj.lib = ['m']
            obj.env.SHLIB_MARKER = obj.env.STLIB_MARKER
            obj.linkflags        = ['-static', '-Wl,--start-group']
    return obj

def build(bld):
    # C/C++ Headers
    includedir = '${INCLUDEDIR}/lilv-%s/lilv' % LILV_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('lilv/*.h'))
    bld.install_files(includedir, bld.path.ant_glob('lilv/*.hpp'))

    lib_source = '''
        src/collections.c
        src/instance.c
        src/lib.c
        src/node.c
        src/plugin.c
        src/pluginclass.c
        src/port.c
        src/query.c
        src/scalepoint.c
        src/state.c
        src/ui.c
        src/util.c
        src/world.c
        src/zix/tree.c
    '''.split()

    lib      = []
    libflags = ['-fvisibility=hidden']
    defines  = []
    if bld.is_defined('HAVE_LIBDL'):
        lib    += ['dl']
    if bld.env.DEST_OS == 'win32':
        lib = []
    if bld.env.MSVC_COMPILER:
        libflags = []
        defines  = ['snprintf=_snprintf']

    # Pkgconfig file
    autowaf.build_pc(bld, 'LILV', LILV_VERSION, LILV_MAJOR_VERSION, [],
                     {'LILV_MAJOR_VERSION' : LILV_MAJOR_VERSION,
                      'LILV_PKG_DEPS'      : 'lv2 serd-0 sord-0 sratom-0',
                      'LILV_PKG_LIBS'      : ' -l'.join([''] + lib)})

    # Shared Library
    if bld.env.BUILD_SHARED:
        obj = bld(features        = 'c cshlib',
                  export_includes = ['.'],
                  source          = lib_source,
                  includes        = ['.', './src'],
                  name            = 'liblilv',
                  target          = 'lilv-%s' % LILV_MAJOR_VERSION,
                  vnum            = LILV_VERSION,
                  install_path    = '${LIBDIR}',
                  defines         = ['LILV_SHARED', 'LILV_INTERNAL'],
                  cflags          = libflags,
                  lib             = lib)
        autowaf.use_lib(bld, obj, 'SERD SORD SRATOM LV2')

    # Static library
    if bld.env.BUILD_STATIC:
        obj = bld(features        = 'c cstlib',
                  export_includes = ['.'],
                  source          = lib_source,
                  includes        = ['.', './src'],
                  name            = 'liblilv_static',
                  target          = 'lilv-%s' % LILV_MAJOR_VERSION,
                  vnum            = LILV_VERSION,
                  install_path    = '${LIBDIR}',
                  defines         = defines + ['LILV_INTERNAL'])
        autowaf.use_lib(bld, obj, 'SERD SORD SRATOM LV2')

    if bld.env.BUILD_TESTS:
        test_libs      = lib
        test_cflags    = ['']
        test_linkflags = ['']
        if not bld.env.NO_COVERAGE:
            test_cflags    += ['--coverage']
            test_linkflags += ['--coverage']

        # Test plugin library
        penv          = bld.env.derive()
        shlib_pattern = penv.cshlib_PATTERN
        if shlib_pattern.startswith('lib'):
            shlib_pattern = shlib_pattern[3:]
        penv.cshlib_PATTERN   = shlib_pattern
        shlib_ext = shlib_pattern[shlib_pattern.rfind('.'):]

        for p in ['test'] + test_plugins:
            obj = bld(features     = 'c cshlib',
                      env          = penv,
                      source       = 'test/%s.lv2/%s.c' % (p, p),
                      name         = p,
                      target       = 'test/%s.lv2/%s' % (p, p),
                      install_path = None,
                      defines      = defines,
                      cflags       = test_cflags,
                      linkflags    = test_linkflags,
                      lib          = test_libs,
                      uselib       = 'LV2')

        for p in test_plugins:
            if not bld.path.find_node('test/%s.lv2/test_%s.c' % (p, p)):
                continue

            obj = bld(features     = 'c cprogram',
                      source       = 'test/%s.lv2/test_%s.c' % (p, p),
                      target       = 'test/test_%s' % p,
                      includes     = ['.', './src'],
                      use          = 'liblilv_profiled',
                      install_path = None,
                      defines      = defines,
                      cflags       = test_cflags,
                      linkflags    = test_linkflags,
                      lib          = test_libs,
                      uselib       = 'LV2')
            autowaf.use_lib(bld, obj, 'SERD SORD SRATOM LV2')

        # Test plugin data files
        for p in ['test'] + test_plugins:
            for i in [ 'manifest.ttl.in', p + '.ttl.in' ]:
                bundle = 'test/%s.lv2/' % p
                bld(features     = 'subst',
                    source       = bundle + i,
                    target       = bundle + i.replace('.in', ''),
                    install_path = None,
                SHLIB_EXT    = shlib_ext)

        # Static profiled library (for unit test code coverage)
        obj = bld(features     = 'c cstlib',
                  source       = lib_source,
                  includes     = ['.', './src'],
                  name         = 'liblilv_profiled',
                  target       = 'lilv_profiled',
                  install_path = None,
                  defines      = defines + ['LILV_INTERNAL'],
                  cflags       = test_cflags,
                  linkflags    = test_linkflags,
                  lib          = test_libs)
        autowaf.use_lib(bld, obj, 'SERD SORD SRATOM LV2')

        # Unit test program
        testdir = os.path.abspath(autowaf.build_dir(APPNAME, 'test'))
        bpath   = os.path.join(testdir, 'test.lv2')
        bpath   = bpath.replace('\\', '/')
        testdir = testdir.replace('\\', '/')
        obj = bld(features     = 'c cprogram',
                  source       = 'test/lilv_test.c',
                  includes     = ['.', './src'],
                  use          = 'liblilv_profiled',
                  lib          = test_libs,
                  target       = 'test/lilv_test',
                  install_path = None,
                  defines      = (defines + ['LILV_TEST_BUNDLE=\"%s/\"' % bpath] +
                                  ['LILV_TEST_DIR=\"%s/\"' % testdir]),
                  cflags       = test_cflags,
                  linkflags    = test_linkflags)
        autowaf.use_lib(bld, obj, 'SERD SORD SRATOM LV2')

        if bld.is_defined('LILV_PYTHON'):
            # Copy Python bindings to build directory
            bld(features     = 'subst',
                is_copy      = True,
                source       = 'bindings/python/lilv.py',
                target       = 'lilv.py',
                install_path = '${PYTHONDIR}')

            # Copy Python unittest files
            for i in [ 'test_api.py' ]:
                bld(features     = 'subst',
                    is_copy      = True,
                    source       = 'bindings/test/python/' + i,
                    target       = 'bindings/' + i,
                    install_path = None)

            # Build bindings test plugin
            obj = bld(features     = 'c cshlib',
                      env          = penv,
                      source       = 'bindings/test/bindings_test_plugin.c',
                      name         = 'bindings_test_plugin',
                      target       = 'bindings/bindings_test_plugin.lv2/bindings_test_plugin',
                      install_path = None,
                      defines      = defines,
                      cflags       = test_cflags,
                      linkflags    = test_linkflags,
                      lib          = test_libs,
                      uselib       = 'LV2')

            # Bindings test plugin data files
            for i in [ 'manifest.ttl.in', 'bindings_test_plugin.ttl.in' ]:
                bld(features     = 'subst',
                    source       = 'bindings/test/' + i,
                    target       = 'bindings/bindings_test_plugin.lv2/' + i.replace('.in', ''),
                    install_path = None,
                    SHLIB_EXT    = shlib_ext)


    # Utilities
    if bld.env.BUILD_UTILS:
        utils = '''
            utils/lilv-bench
            utils/lv2info
            utils/lv2ls
        '''
        for i in utils.split():
            build_util(bld, i, defines)

    if bld.env.HAVE_SNDFILE:
        obj = build_util(bld, 'utils/lv2apply', defines, 'SNDFILE')

    # lv2bench (less portable than other utilities)
    if bld.is_defined('HAVE_CLOCK_GETTIME') and not bld.env.STATIC_PROGS:
        obj = build_util(bld, 'utils/lv2bench', defines)
        if not bld.env.MSVC_COMPILER:
            obj.lib = ['rt']

    # Documentation
    autowaf.build_dox(bld, 'LILV', LILV_VERSION, top, out)

    # Man pages
    bld.install_files('${MANDIR}/man1', bld.path.ant_glob('doc/*.1'))

    # Bash completion
    if bld.env.BASH_COMPLETION:
        bld.install_as(
            '${SYSCONFDIR}/bash_completion.d/lilv', 'utils/lilv.bash_completion')

    bld.add_post_fun(autowaf.run_ldconfig)
    if bld.env.DOCS:
        bld.add_post_fun(fix_docs)

def fix_docs(ctx):
    if ctx.cmd == 'build':
        autowaf.make_simple_dox(APPNAME)

def upload_docs(ctx):
    import glob
    os.system('rsync -ravz --delete -e ssh build/doc/html/ drobilla@drobilla.net:~/drobilla.net/docs/lilv/')
    for page in glob.glob('doc/*.[1-8]'):
        os.system('soelim %s | pre-grohtml troff -man -wall -Thtml | post-grohtml > build/%s.html' % (page, page))
        os.system('rsync -avz --delete -e ssh build/%s.html drobilla@drobilla.net:~/drobilla.net/man/' % page)

def test(ctx):
    assert ctx.env.BUILD_TESTS, "You have run waf configure without the --test flag. No tests were run."
    autowaf.pre_test(ctx, APPNAME)
    if ctx.is_defined('LILV_PYTHON'):
        os.environ['LD_LIBRARY_PATH'] = os.getcwd()
        autowaf.run_tests(ctx, APPNAME, ['python -m unittest discover bindings/'])
    os.environ['PATH'] = 'test' + os.pathsep + os.getenv('PATH')

    Logs.pprint('GREEN', '')
    autowaf.run_test(ctx, APPNAME, 'lilv_test', dirs=['./src','./test'], name='lilv_test')

    for p in test_plugins:
        test_prog = 'test_' + p + ' ' + ('test/%s.lv2/' % p)
        if os.path.exists('test/test_' + p):
            autowaf.run_test(ctx, APPNAME, test_prog, 0,
                             dirs=['./src','./test','./test/%s.lv2' % p])

    autowaf.post_test(ctx, APPNAME)
    try:
        shutil.rmtree('state')
    except:
        pass

def lint(ctx):
    subprocess.call('cpplint.py --filter=+whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-build/header_guard,-readability/casting,-readability/todo,-build/include,-runtime/sizeof src/* lilv/*', shell=True)

def posts(ctx):
    path = str(ctx.path.abspath())
    autowaf.news_to_posts(
        os.path.join(path, 'NEWS'),
        {'title'        : 'Lilv',
         'description'  : autowaf.get_blurb(os.path.join(path, 'README')),
         'dist_pattern' : 'http://download.drobilla.net/lilv-%s.tar.bz2'},
        { 'Author' : 'drobilla',
          'Tags'   : 'Hacking, LAD, LV2, Lilv' },
        os.path.join(out, 'posts'))
