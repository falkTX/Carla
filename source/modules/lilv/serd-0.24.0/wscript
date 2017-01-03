#!/usr/bin/env python
import glob
import os
import shutil
import subprocess
import waflib.Logs as Logs
import waflib.Options as Options
import waflib.extras.autowaf as autowaf

# Library and package version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
SERD_VERSION       = '0.24.0'
SERD_MAJOR_VERSION = '0'

# Mandatory waf variables
APPNAME = 'serd'        # Package name for waf dist
VERSION = SERD_VERSION  # Package version for waf dist
top     = '.'           # Source directory
out     = 'build'       # Build directory

def options(opt):
    opt.load('compiler_c')
    autowaf.set_options(opt, test=True)
    opt.add_option('--no-utils', action='store_true', dest='no_utils',
                   help='Do not build command line utilities')
    opt.add_option('--stack-check', action='store_true', dest='stack_check',
                   help='Include runtime stack sanity checks')
    opt.add_option('--static', action='store_true', dest='static',
                   help='Build static library')
    opt.add_option('--no-shared', action='store_true', dest='no_shared',
                   help='Do not build shared library')
    opt.add_option('--static-progs', action='store_true', dest='static_progs',
                   help='Build programs as static binaries')
    opt.add_option('--largefile', action='store_true', dest='largefile',
                   help='Build with large file support on 32-bit systems')
    opt.add_option('--no-posix', action='store_true', dest='no_posix',
                   help='Do not use posix_memalign, posix_fadvise, and fileno, even if present')

def configure(conf):
    conf.load('compiler_c')
    autowaf.configure(conf)
    autowaf.display_header('Serd Configuration')
    autowaf.set_c99_mode(conf)

    conf.env.BUILD_UTILS  = not Options.options.no_utils
    conf.env.BUILD_SHARED = not Options.options.no_shared
    conf.env.STATIC_PROGS = Options.options.static_progs
    conf.env.BUILD_STATIC = (Options.options.static or
                             Options.options.static_progs)

    if not conf.env.BUILD_SHARED and not conf.env.BUILD_STATIC:
        conf.fatal('Neither a shared nor a static build requested')

    if Options.options.stack_check:
        autowaf.define(conf, 'SERD_STACK_CHECK', SERD_VERSION)

    if Options.options.largefile:
        conf.env.append_unique('DEFINES', ['_FILE_OFFSET_BITS=64'])

    if not Options.options.no_posix:
        conf.check(function_name = 'posix_memalign',
                   header_name   = 'stdlib.h',
                   define_name   = 'HAVE_POSIX_MEMALIGN',
                   defines       = ['_POSIX_C_SOURCE=200809L'],
                   mandatory     = False)

        conf.check(function_name = 'posix_fadvise',
                   header_name   = 'fcntl.h',
                   define_name   = 'HAVE_POSIX_FADVISE',
                   defines       = ['_POSIX_C_SOURCE=200809L'],
                   mandatory     = False)

        conf.check(function_name = 'fileno',
                   header_name   = 'stdio.h',
                   define_name   = 'HAVE_FILENO',
                   defines       = ['_POSIX_C_SOURCE=200809L'],
                   mandatory     = False)

    autowaf.define(conf, 'SERD_VERSION', SERD_VERSION)
    autowaf.set_lib_env(conf, 'serd', SERD_VERSION)
    conf.write_config_header('serd_config.h', remove=False)

    autowaf.display_msg(conf, 'Utilities', bool(conf.env.BUILD_UTILS))
    autowaf.display_msg(conf, 'Unit tests', bool(conf.env.BUILD_TESTS))
    print('')

lib_source = [
    'src/env.c',
    'src/node.c',
    'src/reader.c',
    'src/string.c',
    'src/uri.c',
    'src/writer.c',
]

