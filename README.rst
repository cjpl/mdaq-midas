README
======

Checkout the sources::

   $ git clone git://github.com/cjpl/mdaq-midas.git

Debian Packages
---------------

Directories contain utilities to build debian packages. Please follow steps
below.

Package: mdaq-{root,noroot,doc}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You need the package `root-system` first to build the package. The `root-system`
debian packages can be installed by reference here::

   http://lcg-heppkg.web.cern.ch/lcg-heppkg/debian/

1. get sources from MIDAS subversion repository::

   $ cd mdaq-midas/midas && ./autoconf/prepare-src.sh

2. prepare the "*orig.tar.gz"::

   $ ./autoconf/prepare-orig.sh

3. generate debian packages::

   $ dpkg-buildpackage

Package: mdaq-roody
~~~~~~~~~~~~~~~~~~~

This package contains sources from `ROODY` and `rootana`::

1. http://daq-plone.triumf.ca/SR/ROODY

2. http://daq-plone.triumf.ca/SR/rootana

You may refer to DAQ-Triumf_ for more related codes.

[FIXME]!

.. DAQ-Triumf_: http://daq-plone.triumf.ca/SR

Package: mdaq-rome
~~~~~~~~~~~~~~~~~~

URL: http://midas.psi.ch/rome/

[FIXME]!

Package: mdaq-elog
~~~~~~~~~~~~~~~~~~

URL: http://midas.psi.ch/elog/

[FIXME]!

