#!/bin/bash

MDAQ_WDIR=$(dirname $(readlink -f $0))/../
ORIG_FILE=${MDAQ_WDIR}/../mdaq_2.3.0+r5218.orig.tar.gz
ORIG_PREFIX=mdaq-2.3.0+r5218

if [[ -f ${ORIG_FILE} ]]; then
    echo "Orignal file is ready. No further action needed!"
    exit 0
fi

if [[ ! -d ${MDAQ_WDIR}/mxml || ! -d ${MDAQ_WDIR}/midas || \
      ! -f ${MDAQ_WDIR}/midas/doc/midasdoc-images.tar.gz || \
      ! -f ${MDAQ_WDIR}/midas/doc/logo.png  ]]; then
    echo "Orignal files not ready. Getting them from internet!"
    ${MDAQ_WDIR}/autoconf/prepare-src.sh
fi

OLD_DIR=`pwd`
cd ${MDAQ_WDIR}
tar --transform "s,^,${ORIG_PREFIX}/,S" -czvf ${ORIG_FILE} \
    midas/ mxml/ addons/ autoconf/

cd ${OLD_DIR}

