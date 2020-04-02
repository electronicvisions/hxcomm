#!/usr/bin/env python
import os
from os.path import join
from socket import gethostname
from waflib.extras.test_base import summary
from waflib.extras.symwaf2ic import get_toplevel_path

def depends(dep):
    dep('sctrltp')
    dep('rant')
    dep('hate')
    dep('code-format')
    dep('logger')
    dep('visions-slurm', branch='production')
    dep('flange')


def options(opt):
    opt.load('compiler_c')
    opt.load('compiler_cxx')
    opt.load('boost')
    opt.load("test_base")
    opt.load("gtest")
    opt.load("doxygen")
    opt.add_option("--hxcomm-loglevel",
                   choices=["trace", "debug", "info", "warning", "error", "fatal"],
                   default="info",
                   help="Maximal loglevel to compile in.")


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
    conf.define(
        "HXCOMM_LOG_THRESHOLD",
        {'trace':   0,
         'debug':   1,
         'info':    2,
         'warning': 3,
         'error':   4,
         'fatal':   5}[conf.options.hxcomm_loglevel]
    )


def build_loopbackconnection_test(bld):
    """
    Build Loopbackconnection tests for a variety of parameter combinations.
    This is done via defines so that each target's build time is reduced
    and parallel build can be executed.
    """
    loopbackconnection_obj_targets = []
    types = ['uint8_t', 'uint16_t', 'uint32_t', 'uint64_t']
    header_alignments = [1, 3, 8]
    for subword_type in types:
        for phyword_type in types:
            for header_alignment in header_alignments:
                target = 'hxcomm_loopbackconnection_obj_' + subword_type + \
                    '_' + phyword_type + '_' + str(header_alignment)
                bld(
                    target       = target,
                    features     = 'cxx',
                    source       = 'tests/sw/hxcomm/test-loopbackconnection.cpp',
                    defines      = ['SUBWORD_TYPE=' + subword_type,
                                    'PHYWORD_TYPE=' + phyword_type,
                                    'HEADER_ALIGNMENT=' + str(header_alignment)],
                    use          = ['hxcomm', 'hxcomm_tests_helper'],
                )
                loopbackconnection_obj_targets.append(target)
    return loopbackconnection_obj_targets


def build(bld):
    bld.env.DLSvx_HARDWARE_AVAILABLE = "cube" == os.environ.get("SLURM_JOB_PARTITION")
    bld.env.DLSvx_SIM_AVAILABLE = "FLANGE_SIMULATION_RCF_PORT" in os.environ

    bld(target          = 'hxcomm_inc',
        export_includes = 'include'
    )

    bld(
        target       = 'hxcomm',
        features     = 'use',
        use          = ['hxcomm_inc', 'arqstream_obj', 'BOOST4HXCOMM',
                        'flange', 'rant', 'hate_inc', 'TBB', 'logger_obj', 'visions-slurm_inc'],
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

    loopbackconnection_obj_targets = build_loopbackconnection_test(bld)

    bld(
        target       = 'hxcomm_swtests',
        features     = 'gtest cxx cxxprogram',
        source       = bld.path.ant_glob('tests/sw/hxcomm/test-*.cpp',
                           excl='tests/sw/hxcomm/test-*_throughput.cpp tests/sw/hxcomm/test-loopbackconnection.cpp'),
        use          = ['hxcomm', 'hxcomm_tests_helper'] + loopbackconnection_obj_targets,
    )

    bld(
        target       = 'hxcomm_throughputtests',
        features     = 'gtest cxx cxxprogram',
        source       = bld.path.ant_glob('tests/sw/hxcomm/test-*_throughput.cpp'),
        use          = ['hxcomm', 'hxcomm_tests_helper'],
        cxxflags     = ['-O2'],
        # Throughput targets are only valid for HBPHosts and AMTHosts
        skip_run     = not (gethostname().startswith("HBPHost") or
                            gethostname().startswith("AMTHost"))
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
        skip_run     = not bld.env.DLSvx_SIM_AVAILABLE,
        test_main    = 'tests/hw/hxcomm/main.cpp',
        use          = ['hxcomm', 'hxcomm_tests_helper', 'hxcomm_simtests_inc', 'BOOST4HXCOMMTOOLS'],
        test_timeout = 60
    )

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
