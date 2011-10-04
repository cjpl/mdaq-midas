#!/bin/bash

MDAQ_WDIR=$(dirname $(readlink -f $0))/../
ORIG_FILE=${MDAQ_WDIR}/../mdaq-roody-1.1.0+r236.orig.tar.gz
ORIG_PREFIX=mdaq-roody-1.1.0+r236

if [[ -f ${ORIG_FILE} ]]; then
    echo "Orignal file is ready. No further action needed!"
    exit 0
fi

if [[ ! -d ${MDAQ_WDIR}/roody || ! -d ${MDAQ_WDIR}/rootana ]]; then
    echo "Orignal files not ready. Getting them from internet!"
    ${MDAQ_WDIR}/debian/prepare-src.sh
fi

OLD_DIR=`pwd`
cd ${MDAQ_WDIR}
tar --transform "s,^,${ORIG_PREFIX}/,S" -czvf ${ORIG_FILE} \
    roody/ rootana/ addons/main_el.cxx Makefile

cd ${OLD_DIR}

