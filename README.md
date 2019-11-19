# Low-level communication with HICANN-X via HostARQ

## How to build

```bash
# on hel, cat /etc/motd for latest instructions

module load waf

mkdir my_project && cd my_project/
module load localdir

# first build
waf setup --project hxcomm
srun -p compile -c8 -- singularity exec --app visionary-dls /containers/stable/latest waf configure install --test-execnone

# run (software) tests
singularity exec --app visionary-dls /containers/stable/latest waf install --test-execall
```

## Usage

Typically you would use the shared object in another project.
See `tests/hw/hxcomm/test-jtag_loopback.cpp` for an example.
Doxygen-generated code documentation is deployed [here](https://jenkins.bioai.eu/job/bld_nightly-hxcomm/Documentation_20_28hxcomm_29/) nightly by Jenkins.
For usage instructions related to the C++/SystemVerilog-DPI interface please look at the flange project.

## Contributing

In case you encounter bugs, please [file a work package](https://openproject.bioai.eu/projects/hxcomm/work_packages/) describing all steps required to reproduce the problem.
Don't just post in the chat, as the probability to get lost is very high.

Before committing any changes, make sure to run `git-clang-format` and add the resulting changes to your commit.

## How to add a new instruction

### Creating the new instruction

A instruction consists of a payload size in bits and a typesafe `Payload` type, see `include/hxcomm/vx/instruction/*.h` for implemented instructions.

```cpp
struct MyNewInstruction
{
    constexpr size_t size = N;
    struct Payload;
};
```

The `Payload` type is to implement an abstraction for fields in the payload, e.g. bools, ranged numbers or bitsets.
For uniform payloads, i.e. bitsets, numbers and ranged numbers, templates for direct use exist in `include/hxcomm/common/payload.h`.
In addition, every `Payload` type has to provide an `encode` and a `decode` function encoding or decoding the payload fields from or to a bitset of size `size` with the following signature:
```cpp
struct MyNewInstruction::Payload
{
    template <typename WordType>
    hate::bitset<size, WordType> encode() const;

    template <typename WordType>
    void decode(hate::bitset<size, WordType> const& data);
};
```
Performing `encode` and `decode` after one another has to result in an identity operation.

If this new instruction is to reside in a subdictionary, put it in a new namespace:
```cpp
namespace hxcomm::vx::instruction::my_new_instruction_namespace
```
and therein define a dictionary:
```cpp
typedef hate::type_list<MyNewInstruction> Dictionary;
```

Now add the instruction at the right location in the appropriate higher-level dictionary.
This will most probably be either the `ToFPGADictionary` or the `FromFPGADictionary` in `include/hxcomm/vx/instruction/instruction.h`.
However, dictionary stacking/grouping with more than one level is possible, so subgroups can be defined.

### Testing the new instruction

* For every instruction, the identity operation by encoding and decoding is to be tested individually.
  See `tests/sw/hxcomm/test-omnibus_to_fpga.cpp` for an examplary test implementation.
  - For creation of typical payload field values in a random manner, `tests/sw/hxcomm/test-helper.h` provides convenience functions.
* By adding the new instruction to a dictionary on which one of the toplevel dictionaries `ToFPGADictionary` and `FromFPGADictionary` depend, corresponding `UTMessage` types are automatically tested for serialization possibility, which comes for free, and `UTMessage` `encoding` and `decoding`.

Having done the above steps, the new instruction is available in `UTMessage{To/From}FPGAVariant` in `include/hxcomm/vx/utmessage.h` and can now be directly used in hardware tests, residing under `tests/hw/hxcomm/test-*.cpp` or in the `fisch` software layer.
