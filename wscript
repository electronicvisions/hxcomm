#!/usr/bin/env python
from waflib.extras.test_base import summary

def depends(dep):
    dep('sctrltp')

def options(opt):
    opt.load('compiler_c')
    opt.load('compiler_cxx')
    opt.load('boost')
    opt.load('post_task')
    opt.load("test_base")

def configure(conf):
    conf.load('compiler_c')
    conf.load('compiler_cxx')
    conf.load('boost')
    conf.load('post_task')
    conf.check_boost(lib='program_options system', uselib_store='BOOST4HXCOMM')
    conf.load("test_base")

def build(bld):
    # Create test summary (to stdout and XML file)
    bld.add_post_fun(summary)

    bld(target          = 'hx_comm_inc',
        export_includes = 'include'
    )

    bld.shlib(
        target       = 'hx_comm',
        features     = 'cxx',
        source       = bld.path.ant_glob('src/*.cpp'),
        use          = [ 'hx_comm_inc', 'arqstream_obj', 'BOOST4HXCOMM', 'rcf-sf-only'],
        install_path = '${PREFIX}/lib',
    )

    bld(
        target       = 'hx_comm_example_arq',
        features     = 'cxx cxxprogram',
        source       = ['example/hx_comm.cpp'],
        use          = ['hx_comm'],
        install_path = '${PREFIX}/bin',
    )

def doc(dox):
    pass
