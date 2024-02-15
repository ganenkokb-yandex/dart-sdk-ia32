#!/usr/bin/env python3

import os
import re
import sys

regexp_tests = re.compile("^FAILED\:.*")
arch = "ia32"

if sys.argv.count("help") or sys.argv.count("--help"):
    print("Usage: " + sys.argv[0] + " [x64]")
    print("    x64 - run tests for x64.")
    exit(0)

if sys.argv.count("x64") > 0:
    arch = "x64"

test_results_file = "./dart_tests_results_" + arch +".txt"

# Run tests with redirected output to the file.
print("Run tests (" + arch + "). Output: " + test_results_file)
#
# dartk:                Compile the Dart code into Kernel before running test.
#
run_tests_command = "python3 ./tools/test.py --mode release --arch " + arch + " --runtime vm --compiler dartk -v --build vm/cc > " + test_results_file
print(run_tests_command)
os.system(run_tests_command)

#
# dartkp:               Compile the Dart code into Kernel and then Kernel into
#                       AOT snapshot before running the test.
#
# Uncomment to run dartkp tests:
run_tests_command="python3 ./tools/test.py --mode release --arch " + arch + " --compiler dartkp -v --build vm --use-elf >> " + test_results_file
print(run_tests_command)
os.system(run_tests_command)

# Read tests results.
lines = []
with open(test_results_file) as file:
    lines = [line.rstrip() for line in file]

# Parse tests results and print failed tests with context.
failed_tests = []
delimiter = "============================================"
print(delimiter)
for i in range(len(lines)):
    line = lines[i]
    if re.search(regexp_tests, line):
        failed_tests.append(line)
        # Print context.
        for j in range(len(lines) - i):
            if lines[i + j] == "--- Re-run this test:":
                print(delimiter)
                break;
            print(lines[i+j])

# Check for failed tests.
if not len(failed_tests):
    print("All tests passed")
    exit(0)

print("--- List of failed tests:")
for test in set(failed_tests):
    print(test)

print("=== Failed tests: ", len(failed_tests))

exit(-1)
