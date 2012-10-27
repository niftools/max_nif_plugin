#!/bin/bash

# quick script to create zip file containing all sources
# uses https://github.com/amorilia/git-archive-all

VERSION="3.6.0"
wcrev=`git log -1 --pretty=format:%h`
if [ "$1" == "" ]
then
    extversion=${VERSION}.${wcrev}
else
    extversion=${VERSION}-$1.${wcrev}
fi

git clean -xfd
./makeconfig.sh
git-archive-all max_nif_plugin-${extversion}.zip --prefix max_nif_plugin/ -v --extra NifCommon/config.h

