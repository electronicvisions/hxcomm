import unittest
import inspect
import tempfile
import re
import pyhxcomm
import random


# This is on module level as many unittest frameworks will ignore main
random.seed(1234)


def get_all_classes(m):
    ret = dict()
    for name, obj in inspect.getmembers(m):
        if (inspect.isclass(obj) and
                str(obj.__class__) == "<class 'pybind11_type'>"):
            ret[name] = obj
            get_all_classes(obj)
    return ret


pyhxcomm_classes = get_all_classes(pyhxcomm)

ignored_classes = [pyhxcomm.ARQConnection, pyhxcomm.SimConnection]

class PyHxcommBindings(unittest.TestCase):
    def instantiate_with_guessed_constructor(self, obj):
        """
        Instantiate an object of class `obj`. If it has no default
        constructor, try to guess meaningful arguments.
        """
        try:
            return obj()
        except TypeError as error:
            # Extract the missing arguments
            arg_strs = self.parse_constructor_exception(error.message)

            # Try to construct them and instantiate the test object
            arguments = [eval(arg_type)() for arg_type in arg_strs]
            return obj(*arguments)

    @staticmethod
    def parse_constructor_exception(error_string):
        """
        Parse an "Incompatible Constructor" exception and extract the
        correct argument types.

        Such an exception might look like this:

        ```
        TypeError: __init__(): incompatible constructor arguments.
        The following argument types are supported:

            1. _pydls.Trigger_mode(arg0: int)
        ```

        :param error_string: Exception message
        :type error_string: str
        :return: List of the expected argument types
        :rtype: list of str
        """
        regex = r"(?<=arg\d: ).*?(?=\,|\))"
        return re.findall(regex, error_string)


def construct_base_checks():
    for name, obj in pyhxcomm_classes.items():
        def generate_test_function(test_obj):
            """
            Generate an "is-constructible" test for a given class.
            """
            @unittest.skipIf(obj in ignored_classes, "Ignored")
            def test_function(self):
                original = self.instantiate_with_guessed_constructor(test_obj)
            return test_function

        func = generate_test_function(obj)
        func.__name__ = 'test_base_constructible_%s' % name
        setattr(PyHxcommBindings, func.__name__, func)


def construct_derived_checks():
    for name, obj in pyhxcomm_classes.items():
        def generate_test_function(test_obj):
            """
            Generate an "is-constructible" test for an arbitrary derived class of `test_obj`.
            """

            @unittest.skipIf(obj in ignored_classes, "Ignored")
            def test_function(self):
                class Derived(test_obj):
                    def __init__(self, *args, **kwargs):
                        super(Derived, self).__init__(*args, **kwargs)

                        # Add new attribute, ensure it hasn't been there before
                        new_name = "x%d" % random.randint(0, int(2**32-1))
                        while hasattr(self, new_name):
                            new_name = "x%d" % random.randint(0, int(2**32-1))
                        new_value = 0xdeadbeef
                        setattr(self, new_name, new_value)

                Derived.__name__ = "derived_%s" % name

                # pickle needs it in global namespace
                globals()[Derived.__name__] = Derived

                # Test it
                original = self.instantiate_with_guessed_constructor(Derived)
            return test_function

        func = generate_test_function(obj)
        func.__name__ = 'test_derived_constructible_%s' % name
        setattr(PyHxcommBindings, func.__name__, func)


# Register tests: has to be done on module level and must not contain "test"
construct_base_checks()
construct_derived_checks()


if __name__ == '__main__':
    unittest.main()
