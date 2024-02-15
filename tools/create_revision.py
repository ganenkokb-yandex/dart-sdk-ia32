#!/usr/bin/env python3
# Copyright 2023 Yandex LLC. All rights reserved.

import os
import subprocess
import sys


SRC_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
FLUTTER_DEPS_FILE = os.path.join(
    SRC_DIR, 'flutter', 'DEPS',
)


def Main(argv):
    gclient_executable = 'gclient.bat' if sys.platform == 'win32' else 'gclient'
    env = dict(os.environ, DEPOT_TOOLS_UPDATE='0')

    dart_revision = subprocess.check_output([
        gclient_executable,
        'getdep',
        '--deps-file', FLUTTER_DEPS_FILE,
        '-r', 'src/third_party/dart',
    ], env=env).strip()

    with open(argv[1], 'wb') as f:
        f.write(dart_revision)
    return 0


if __name__ == '__main__':
    sys.exit(Main(sys.argv))
