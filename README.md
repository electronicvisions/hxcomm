Low-level communication with HICANN-X via HostARQ

# Setup

```
waf setup --project hxcomm

container="singularity exec --app visionary-dls /containers/stable/latest"
$container waf configure
srun -p compile -c8 -N1 --pty $container waf install
```

# Usage

Typically you would use the shared object in another project.
See `test/hx_comm.cpp` for an example.


# Contributing

In case you encounter bugs, please [file a work package](https://brainscales-r.kip.uni-heidelberg.de/projects/hxcomm/work_packages/) describing all steps required to reproduce the problem.
Don't just post in the chat, as the probability to get lost is very high.

Before committing any changes, make sure to run `git-clang-format` and add the resulting changes to your commit.
