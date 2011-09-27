
Required files:
   + configure.ac    -- for autoconf
   + Makefile.am     -- for automake
   + mdaq-config.in  -- for generate mdaq-config

Commands sequence:
   1. libtoolize -c  -- copy 'ltmain.sh' script
   2. aclocal
   3. autoconf
   4. automake --add-missing -c
Step 1 and 2 only need to be executed once. You may repeat step 3 and 4.
Temporary files `aclocal.m4' and `autom4te.cache' can be deleted.

