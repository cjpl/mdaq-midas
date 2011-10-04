#!/bin/bash

ROODY_SVN="svn://ladd00.triumf.ca/roody/trunk"
ROODY_REV=236
ROOTANA_SVN="svn://ladd00.triumf.ca/rootana/trunk"
ROOTANA_REV=79

OLD_DIR=`pwd`
MDAQ_WDIR=$(dirname $(readlink -f $0))/../
cd ${MDAQ_WDIR}

# Checkout ROODY
if [[ ! -d roody ]]; then
    echo "## Checkout ROODY source from public subversion repository ..."
    svn checkout ${ROODY_SVN} -r ${ROODY_REV} roody
else
    echo "## The roody directory already exists."
    echo "## Please make sure it is what you need!"
    echo
fi

# Checkout ROOTANA
if [[ ! -d rootana ]]; then
    echo "## Checkout ROOTANA source from public subversion repository ..."
    svn checkout ${ROOTANA_SVN} -r ${ROOTANA_REV} rootana
else
    echo "## The rootana directory already exists."
    echo "## Please make sure it is what you need!"
    echo
fi

####
cd ${OLD_DIR}

