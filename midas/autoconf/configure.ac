### Autoconf support for midas
# June 17, 2011 -- created by Exaos Lee <exaos.lee(at)gmail.com>
###########################################################
# Prelude
AC_INIT([mdaq], [2.3.0+r5254], [exaos.lee@gmail.com])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE(mdaq, 2.3)

############################################################
# Checking sources
AC_CONFIG_SRCDIR(../midas/src)
AC_CONFIG_SRCDIR(../midas/include)
AC_CONFIG_SRCDIR(../midas/utils)
AC_CONFIG_SRCDIR(../mxml)
#AC_CONFIG_MACRO_DIR([m4])

# Directories
AC_PREFIX_DEFAULT([`pwd`]) # ([/usr/local])
test "$prefix" = NONE && prefix=$ac_default_prefix
test "$exec_prefix" = NONE && exec_prefix=$prefix
test "$bindir" = '${exec_prefix}/bin' && bindir="$exec_prefix/bin"
test "$libdir" = '${exec_prefix}/lib' && libdir="$exec_prefix/lib"
test "$includedir" = '${prefix}/include' && includedir="$prefix/include"
test "$datarootdir" = '${prefix}/share' && datarootdir="$prefix/share"
AC_SUBST(datadir)
test "$datadir" = '${datarootdir}' && datadir="${datarootdir}"
AC_SUBST(docdir)
test "$docdir" = '${datarootdir}/doc/${PACKAGE_TARNAME}' && docdir="${datarootdir}/doc/${PACKAGE_TARNAME}"

############################################################
# Checks for compilers
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_CXX_C_O
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_SUBST(mdaq_cflags)
AC_SUBST(mdaq_osflags)
AC_SUBST(mdaq_osprogs)
AC_SUBST(mdaq_libs)
AC_SUBST(platform)

mdaq_osflags=""
mdaq_osprogs=""
need_zlib=""
need_strlcpy=""
platform=""
case "$target" in
     x86_64-*-linux*) # linux x86_64
	echo "Using 64-bit unix defines (for $host_os)"
	need_zlib=1
	mdaq_osflags="-DOS_LINUX -fPIC -Wno-unused-function -D_LARGEFILE64_SOURCE -m64"
	mdaq_osprogs="mlxspeaker dio"
	platform=linux
	;;
     *-*-linux*) # linux
	echo "Using unix defines (for $host_os)"
	need_zlib=1
	mdaq_osflags="-DOS_LINUX -fPIC -Wno-unused-function -D_LARGEFILE64_SOURCE -m32"
	mdaq_osprogs="mlxspeaker dio"
	platform=linux
	;;
     *-*-darwin*) # Mac OS X
	echo "Using MacOS defines (for $host_os)"
	mdaq_osflags="-DOS_LINUX -DOS_DARWIN -DHAVE_STRLCPY -fPIC -Wno-unused-function -DUSE_POSIX_SHM"
	mdaq_osprogs="mlxspeaker"
	need_zlib=1
	;;
     *-*-cygwin*) # Windows cygwin?
        mdaq_osflags="-DOS_LINUX -DOS_CYGWIN -Wno-unused-function"
	;;
     *-*-mingw*) # Windows MingW
	;;
     *)
	echo "Unknown"
	;;
esac
mdaq_cflags="-Wall -Wuninitialized -DHAVE_FTPLIB ${mdaq_osflags}"
AC_MSG_NOTICE([MDAQ_CFLAGS is set to "${mdaq_cflags}"])

#### check headers && libs
# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h limits.h mach/mach.h malloc.h \
  memory.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h  \
  sys/ioctl.h sys/mount.h sys/param.h sys/socket.h sys/statfs.h      \
  sys/statvfs.h sys/time.h sys/timeb.h sys/vfs.h syslog.h termios.h  \
  unistd.h])

## check libraries
AC_CHECK_LIB([pthread], [pthread_kill])
AC_CHECK_LIB([rt], [clock_gettime])
AC_CHECK_LIB([util], [forkpty])
AC_CHECK_LIB([m], [log])
AC_CHECK_LIB([socket], [socket])
AC_CHECK_LIB([crypt], [crypt])

# unixodbc or iodbc?
AC_CHECK_HEADERS([sql.h sqlext.h sqltypes.h])
if test "x$ac_cv_header_sql_h" = x""yes && \
   test "x$ac_cv_header_sqlext_h" = x""yes && \
   test "x$ac_cv_header_sqltypes_h" = x""yes ; then
   AC_CHECK_LIB([odbc],[SQLConnect])
   if test "x$ac_cv_lib_odbc_SQLConnect" = x""yes ; then
      mdaq_cflags="${mdaq_cflags} -DHAVE_ODBC"
      mdaq_osprogs="${mdaq_osprogs} mh2sql"
   fi
fi

############################################################
# check arguments

# enable zlib
AC_ARG_ENABLE(zlib,
  AC_HELP_STRING([--disable-zlib], [enable zlib support]))
AS_IF([test "X$enable_zlib" != "Xno"], [
  AC_CHECK_LIB([z],[gzwrite])
  AC_CHECK_HEADER([zlib.h])
  if test "x$ac_cv_lib_z_gzwrite" = "xyes" && \
     test "x$ac_cv_header_zlib_h" = "xyes" ; then
     mdaq_cflags="${mdaq_cflags} -DHAVE_ZLIB"
     AC_MSG_NOTICE([zlib enabled])
  fi
])