def build(bld):
    # C Headers
    includedir = '${INCLUDEDIR}/serd-%s/serd' % SERD_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('serd/*.h'))

    # Pkgconfig file
    autowaf.build_pc(bld, 'SERD', SERD_VERSION, SERD_MAJOR_VERSION, [],
                     {'SERD_MAJOR_VERSION' : SERD_MAJOR_VERSION})

    libflags = ['-fvisibility=hidden']
    libs     = ['m']
    defines  = []
    if bld.env.MSVC_COMPILER:
        libflags = []
        libs     = []
        defines  = ['snprintf=_snprintf']

    # Shared Library
    if bld.env.BUILD_SHARED:
        bld(features        = 'c cshlib',
            export_includes = ['.'],
            source          = lib_source,
            includes        = ['.', './src'],
            lib             = libs,
            name            = 'libserd',
            target          = 'serd-%s' % SERD_MAJOR_VERSION,
            vnum            = SERD_VERSION,
            install_path    = '${LIBDIR}',
            defines         = defines + ['SERD_SHARED', 'SERD_INTERNAL'],
            cflags          = libflags)

    # Static library
    if bld.env.BUILD_STATIC:
        bld(features        = 'c cstlib',
            export_includes = ['.'],
            source          = lib_source,
            includes        = ['.', './src'],
            lib             = libs,
            name            = 'libserd_static',
            target          = 'serd-%s' % SERD_MAJOR_VERSION,
            vnum            = SERD_VERSION,
            install_path    = '${LIBDIR}',
            defines         = defines + ['SERD_INTERNAL'])

    if bld.env.BUILD_TESTS:
        test_libs       = libs
        test_cflags     = ['']
        test_linkflags  = ['']
        if not bld.env.NO_COVERAGE:
            test_cflags    += ['--coverage']
            test_linkflags += ['--coverage']

        # Profiled static library for test coverage
        bld(features     = 'c cstlib',
            source       = lib_source,
            includes     = ['.', './src'],
            lib          = test_libs,
            name         = 'libserd_profiled',
            target       = 'serd_profiled',
            install_path = '',
            defines      = defines + ['SERD_INTERNAL'],
            cflags       = test_cflags,
            linkflags    = test_linkflags)

        # Static profiled serdi for tests
        bld(features     = 'c cprogram',
            source       = 'src/serdi.c',
            includes     = ['.', './src'],
            use          = 'libserd_profiled',
            lib          = test_libs,
            target       = 'serdi_static',
            install_path = '',
            defines      = defines,
            cflags       = test_cflags,
            linkflags    = test_linkflags)

        # Unit test program
        bld(features     = 'c cprogram',
            source       = 'tests/serd_test.c',
            includes     = ['.', './src'],
            use          = 'libserd_profiled',
            lib          = test_libs,
            target       = 'serd_test',
            install_path = '',
            defines      = defines,
            cflags       = test_cflags,
            linkflags    = test_linkflags)

    # Utilities
    if bld.env.BUILD_UTILS:
        obj = bld(features     = 'c cprogram',
                  source       = 'src/serdi.c',
                  target       = 'serdi',
                  includes     = ['.', './src'],
                  use          = 'libserd',
                  lib          = libs,
                  install_path = '${BINDIR}')
        if not bld.env.BUILD_SHARED or bld.env.STATIC_PROGS:
            obj.use = 'libserd_static'
        if bld.env.STATIC_PROGS:
            obj.env.SHLIB_MARKER = obj.env.STLIB_MARKER
            obj.linkflags        = ['-static']

    # Documentation
    autowaf.build_dox(bld, 'SERD', SERD_VERSION, top, out)

    # Man page
    bld.install_files('${MANDIR}/man1', 'doc/serdi.1')

    bld.add_post_fun(autowaf.run_ldconfig)
    if bld.env.DOCS:
        bld.add_post_fun(fix_docs)

def lint(ctx):
    subprocess.call('cpplint.py --filter=+whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-build/header_guard,-readability/casting,-readability/todo,-build/include src/* serd/*', shell=True)

def amalgamate(ctx):
    shutil.copy('serd/serd.h', 'build/serd.h')
    amalgamation = open('build/serd.c', 'w')

    serd_internal_h = open('src/serd_internal.h')
    for l in serd_internal_h:
        if l == '#include "serd/serd.h"\n':
            amalgamation.write('#include "serd.h"\n')
        else:
            amalgamation.write(l)
    serd_internal_h.close()

    for f in lib_source:
        fd = open(f)
        amalgamation.write('\n/**\n   @file %s\n*/' % f)
        header = True
        for l in fd:
            if header:
                if l == '*/\n':
                    header = False
            else:
                if l != '#include "serd_internal.h"\n':
                    amalgamation.write(l)
        fd.close()
    amalgamation.close()

    for i in ['c', 'h']:
        Logs.info('Wrote build/serd.%s' % i)

