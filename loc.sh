#!/bin/bash

cloc ./ --exclude-dir=.cache,.git,.vscode,.idea,build,external,vcpkg_installed --exclude-list-file=compile_commands.json