## Check MYSQL
AC_ARG_ENABLE(mysql,
  AC_HELP_STRING([--disable-mysql], [enable mysql support]))
AS_IF([test "X$enable_mysql" != "Xno"], [
  AC_CHECK_PROGS([MYSQLCFG], [mysql_config], [:])
  if test "$MYSQLCFG" = :; then
     AC_MSG_WARN([mysql_config not found!])
  else
     AC_MSG_NOTICE([mysql-config found!])
     MYSQLINCDIR=`mysql_config --include | sed -e 's/-I//g'`
     if test -r ${MYSQLINCDIR}/mysql.h ; then
        NEED_MYSQL=1
	AC_MSG_NOTICE([mysql.h found! build with mysql])
	mdaq_cflags="${mdaq_cflags} -DHAVE_MYSQL `${MYSQLCFG} --include`"
	mdaq_libs="${mdaq_libs} `${MYSQLCFG} --libs`"
     fi
  fi
])

## Check root ("root-config")
AC_SUBST(anaobj_root)
AC_SUBST(root_cfg)
AC_SUBST(root_sys)
AC_SUBST(root_libs)
AC_SUBST(root_glibs)
AC_SUBST(root_cflags)

AC_ARG_ENABLE(root,
  AC_HELP_STRING([--disable-root], [enable ROOT support]))
AC_SUBST(mdaq_enable_root)
mdaq_enable_root="no"
AS_IF([test "X$enable_root" != "Xno"], [
  AC_CHECK_PROGS([root_cfg], [root-config], [:])
  if test "$root_cfg" = :; then
     #AC_MSG_ERROR([This package needs root-config.])
     AC_MSG_WARN([ROOT not installed!])
  else
     root_sys=`${root_cfg} --prefix`
     root_libs=`${root_cfg} --libs`
     root_glibs=`${root_cfg} --glibs`
     root_cflags="-DHAVE_ROOT `${root_cfg} --cflags`"
     AC_MSG_NOTICE([ROOTSYS = $root_sys])
     AC_MSG_NOTICE([ROOT_CFLAGS = ${root_cflags}])
     AC_MSG_NOTICE([ROOT_LIBS = ${root_libs}])
     AC_MSG_NOTICE([ROOT_GLIBS = ${root_glibs}])
     anaobj_root=rmana.o
     mdaq_enable_root="yes"
  fi
])

# enable static ROOT support?
AC_ARG_ENABLE(root-static,
  AC_HELP_STRING([--enable-root-static], [enable static root library support]))
AC_SUBST(mdaq_enable_root_static)
mdaq_enable_root_static="no"
AS_IF([test "X$enable_root_static" = "Xyes"], [
  if test "X$root_sys" = "X"; then
     AC_MSG_ERROR([ROOT not installed!])
  else
     if test ! -f `${root_cfg} --libdir`/libRoot.a ; then
        AC_MSG_ERROR([Static ROOT library not found!])
     fi
     root_libs="`${root_cfg} --libdir`/libRoot.a -lssl -ldl -lcrypt"
     root_glibs="${root_libs} -lfreetype"
     mdaq_enable_root_static="yes"
  fi
])

# enable cernlib: hmana.o
AC_ARG_ENABLE(cernlib,
  AC_HELP_STRING([--disable-cernlib], [open cernlib support]))
AS_IF([test "X$enable_cernlib" != "Xno"], [
  ac_cv_cernlib_config=yes
  AC_SUBST(anaobj_cern)
  anaobj_cern=hmana.o
  AC_MSG_NOTICE([Builing CERNLIB supported analysis object: hmana.o])
])

# need mscb?
AC_ARG_ENABLE(mscb,
  AC_HELP_STRING([--disable-mscb], [build with mscb]))
AS_IF([test "X$enable_mscb" != "Xno"], [
  if test -d ${srcdir}/../midas/mscb ; then
     mdaq_cflags="${mdaq_cflags} -DHAVE_MSCB"
     AC_SUBST(mscb_dir)
     mscb_dir=$srcdir/../midas/mscb
     AC_SUBST(mscb_obj)
     mscb_obj=mscb.o
  else
     AC_MSG_ERROR([Failed to find the mscb source dir!])
  fi
])

# define MIDAS_MAX_EVENT_SIZE
AC_ARG_WITH(max-event-size,
  AC_HELP_STRING([--with-max-event-size=SIZE],
     [Change the default maximum event size (0x400000)]),
  [
  if test x"$withval" != x""; then
     AC_SUBST(max_event_size)
     max_event_size=$withval
     mdaq_cflags="${mdaq_cflags} -DMAX_EVENT_SIZE=${max_event_size}"
     AC_MSG_NOTICE([Set default maximum event size to ${max_event_size}])
  fi
  ]
)

####
AC_MSG_NOTICE([LIBS = $LIBS])
mdaq_libs="$LIBS ${mdaq_libs}"
AC_MSG_NOTICE([MDAQ_LIBS = ${mdaq_libs}])
AC_SUBST(mdaq_config_args)
mdaq_config_args=$ac_cs_config
AC_MSG_NOTICE([mdaq_config_args = ${ac_cs_config}])

############################################################
# Headers
#AC_CONFIG_HEADERS([config.h])

############################################################
# Outputs
AC_CONFIG_FILES([Makefile mdaq-config])
AC_OUTPUT