def fix_docs(ctx):
    if ctx.cmd == 'build':
        autowaf.make_simple_dox(APPNAME)

def upload_docs(ctx):
    os.system('rsync -ravz --delete -e ssh build/doc/html/ drobilla@drobilla.net:~/drobilla.net/docs/serd/')
    for page in glob.glob('doc/*.[1-8]'):
        os.system('soelim %s | pre-grohtml troff -man -wall -Thtml | post-grohtml > build/%s.html' % (page, page))
        os.system('rsync -avz --delete -e ssh build/%s.html drobilla@drobilla.net:~/drobilla.net/man/' % page)

def file_equals(patha, pathb, subst_from='', subst_to=''):
    fa = open(patha, 'rU')
    fb = open(pathb, 'rU')
    for line in fa:
        if line.replace(subst_from, subst_to) != fb.readline().replace(subst_from, subst_to):
            return False
    fa.close()
    fb.close()
    return True

def earl_assertion(test, passed, asserter):
    import datetime

    asserter_str = ''
    if asserter is not None:
        asserter_str = '\n\tearl:assertedBy <%s> ;' % asserter

    passed_str = 'earl:failed'
    if passed:
        passed_str = 'earl:passed'

    return '''
[]
	a earl:Assertion ;%s
	earl:subject <http://drobilla.net/sw/serd> ;
	earl:test <%s> ;
	earl:result [
		a earl:TestResult ;
		earl:outcome %s ;
		dc:date "%s"^^xsd:dateTime
	] .
''' % (asserter_str,
       test,
       passed_str,
       datetime.datetime.now().replace(microsecond=0).isoformat())

def check_output(out_filename, check_filename, subst_from='', subst_to=''):
    if not os.access(out_filename, os.F_OK):
        Logs.pprint('RED', 'FAIL: output %s is missing' % out_filename)
    elif not file_equals(check_filename, out_filename, subst_from, subst_to):
        Logs.pprint('RED', 'FAIL: %s != %s' % (os.path.abspath(out_filename),
                                               check_filename))
    else:
        return True

    return False

def test_thru(ctx, base, path, check_filename, flags):
    in_filename = os.path.join(ctx.path.abspath(), path);
    out_filename = path + '.thru'

    command = ('%s %s -i turtle -o turtle -p foo "%s" "%s" | '
               '%s -i turtle -o ntriples -c foo - "%s" > %s') % (
        'serdi_static', flags.ljust(5),
        in_filename, base,
        'serdi_static', base, out_filename)

    if autowaf.run_test(ctx, APPNAME, command, 0, name=out_filename):
        autowaf.run_test(ctx, APPNAME,
                         [out_filename + '.check', check_output(out_filename, check_filename, '_:docid', '_:genid')],
                         1)
    else:
        Logs.pprint('RED', 'FAIL: error running %s' % command)

