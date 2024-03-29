#!/bin/bash

MIDAS_SVN="svn+ssh://svn@savannah.psi.ch/afs/psi.ch/project/meg/svn/midas/trunk"
MIDAS_REV=5254
MXML_SVN="svn+ssh://svn@savannah.psi.ch/afs/psi.ch/project/meg/svn/mxml/trunk"
MXML_REV=73
MIDASDOC_URL="http://ladd00.triumf.ca/~daqweb/ftp/midasdoc-images.tar.gz"

OLD_DIR=`pwd`
MDAQ_WDIR=$(dirname $(readlink -f $0))/../
cd ${MDAQ_WDIR}

# Checkout mxml
if [[ ! -d mxml ]]; then
    echo "## Checkout mxml source from public subversion repository ..."
    svn checkout ${MXML_SVN} -r ${MXML_REV} mxml
else
    echo "## The mxml directory already exists."
    echo "## Please make sure it is what you need!"
    echo
fi

# Checkout midas
if [[ ! -d midas ]]; then
    echo "## Checkout midas source from public subversion repository ..."
    svn checkout ${MIDAS_SVN} -r ${MIDAS_REV} midas
else
    echo "## The midas directory already exists."
    echo "## Please make sure it is what you need!"
    echo
fi

# Downloading midasdoc.tar.gz
if [[ -f midas/doc/midasdoc-images.tar.gz ]]; then
    echo "## midasdoc-images.tar.gz already exists."
    echo
else
    echo "## Downloading midasdoc-images.tar.gz ..."
    wget -c ${MIDASDOC_URL}
    mv midasdoc-images.tar.gz midas/doc/
    echo
fi

if [[ ! -f midas/doc/logo.png ]]; then
    wget -c http://midas.psi.ch/img/midas_logo.jpg
    convert midas_logo.jpg  png:midas/doc/logo.png
    rm midas_logo.jpg
fi

####
cd ${OLD_DIR}

