dnl Based on wojtekka's m4 from http://wloc.wsinf.edu.pl/~kklos/ekg-20080219/m4/readline.m4

AC_DEFUN([AC_CHECK_READLINE],[
  AC_SUBST(READLINE_LIBS)
  AC_SUBST(READLINE_INCLUDES)

  AC_ARG_WITH(readline,
    [[  --with-readline[=dir]   Compile with readline/locate base dir]],
    if test "x$withval" = "xno" ; then
      without_readline=yes
    elif test "x$withval" != "xyes" ; then
      with_arg="$withval/include:-L$withval/lib $withval/include/readline:-L$withval/lib"
    fi)

  AC_MSG_CHECKING(for readline.h)

  if test "x$cross_compiling" == "xyes"; then
      without_readline=yes
  fi

  if test "x$without_readline" != "xyes"; then
    for i in $with_arg \
	     /usr/include: \
	     /usr/local/include:-L/usr/local/lib \
	     /usr/pkg/include:-L/usr/pkg/lib; do
    
      incl=`echo "$i" | sed 's/:.*//'`
      lib=`echo "$i" | sed 's/.*://'`

      if test -f $incl/readline/readline.h ; then
        AC_MSG_RESULT($incl/readline/readline.h)
        READLINE_LIBS="$lib -lreadline"
	if test "$incl" != "/usr/include"; then
	  READLINE_INCLUDES="-I$incl/readline -I$incl"
	else
	  READLINE_INCLUDES="-I$incl/readline"
	fi
        AC_DEFINE(HAVE_READLINE, 1, [define if you have readline])
        have_readline=yes
        break
      elif test -f $incl/readline.h -a "x$incl" != "x/usr/include"; then
        AC_MSG_RESULT($incl/readline.h)
        READLINE_LIBS="$lib -lreadline"
        READLINE_INCLUDES="-I$incl"
        AC_DEFINE(HAVE_READLINE, 1, [define if you have readline])
        have_readline=yes
        break
      fi
    done
  fi

  if test "x$have_readline" != "xyes"; then
    AC_MSG_RESULT(not found)
  fi
])

