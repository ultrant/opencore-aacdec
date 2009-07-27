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
with_aac_plus="yes"
with_hq_sbr="yes"
with_ps="yes"

AC_MSG_CHECKING([enabled SBR decoding])
AC_ARG_WITH([aac_plus], [APR_HELP_STRING([--with-aac-plus], [enable SBR decoding [[default=yes]]])],
[
    if test "$with_aac_plus" = "no"; then
        AC_MSG_RESULT([no])
    else
		AC_DEFINE([AAC_PLUS], 1, [SBR decoding])
		AC_MSG_RESULT([yes])
		
		AC_MSG_CHECKING([enabled High-Quality SBR algorithms])
		AC_ARG_WITH([hq_sbr], [APR_HELP_STRING([--with-hq-sbr], [enable High-Quality SBR algorithms [[default=yes]]])],
		[
		    if test "$with_hq_sbr" = "no"; then
			AC_MSG_RESULT([no])
		    else
			AC_DEFINE([HQ_SBR], 1, [High-Quality SBR])
			AC_MSG_RESULT([yes])
		    fi
		], [AC_MSG_RESULT([yes])])
		
		
		AC_MSG_CHECKING([enabled Parametric-Stereo decoding])
		AC_ARG_WITH([ps], [APR_HELP_STRING([--with-ps], [enable Parametric-Stereo decoding [[default=yes]]])],
		[
		    if test "$with_ps" = "no"; then
			AC_MSG_RESULT([no])
		    else
			AC_DEFINE([PARAMETRICSTEREO], 1, [Parametric-Stereo decoding])
			AC_MSG_RESULT([yes])
		    fi
		], [AC_MSG_RESULT([yes])])
		
		
    fi
], [AC_MSG_RESULT([yes])])





# PV_ARM_V5
# PV_ARM_V4
# PV_ARM_MSC_EVC_V4
# PV_ARM_MSC_EVC_V5
# PV_ARM_GCC_V5
# PV_ARM_GCC_V4


dnl ############## Header Checks
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h pthread.h])


dnl ############## Final Output
CFLAGS="$CFLAGS -Wall"

AC_OUTPUT([ Makefile src/Makefile include/Makefile sample/Makefile oscl/Makefile ])
