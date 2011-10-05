SIS1100/310X Linux Driver Packages
==================================

The driver code is imported from sis1100_ driver page.

.. _sis1100: http://www.struck.de/linux1100.htm

Current version: 2.13-5

Supported Linux kernel: up to 2.6.37

The debian package source is slightly modified from `dkms mkdsc` results:

  1. sudo dkms add -m sis1100 -v 2.13
  2. sudo dkms build -m sis1100 -v 2.13
  3. sudo dkms install -m sis1100 -v 2.13
  4. sudo modprobe sis1100
  5. sudo dkms mkdsc -m sis1100 -v 2.13 --source-only


