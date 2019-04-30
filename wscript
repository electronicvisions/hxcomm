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
    conf.check_boost(lib='program_options system coroutine context', uselib_store='BOOST4HXCOMM')
    conf.load("test_base")
    conf.load("gtest")
    conf.check_cxx(mandatory=True, header_name='cereal/cereal.hpp')
    conf.check_cxx(lib='tbb', uselib_store="TBB")
    conf.load("doxygen")
    conf.recurse('pyflange')


def build(bld):
    bld.env.DLSvx_HARDWARE_AVAILABLE = "cube" == os.environ.get("SLURM_JOB_PARTITION")

    bld(target          = 'hx_comm_inc',
        export_includes = 'include'
    )

    bld(target          = 'flange_inc',
        export_includes = 'include'
    )

    bld.shlib(
        target       = 'hx_comm',
        features     = 'cxx',
        source       = bld.path.ant_glob('src/hxcomm/*.cpp'),
        use          = ['hx_comm_inc', 'arqstream_obj', 'BOOST4HXCOMM', 'rcf-sf-only',
                        'flange', 'hate_inc', 'halco_common', 'TBB'],
        install_path = '${PREFIX}/lib',
    )

    bld.shlib(
        target       = 'flange',
        features     = 'cxx',
        source       = bld.path.ant_glob('src/flange/*.cpp'),
        use          = ['flange_inc', 'rcf-sf-only'],
        install_path = '${PREFIX}/lib',
    )

    bld(
        target       = 'hx_comm_example_arq',
        features     = 'cxx cxxprogram',
        source       = ['example/hx_comm_arq.cpp'],
        use          = ['hx_comm'],
        install_path = '${PREFIX}/bin',
    )

    bld(
        target       = 'hx_comm_example_sim',
        features     = 'cxx cxxprogram',
        source       = ['example/hx_comm_sim.cpp'],
        use          = ['hx_comm'],
        install_path = '${PREFIX}/bin',
    )

    bld(
        target       = 'hx_comm_example_pipe',
        features     = 'cxx cxxprogram',
        source       = ['example/hx_comm_pipe.cpp'],
        use          = ['hx_comm'],
        install_path = '${PREFIX}/bin',
    )

    bld(
        target       = 'hx_comm_example_loopback_throughput',
        features     = 'cxx cxxprogram',
        source       = ['example/hx_comm_loopback_throughput.cpp'],
        use          = ['hx_comm'],
        install_path = '${PREFIX}/bin',
    )

    bld(
        target       = 'hx_comm_example_double_buffer_throughput',
        features     = 'cxx cxxprogram',
        source       = ['example/hx_comm_double_buffer_throughput.cpp'],
        use          = ['hx_comm'],
        install_path = '${PREFIX}/bin',
    )

    bld(
        target       = 'hx_comm_example_encode_decode_throughput',
        features     = 'cxx cxxprogram',
        source       = ['example/hx_comm_encode_decode_throughput.cpp'],
        use          = ['hx_comm'],
        install_path = '${PREFIX}/bin',
    )

    bld(
        target          = 'hx_comm_tests_inc',
        export_includes = 'tests/common'
    )

    bld.shlib(
        target       = 'hx_comm_tests_helper',
        features     = 'cxx',
        source       = bld.path.ant_glob('tests/common/test-helper.cpp'),
        use          = ['hx_comm', 'hx_comm_tests_inc'],
    )

    bld(
        target       = 'hx_comm_swtests',
        features     = 'gtest cxx cxxprogram',
        source       = bld.path.ant_glob('tests/sw/hxcomm/test-*.cpp'),
        use          = ['hx_comm', 'hx_comm_tests_helper'],
    )

    bld(target          = 'hx_comm_hwtests_inc',
        export_includes = 'tests/hw/hxcomm/connection_hw'
    )

    bld(
        target       = 'hx_comm_hwtests',
        features     = 'gtest cxx cxxprogram',
        source       = bld.path.ant_glob('tests/hw/hxcomm/test-*.cpp'),
        skip_run     = not bld.env.DLSvx_HARDWARE_AVAILABLE,
        test_main    = 'tests/hw/hxcomm/main.cpp',
        use          = ['hx_comm', 'hx_comm_tests_helper', 'hx_comm_hwtests_inc'],
    )

    bld(target          = 'hx_comm_simtests_inc',
        export_includes = 'tests/hw/hxcomm/connection_sim'
    )

    bld(
        target       = 'hx_comm_simtests',
        features     = 'gtest cxx cxxprogram',
        source       = bld.path.ant_glob('tests/hw/hxcomm/test-*.cpp'),
        skip_run     = True,
        test_main    = 'tests/hw/hxcomm/main.cpp',
        use          = ['hx_comm', 'hx_comm_tests_helper', 'hx_comm_simtests_inc'],
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
            "OUTPUT_DIRECTORY": join(get_toplevel_path(), "build", "hxcomm", "doc")
        },
    )

    # Create test summary (to stdout and XML file)
    bld.add_post_fun(summary)
