#!/usr/bin/env python
import os
from os.path import join
from waflib.extras.test_base import summary
from waflib.extras.symwaf2ic import get_toplevel_path

def depends(dep):
    dep('sctrltp')
    dep('lib-rcf')
    dep('hate')
    dep('halco')
    dep('code-format')
    dep.recurse('pyflange')


def options(opt):
    opt.load('compiler_c')
    opt.load('compiler_cxx')
    opt.load('boost')
    opt.load("test_base")
    opt.load("gtest")
    opt.load("doxygen")
    opt.recurse('pyflange')


def configure(conf):
    conf.load('compiler_c')
    conf.load('compiler_cxx')
    conf.load('boost')
    conf.check_boost(lib='coroutine context', uselib_store='BOOST4HXCOMM')
    conf.check_boost(lib='program_options', uselib_store='BOOST4HXCOMMTOOLS')
    conf.load("test_base")
    conf.load("gtest")
    conf.check_cxx(mandatory=True, header_name='cereal/cereal.hpp')
    conf.check_cxx(lib='tbb', uselib_store="TBB")
    conf.load("doxygen")
    conf.recurse('pyflange')


def build(bld):
    bld.env.DLSvx_HARDWARE_AVAILABLE = "cube" == os.environ.get("SLURM_JOB_PARTITION")

    bld(target          = 'hxcomm_inc',
        export_includes = 'include'
    )

    bld(
        target       = 'hxcomm',
        features     = 'use',
        use          = ['hxcomm_inc', 'arqstream_obj', 'BOOST4HXCOMM', 'rcf-sf-only',
                        'flange', 'hate_inc', 'halco_hicann_dls_vx', 'TBB'],
    )

    bld.shlib(
        target       = 'flange',
        features     = 'cxx',
        source       = bld.path.ant_glob('src/flange/*.cpp'),
        use          = ['hxcomm_inc', 'rcf-sf-only'],
        install_path = '${PREFIX}/lib',
    )

    bld(
        target       = 'hxcomm_example_arq',
        features     = 'cxx cxxprogram',
        source       = ['example/hxcomm_arq.cpp'],
        use          = ['hxcomm', 'BOOST4HXCOMMTOOLS'],
        install_path = '${PREFIX}/bin',
    )

    bld(
        target       = 'hxcomm_example_sim',
        features     = 'cxx cxxprogram',
        source       = ['example/hxcomm_sim.cpp'],
        use          = ['hxcomm', 'BOOST4HXCOMMTOOLS'],
        install_path = '${PREFIX}/bin',
    )

    bld(
        target       = 'hxcomm_example_loopback_throughput',
        features     = 'cxx cxxprogram',
        source       = ['example/hxcomm_loopback_throughput.cpp'],
        use          = ['hxcomm', 'BOOST4HXCOMMTOOLS'],
        install_path = '${PREFIX}/bin',
    )

    bld(
        target          = 'hxcomm_tests_inc',
        export_includes = 'tests/common'
    )

    bld.shlib(
        target       = 'hxcomm_tests_helper',
        features     = 'cxx',
        source       = bld.path.ant_glob('tests/common/test-helper.cpp'),
        use          = ['hxcomm', 'hxcomm_tests_inc'],
    )

    bld(
        target       = 'hxcomm_swtests',
        features     = 'gtest cxx cxxprogram',
        source       = bld.path.ant_glob('tests/sw/hxcomm/test-*.cpp',
                           excl='tests/sw/hxcomm/test-*_throughput.cpp'),
        use          = ['hxcomm', 'hxcomm_tests_helper'],
    )

    bld(
        target       = 'hxcomm_throughputtests',
        features     = 'gtest cxx cxxprogram',
        source       = bld.path.ant_glob('tests/sw/hxcomm/test-*_throughput.cpp'),
        use          = ['hxcomm', 'hxcomm_tests_helper'],
        cxxflags     = ['-O2'],
    )

    bld(target          = 'hxcomm_hwtests_inc',
        export_includes = 'tests/hw/hxcomm/connection_hw'
    )

    bld(
        target       = 'hxcomm_hwtests',
        features     = 'gtest cxx cxxprogram',
        source       = bld.path.ant_glob('tests/hw/hxcomm/test-*.cpp'),
        skip_run     = not bld.env.DLSvx_HARDWARE_AVAILABLE,
        test_main    = 'tests/hw/hxcomm/main.cpp',
        use          = ['hxcomm', 'hxcomm_tests_helper', 'hxcomm_hwtests_inc', 'BOOST4HXCOMMTOOLS'],
    )

    bld(target          = 'hxcomm_simtests_inc',
        export_includes = 'tests/hw/hxcomm/connection_sim'
    )

    bld(
        target       = 'hxcomm_simtests',
        features     = 'gtest cxx cxxprogram',
        source       = bld.path.ant_glob('tests/hw/hxcomm/test-*.cpp'),
        skip_run     = True,
        test_main    = 'tests/hw/hxcomm/main.cpp',
        use          = ['hxcomm', 'hxcomm_tests_helper', 'hxcomm_simtests_inc', 'BOOST4HXCOMMTOOLS'],
    )

    bld(
        target       = 'flange_swtests',
        features     = 'gtest cxx cxxprogram',
        source       = bld.path.ant_glob('tests/sw/flange/test-*.cpp'),
        use          = ['flange'],
    )

    bld.recurse('pyflange')

    bld(
        features = 'doxygen',
        name = 'hxcomm_documentation',
        doxyfile = bld.root.make_node(join(get_toplevel_path(), "code-format", "doxyfile")),
        install_path = 'doc/hxcomm',
        pars = {
            "PROJECT_NAME": "\"HX Communication\"",
            "INPUT": join(get_toplevel_path(), "hxcomm", "include"),
            "PREDEFINED": "GENPYBIND()= GENPYBIND_TAG_FLANGE=",
            "OUTPUT_DIRECTORY": join(get_toplevel_path(), "build", "hxcomm", "doc")
        },
    )

    # Create test summary (to stdout and XML file)
    bld.add_post_fun(summary)
