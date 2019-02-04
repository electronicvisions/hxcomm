#!/usr/bin/env python
from waflib.extras.test_base import summary

def depends(dep):
    dep('sctrltp')
    dep('lib-rcf')


def options(opt):
    opt.load('compiler_c')
    opt.load('compiler_cxx')
    opt.load('boost')
    opt.load("test_base")
    opt.load("gtest")


def configure(conf):
    conf.load('compiler_c')
    conf.load('compiler_cxx')
    conf.load('boost')
    conf.check_boost(lib='program_options system', uselib_store='BOOST4HXCOMM')
    conf.load("test_base")
    conf.load("gtest")


def build(bld):
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
        use          = ['hx_comm_inc', 'arqstream_obj', 'BOOST4HXCOMM', 'rcf-sf-only'],
        install_path = '${PREFIX}/lib',
    )

    bld.shlib(
        target       = 'flange_simulator_control',
        features     = 'cxx',
        source       = bld.path.ant_glob('src/flange/*.cpp'),
        use          = ['flange_inc', 'rcf-sf-only'],
    )

    bld(
        target       = 'hx_comm_example_arq',
        features     = 'cxx cxxprogram',
        source       = ['example/hx_comm.cpp'],
        use          = ['hx_comm'],
        install_path = '${PREFIX}/bin',
    )

    bld(
        target       = 'hx_comm_swtests',
        features     = 'gtest cxx cxxprogram',
        source       = bld.path.ant_glob('tests/sw/hxcomm/test-*.cpp'),
        use          = ['hx_comm'],
    )

    bld(
        target       = 'flange_swtests',
        features     = 'gtest cxx cxxprogram',
        source       = bld.path.ant_glob('tests/sw/flange/test-*.cpp'),
        use          = ['flange_simulator_control'],
    )

    # Create test summary (to stdout and XML file)
    bld.add_post_fun(summary)


def doc(dox):
    pass
