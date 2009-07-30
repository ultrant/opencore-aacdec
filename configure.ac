dnl Require autoconf version >= 2.57
AC_PREREQ(2.57)


dnl ############# Initialization

AC_INIT([libaacdec], [1.0.0], [tipok@tipok.org.ua])

dnl Version 1.7 of automake is recommended
AM_INIT_AUTOMAKE(libaacdec, 1.0.0)
AM_CONFIG_HEADER(config.h)


dnl ############# Compiler and tools Checks

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_C_INLINE
AC_STDC_HEADERS
AM_PROG_LIBTOOL


AC_DEFUN([APR_HELP_STRING],[ifelse(regexp(AC_ACVERSION, 2\.1), -1, 
AC_HELP_STRING([$1],[$2]),[  ][$1] substr([                       
],len($1))[$2])])


dnl ############## Arguments Checks

AC_MSG_CHECKING([enabled SBR decoding])
AC_ARG_WITH([aac_plus], [APR_HELP_STRING([--with-aac-plus], [enable SBR decoding [[default=yes]]])],
[
    if test "$with_aac_plus" != "no"; then
        AC_MSG_RESULT([yes])
    else
        AC_MSG_RESULT([no])
    fi
], [AC_MSG_RESULT([yes])])



if test "$with_aac_plus" != "no"; then
    AC_DEFINE([AAC_PLUS], 1, [SBR decoding])
    AC_MSG_CHECKING([enabled High-Quality SBR algorithms])
    AC_ARG_WITH([hq_sbr], [APR_HELP_STRING([--with-hq-sbr], [enable High-Quality SBR algorithms [[default=yes]]])],
    [
        if test "$with_hq_sbr" == "no"; then
            AC_MSG_RESULT([no])
        else
            AC_MSG_RESULT([yes])
        fi
    ], [AC_MSG_RESULT([yes])])

    if test "$with_hq_sbr" != "no"; then
        AC_DEFINE([HQ_SBR], 1, [High-Quality SBR])
    fi

    AC_MSG_CHECKING([enabled Parametric-Stereo decoding])
    AC_ARG_WITH([ps], [APR_HELP_STRING([--with-ps], [enable Parametric-Stereo decoding [[default=yes]]])],
    [
        if test "$with_ps" == "no"; then
            AC_MSG_RESULT([no])
        else
            AC_DEFINE([PARAMETRICSTEREO], 1, [Parametric-Stereo decoding])
            AC_MSG_RESULT([yes])
        fi
    ], [AC_MSG_RESULT([yes])])

    if test "$with_ps" != "no"; then
        AC_DEFINE([PARAMETRICSTEREO], 1, [Parametric-Stereo decoding])
    fi

fi



#
# check target architecture
#
have_x86=no
have_arm=no

with_arm_v4=no
with_arm_v5=no
with_arm_msc_evc_v4=no
with_arm_msc_evc_v5=no
with_arm_gcc_v4=no
with_arm_gcc_v5=no

case x"$target" in
    xNONE | x)
        target_or_host="$host" ;;
    *)
        target_or_host="$target" ;;
esac

case "$target_or_host" in
    i*86-*-*)
        have_x86=yes ;;
    *armv5te*)
        have_arm=yes
        with_arm_gcc_v5=yes ;;
    *armv4te*)
        have_arm=yes
        with_arm_gcc_v4=yes ;;
    *arm*)
        have_arm=yes ;;
    ppc-*-linux* | powerpc-*)
        have_ppc=yes ;;
    *)
        ;;
esac


if test x$have_arm = xyes; then

dnl ############## ASM Optimisations

# EXTENDED_ASM

