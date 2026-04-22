#!/bin/bash

cloc ./ --exclude-dir=.cache,.git,.vscode,.idea,build,vcpkg_installed --exclude-list-file=compile_commands.json
