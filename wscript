from waflib.extras.test_base import summary


def depends(dep):
    pass


def options(opt):
    opt.load("test_base")


def configure(cfg):
    cfg.load("test_base")


def build(bld):
    # Create test summary (to stdout and XML file)
    bld.add_post_fun(summary)


def doc(dox):
    pass
