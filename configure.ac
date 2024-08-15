AC_INIT([otairedis],[1.0])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([])
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([subdir-objects foreign serial-tests])
AC_LANG_C
AC_LANG([C++])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_HEADER_STDC

AC_ARG_ENABLE(debug,
[  --enable-debug          turn on debugging],
[case "${enableval}" in
	yes) debug=true ;;
	no)  debug=false ;;
	*) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
esac],[debug=false])
AM_CONDITIONAL(DEBUG, test x$debug = xtrue)

AC_ARG_WITH(otai,
[  --with-otai=vs           compiling against specific OTAI],
[case "${withval}" in
	vs)   otai=vs ;;
	*) AC_MSG_ERROR(bad value ${withval} for --with-otai) ;;
esac])
AM_CONDITIONAL(OTAIVS,   test x$otai = xvs)

AC_ARG_WITH(swss-common-inc,
[  --with-swss-common-inc=DIR
                           prefix where swss-common includes are installed],
[AC_SUBST(CPPFLAGS, "$CPPFLAGS -I${withval}")
 AC_SUBST(SWSS_COMMON_INC_DIR, "${withval}")])

AC_ARG_WITH(swss-common-lib,
[  --with-swss-common-lib=DIR
                           prefix where swss-common libraries are installed],
[AC_SUBST(LDFLAGS, "$LDFLAGS -L${withval}")
 AC_SUBST(SWSS_COMMON_LIB_DIR, "${withval}")])

AC_ARG_ENABLE(syncd,
[  --enable-syncd          build syncd],
[case "${enableval}" in
   yes) syncd=true ;;
   no)  syncd=false ;;
   *) AC_MSG_ERROR(bad value ${enableval} for --enable-syncd) ;;
esac],[syncd=true])
AM_CONDITIONAL(SYNCD, test x$syncd = xtrue)
AM_COND_IF([SYNCD], [
        AM_COND_IF([OTAIVS], [], [AC_CHECK_LIB([otai], [main], [AC_MSG_NOTICE(libotai found)], [AC_MSG_ERROR(libotai is required for syncd)])
])])

AC_ARG_ENABLE(redis-test,
[  --enable-redis-test     test with redis service],
[case "${enableval}" in
	yes) rtest=true ;;
	no)  rtest=false ;;
	*) AC_MSG_ERROR(bad value ${enableval} for --enable-redis-test) ;;
esac],[rtest=false])
AM_CONDITIONAL(RTEST, test x$rtest = xtrue)

if test x$CONFIGURED_ARCH = xarmhf && test x$CROSS_BUILD_ENVIRON = xy; then
	AM_CONDITIONAL(ARCH64, false)
else
	AM_CONDITIONAL(ARCH64, test `getconf LONG_BIT` = "64")
fi

AC_PATH_PROGS(SWIG, [swig3.0 swig])

CXXFLAGS_COMMON=""
CXXFLAGS_COMMON+=" -ansi"
CXXFLAGS_COMMON+=" -fPIC"
CXXFLAGS_COMMON+=" -pipe"
CXXFLAGS_COMMON+=" -std=c++14"
CXXFLAGS_COMMON+=" -Wall"
CXXFLAGS_COMMON+=" -Wcast-align"
CXXFLAGS_COMMON+=" -Wcast-qual"
CXXFLAGS_COMMON+=" -Wconversion"
CXXFLAGS_COMMON+=" -Wdisabled-optimization"
CXXFLAGS_COMMON+=" -Werror"
CXXFLAGS_COMMON+=" -Wextra"
CXXFLAGS_COMMON+=" -Wfloat-equal"
CXXFLAGS_COMMON+=" -Wformat=2"
CXXFLAGS_COMMON+=" -Wformat-nonliteral"
CXXFLAGS_COMMON+=" -Wformat-security"
CXXFLAGS_COMMON+=" -Wformat-y2k"
CXXFLAGS_COMMON+=" -Wimport"
CXXFLAGS_COMMON+=" -Winit-self"
CXXFLAGS_COMMON+=" -Wno-inline"
CXXFLAGS_COMMON+=" -Winvalid-pch"
CXXFLAGS_COMMON+=" -Wmissing-field-initializers"
CXXFLAGS_COMMON+=" -Wmissing-format-attribute"
CXXFLAGS_COMMON+=" -Wmissing-include-dirs"
CXXFLAGS_COMMON+=" -Wmissing-noreturn"
CXXFLAGS_COMMON+=" -Wno-aggregate-return"
CXXFLAGS_COMMON+=" -Wno-padded"
CXXFLAGS_COMMON+=" -Wno-switch-enum"
CXXFLAGS_COMMON+=" -Wno-unused-parameter"
CXXFLAGS_COMMON+=" -Wpacked"
CXXFLAGS_COMMON+=" -Wpointer-arith"
CXXFLAGS_COMMON+=" -Wredundant-decls"
CXXFLAGS_COMMON+=" -Wshadow"
CXXFLAGS_COMMON+=" -Wstack-protector"
CXXFLAGS_COMMON+=" -Wstrict-aliasing=3"
CXXFLAGS_COMMON+=" -Wswitch"
CXXFLAGS_COMMON+=" -Wswitch-default"
CXXFLAGS_COMMON+=" -Wunreachable-code"
CXXFLAGS_COMMON+=" -Wunused"
CXXFLAGS_COMMON+=" -Wvariadic-macros"
CXXFLAGS_COMMON+=" -Wwrite-strings"
CXXFLAGS_COMMON+=" -Wno-switch-default"
CXXFLAGS_COMMON+=" -Wconversion"
CXXFLAGS_COMMON+=" -Wno-psabi"

SAVED_FLAGS="$CXXFLAGS"
CXXFLAGS="-Wcast-align=strict"
AC_MSG_CHECKING([whether CXX supports -Wcast-align=strict])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
    [AC_MSG_RESULT([yes])]
    [AC_SUBST([CXXFLAGS_COMMON], ["${CXXFLAGS_COMMON} -Wcast-align=strict"])],
    [AC_MSG_RESULT([no])])
CXXFLAGS="$SAVED_FLAGS"

AC_SUBST(NO_CAST_FUNCTION_TYPE)

SAVED_FLAGS="$CXXFLAGS"
CXXFLAGS="-Wno-cast-function-type"
AC_MSG_CHECKING([whether CXX supports -Wno-cast-function-type])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
    [AC_MSG_RESULT([yes])]
    [AC_SUBST([NO_CAST_FUNCTION_TYPE], ["-Wno-cast-function-type"])],
    [AC_MSG_RESULT([no])])
CXXFLAGS="$SAVED_FLAGS"

AC_SUBST(CXXFLAGS_COMMON)
AC_SUBST(OTAIINC, "-I\$(top_srcdir)/OTAI/inc -I\$(top_srcdir)/OTAI/meta")

AC_OUTPUT(Makefile
          meta/Makefile
	      lib/Makefile
          vslib/Makefile
	      syncd/Makefile)