def test_manifest(ctx, srcdir, testdir, report, base_uri):
    import rdflib
    import urlparse

    rdf  = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')
    rdfs = rdflib.Namespace('http://www.w3.org/2000/01/rdf-schema#')
    mf   = rdflib.Namespace('http://www.w3.org/2001/sw/DataAccess/tests/test-manifest#')
    rdft = rdflib.Namespace('http://www.w3.org/ns/rdftest#')
    earl = rdflib.Namespace('http://www.w3.org/ns/earl#')

    model = rdflib.ConjunctiveGraph()
    model.parse(os.path.join(srcdir, 'tests', testdir, 'manifest.ttl'),
                rdflib.URIRef(base_uri + 'manifest.ttl'),
                format='n3')

    asserter = ''
    if os.getenv('USER') == 'drobilla':
        asserter = 'http://drobilla.net/drobilla#me'

    def run_test(action_node, expected_return):
        output  = os.path.join('tests', testdir, action_node + '.out')
        action  = os.path.join(srcdir, 'tests', testdir, action_node)
        rel     = os.path.relpath(action, os.path.join(srcdir, 'tests', testdir))
        command = 'serdi_static -f "%s" "%s" > %s' % (action, base_uri + rel, output)

        return autowaf.run_test(ctx, APPNAME, command, expected_return, name=str(action))

    def run_tests(test_class, expected_return, check_result=False):
        autowaf.begin_tests(ctx, APPNAME, str(test_class))
        for i in sorted(model.triples([None, rdf.type, test_class])):
            test        = i[0]
            name        = model.value(test, mf.name, None)
            action_node = model.value(test, mf.action, None)[len(base_uri):]
            passed      = run_test(action_node, expected_return)

            if passed and check_result:
                result_node = model.value(test, mf.result, None)[len(base_uri):]
                action      = os.path.join('tests', testdir, action_node)
                output      = action + '.out'
                result      = os.path.join(srcdir, 'tests', testdir, result_node)
                passed      = check_output(output, result)

                test_thru(ctx, base_uri + action_node, action, result, "")

            report.write(earl_assertion(test, passed, asserter))
        autowaf.end_tests(ctx, APPNAME, str(test_class))

    run_tests(rdft.TestTurtlePositiveSyntax, 0)
    run_tests(rdft.TestTurtleNegativeSyntax, 1)
    run_tests(rdft.TestTurtleNegativeEval, 1)
    run_tests(rdft.TestTurtleEval, 0, True)

