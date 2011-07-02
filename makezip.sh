#!/bin/bash

# quick script to create zip file containing all sources
# uses https://github.com/amorilia/git-archive-all
git clean -xfd
git-archive-all max_nif_plugin-`git log -1 --pretty=format:%h`.zip --prefix max_nif_plugin/ -v
