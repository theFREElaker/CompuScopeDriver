#
# autotools and libtool initialization
AC_PREREQ(2.61)
AC_INIT([Gage Driver], [5.04.36], [linux@gage-applied.com])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
AM_SILENT_RULES([yes])
AM_PROG_AR
LT_INIT([dlopen disable-static])
AC_CONFIG_HEADER([config.h])
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_CC
#
# perform checks
#AC_HEADER_STDC
#AC_HEADER_STDBOOL
#AC_CHECK_HEADERS([fcntl.h inttypes.h stdint.h stdlib.h string.h sys/ioctl.h sys/time.h sys/timeb.h unistd.h])
#AC_C_CONST
#AC_C_INLINE
#AC_TYPE_INT16_T
#AC_TYPE_INT32_T
#AC_TYPE_INT64_T
#AC_TYPE_INT8_T
#AC_TYPE_SIZE_T
#AC_TYPE_UINT16_T
#AC_TYPE_UINT32_T
#AC_TYPE_UINT64_T
#AC_TYPE_UINT8_T
#
# instruct ac to generate Makefile (from Makefile.am)
AC_CONFIG_FILES([Makefile
                 Middle/CsDisp/Makefile
                 Middle/CsFs/Makefile
                 Middle/CsRmDLL/Makefile
                 Middle/CsRm/Makefile
                 Middle/CsSsm/Makefile
                 Sdk/CsAppSupport/Makefile
                 Boards/UserSpace/CsRabbit/Makefile
                 Boards/UserSpace/CsSpider/Makefile
                 Boards/UserSpace/CsSplenda/Makefile
                 Boards/UserSpace/CsDecade12/Makefile
                 Boards/UserSpace/CsHexagon/Makefile
                 Boards/UserSpace/CsJohnDeere/Makefile])
#
# replace all occurrence in Makefile.am with the include path found by configure
AC_SUBST(all_includes)
AC_OUTPUT
