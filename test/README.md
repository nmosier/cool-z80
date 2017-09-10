# CS433 Testing Infrastructure

For your convenience, your compiler skeleton includes the same test
infrastructure used to evaluate your submissions (but not the tests
themselves). You can use this infrastructure to run multiple test scenarios
and/or implement unit tests.

You can run this tests from within your project with:

```
make test CTEST_OUTPUT_ON_FAILURE=TRUE
```

The latter variable is set to ensure
[CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html) (part of
CMake) displays the messages from the test runner on error.

## Integration Tests

In the `integration` sub-directory you will find test runners and additional
sub-directories for each phase with the respective tests. You should not need
to modify the runners. You may to wish to modify the `CMakeLists.txt` file to
control whether the tests use your implementations for all phases, or just the
phase under test (the default).

Within each phase directory, e.g. `lexer`, you can define a new test by
creating a Cool file ending with `.test`. You then need to define the expected
`stdout` and `stderr` output. The test runner will compare the output of the
program-under-test to these files. For example if your new test is named
"new_test.test", you could create the reference files with (assuming you are in
the `integration/lexer` directory:

```
/path/to/reference/lexer new_test.test > new_test.test.stdout 2> new_test.test.stderr
```

Note that the runner expects the `stdout` and `stderr` files have the same name
as the test file with ".stdout" and ".stderr" appended, respectively (as shown
above).

## Unit Tests

Unit tests are implemented with [Google
Test](https://github.com/google/googletest), refer to the existing tests and
the online documentation for how to implement additional tests.

Note that before you can run the unit tests, you will need to build the unit test executable with

```
make midd-cool-test
```
