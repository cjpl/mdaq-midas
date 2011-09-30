README
======

How to build debian packages? Please follow the steps below::

1. checkout the sources::

   $ git clone git://github.com/cjpl/mdaq-midas.git

2. get sources from MIDAS subversion repository::

   $ cd mdaq-midas && ./autoconf/prepare-src.sh

3. prepare the "*orig.tar.gz"::

   $ ./autoconf/prepare-orig.sh

4. generate debian packages::

   $ dpkg-buildpackage