def test(ctx):
    blddir = autowaf.build_dir(APPNAME, 'tests')
    for i in ['', 'bad', 'good', 'new', 'TurtleTests', 'extra']:
        try:
            os.makedirs(os.path.join(blddir, i))
        except:
            pass

    for i in glob.glob(blddir + '/*.*'):
        os.remove(i)

    srcdir   = ctx.path.abspath()
    orig_dir = os.path.abspath(os.curdir)

    os.chdir(os.path.join(srcdir, 'tests', 'good'))
    old_good_tests = glob.glob('*.ttl')
    old_good_tests.sort()
    old_good_tests.remove('manifest.ttl')
    good_tests = { 'good': old_good_tests }
    os.chdir(orig_dir)

    os.chdir(srcdir)
    bad_tests = glob.glob('tests/bad/*.ttl')
    bad_tests.sort()
    os.chdir(orig_dir)

    autowaf.pre_test(ctx, APPNAME)

    os.environ['PATH'] = '.' + os.pathsep + os.getenv('PATH')

    autowaf.run_test(ctx, APPNAME, 'serd_test', dirs=['.'])

    autowaf.run_tests(ctx, APPNAME, [
            'serdi_static -q -o turtle "%s/tests/good/base.ttl" "base.ttl" > tests/good/base.ttl.out' % srcdir
    ], 0, name='base')

    if not file_equals('%s/tests/good/base.ttl' % srcdir, 'tests/good/base.ttl.out'):
        Logs.pprint('RED', 'FAIL: build/tests/base.ttl.out is incorrect')

    nul = os.devnull
    autowaf.run_tests(ctx, APPNAME, [
            'serdi_static "file://%s/tests/good/manifest.ttl" > %s' % (srcdir, nul),
#            'serdi_static %s/tests/good/UTF-8.ttl > %s' % (srcdir, nul),
            'serdi_static -v > %s' % nul,
            'serdi_static -h > %s' % nul,
            'serdi_static -s "<foo> a <#Thingie> ." > %s' % nul,
            'serdi_static %s > %s' % (nul, nul)
    ], 0, name='serdi-cmd-good')

    # Test read error by reading a directory
    autowaf.run_test(ctx, APPNAME, 'serdi_static "file://%s/"' % srcdir,
                     1, name='read_error')

    # Test write error by writing to /dev/full
    if os.path.exists('/dev/full'):
        autowaf.run_test(ctx, APPNAME,
                         'serdi_static "file://%s/tests/good/manifest.ttl" > /dev/full' % srcdir,
                         1, name='write_error')

    autowaf.run_tests(ctx, APPNAME, [
            'serdi_static -q "file://%s/tests/bad-id-clash.ttl" > %s' % (srcdir, nul),
            'serdi_static > %s' % nul,
            'serdi_static ftp://example.org/unsupported.ttl > %s' % nul,
            'serdi_static -i > %s' % nul,
            'serdi_static -o > %s' % nul,
            'serdi_static -z > %s' % nul,
            'serdi_static -p > %s' % nul,
            'serdi_static -c > %s' % nul,
            'serdi_static -r > %s' % nul,
            'serdi_static -i illegal > %s' % nul,
            'serdi_static -o illegal > %s' % nul,
            'serdi_static -i turtle > %s' % nul,
            'serdi_static /no/such/file > %s' % nul],
                      1, name='serdi-cmd-bad')

    def test_base(test):
        return ('http://www.w3.org/2001/sw/DataAccess/df1/tests/'
                + test.replace('\\', '/'))

    # Good tests
    for tdir, tests in good_tests.items():
        commands = []

        for test in tests:
            for lax in ['', '-l']:
                path = os.path.join('tests', tdir, test)
                commands += [ 'serdi_static %s -f "%s" "%s" > %s.out' % (
                    lax, os.path.join(srcdir, path), test_base(test), path) ]

        autowaf.run_tests(ctx, APPNAME, commands, 0, name=tdir)

        verify_tests = []
        for test in tests:
            check_filename = os.path.join(
                srcdir, 'tests', tdir, test.replace('.ttl', '.nt'))
            out_filename = os.path.join('tests', tdir, test + '.out')
            if check_output(out_filename, check_filename):
                verify_tests += [[out_filename, 0]]
            else:
                verify_tests += [[out_filename, 1]]
        autowaf.run_tests(ctx, APPNAME, verify_tests, name='verify_turtle_to_ntriples')

    # Bad tests
    commands = []
    for test in bad_tests:
        for lax in ['', '-l']:
            commands += [ 'serdi_static %s -q "%s" "%s" > %s.out' % (
                lax, os.path.join(srcdir, test), test_base(test), test) ]

    autowaf.run_tests(ctx, APPNAME, commands, 1, name='bad')

    # Don't do a round-trip test for test-id.ttl, IDs have changed
    good_tests['good'].remove('test-id.ttl')

    # Round-trip good tests
    autowaf.begin_tests(ctx, APPNAME, 'round_trip_good')
    for tdir, tests in good_tests.items():
        thru_tests = tests;

        commands = []
        num = 0
        for test in thru_tests:
            num += 1
            flags = ''
            if (num % 2 == 0):
                flags += '-b'
            if (num % 5 == 0):
                flags += ' -f'
            if (num % 3 == 0):
                flags += ' -r http://www.w3.org/'
            if (num % 7 == 0):
                flags += ' -e'

            path  = os.path.join('tests', tdir, test)
            check = os.path.join(srcdir, path.replace('.ttl', '.nt'))
            test_thru(ctx, test_base(test), path, check, flags)
    autowaf.end_tests(ctx, APPNAME, 'round_trip_good')

    # New manifest-driven tests
    try:
        report = open('earl.ttl', 'w')
        report.write('''@prefix earl: <http://www.w3.org/ns/earl#> .
@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n''')

        serd_ttl = open(os.path.join(srcdir, 'serd.ttl'))
        for line in serd_ttl:
            report.write(line)
        serd_ttl.close()
        turtle_tests = 'http://www.w3.org/2013/TurtleTests/'
        test_manifest(ctx, srcdir, 'TurtleTests', report, turtle_tests)
        report.close()

    except:
        pass

    autowaf.post_test(ctx, APPNAME)

def posts(ctx):
    path = str(ctx.path.abspath())
    autowaf.news_to_posts(
        os.path.join(path, 'NEWS'),
        {'title'        : 'Serd',
         'description'  : autowaf.get_blurb(os.path.join(path, 'README')),
         'dist_pattern' : 'http://download.drobilla.net/serd-%s.tar.bz2'},
        { 'Author' : 'drobilla',
          'Tags'   : 'Hacking, RDF, Serd' },
        os.path.join(out, 'posts'))