dnl ############## Simple ARM
AC_MSG_CHECKING([enabled ARM v4])
AC_ARG_WITH([arm_v4], [APR_HELP_STRING([--with-arm-v4], [ARM v4 [[default=no]]])],
[
    if test "$with_arm_v4" == "no"; then
        AC_MSG_RESULT([no])
    else
        AC_DEFINE([PV_ARM_V4], 1, [ARM v4])
        AC_MSG_RESULT([yes])
    fi
], [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([enabled ARM v5])
AC_ARG_WITH([arm_v5], [APR_HELP_STRING([--with-arm-v5], [ARM v5 [[default=no]]])],
[
    if test "$with_arm_v5" == "no"; then
        AC_MSG_RESULT([no])
    else
        AC_DEFINE([PV_ARM_V5], 1, [ARM v5])
        AC_MSG_RESULT([yes])
    fi
], [AC_MSG_RESULT([no])])

dnl ############## MSC EVC ARM
AC_MSG_CHECKING([enabled MSC EVC ARM v4])
AC_ARG_WITH([arm_msc_evc_v4], [APR_HELP_STRING([--with-arm-msc-evc-v4], [MSC EVC ARM v4 [[default=no]]])],
[
    if test "$with_arm_msc_evc_v4" == "no"; then
        AC_MSG_RESULT([no])
    else
        AC_DEFINE([PV_ARM_MSC_EVC_V4], 1, [MSC EVC ARM v4])
        AC_MSG_RESULT([yes])
    fi
], [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([enabled MSC EVC ARM v5])
AC_ARG_WITH([arm_msc_evc_v5], [APR_HELP_STRING([--with-arm-msc-evc-v5], [MSC EVC ARM v5 [[default=no]]])],
[
    if test "$with_arm_msc_evc_v5" == "no"; then
        AC_MSG_RESULT([no])
    else
        AC_DEFINE([PV_ARM_MSC_EVC_V5], 1, [MSC EVC ARM v5])
        AC_MSG_RESULT([yes])
    fi
], [AC_MSG_RESULT([no])])

dnl ############## GCC ARM
AC_MSG_CHECKING([enabled GCC ARM v4])
AC_ARG_WITH([arm_gcc_v4], [APR_HELP_STRING([--with-arm-gcc-v4], [GCC ARM v4 [[default=auto]]])],
[
    if test "$with_arm_gcc_v4" == "no"; then
        AC_MSG_RESULT([no])
    else
        AC_DEFINE([PV_ARM_GCC_V4], 1, [GCC ARM v4])
        AC_MSG_RESULT([yes])
    fi
], [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([enabled GCC ARM v5])
AC_ARG_WITH([arm_gcc_v5], [APR_HELP_STRING([--with-arm-gcc-v5], [GCC ARM v5 [[default=auto]]])],
[
    if test "$with_arm_gcc_v5" == "no"; then
        AC_MSG_RESULT([no])
    else
        AC_DEFINE([PV_ARM_GCC_V5], 1, [GCC ARM v5])
        CFLAGS="$CFLAGS -march=armv5te"
        AC_MSG_RESULT([yes])
    fi
], [AC_MSG_RESULT([no])])
fi


AC_MSG_CHECKING([enabled Network player])
AC_ARG_WITH([netplayer], [APR_HELP_STRING([--with-netplayer], [enable Network player [[default=yes]]])],
[
    if test "$with_netplayer" == "no"; then
		AC_MSG_RESULT([no])
	else
		# Check for libshout
		AC_MSG_RESULT([yes])
	fi
], [AC_MSG_RESULT([yes])])

dnl ############## Header Checks
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

if test "$with_netplayer" != "no"; then
	AC_CHECK_HEADERS([curl/curl.h pthread.h])
	PKG_CHECK_MODULES([CURL], [libcurl >= 7.16])
	NETPLAYER=aacNetPlayer
	AC_SUBST(NETPLAYER)
fi

dnl ############## Final Output
CFLAGS="$CFLAGS -Wall"

AC_OUTPUT([ Makefile src/Makefile include/Makefile sample/Makefile oscl/Makefile ])
