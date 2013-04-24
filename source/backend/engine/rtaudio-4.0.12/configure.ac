# Process this file with autoconf to produce a configure script.
AC_INIT(RtAudio, 4.0, gary@music.mcgill.ca, rtaudio)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(RtAudio.cpp)
AC_CONFIG_FILES([rtaudio-config librtaudio.pc Makefile tests/Makefile])

# Fill GXX with something before test.
AC_SUBST( GXX, ["no"] )

dnl Check for pkg-config program, used for configuring some libraries.
m4_define_default([PKG_PROG_PKG_CONFIG],
[AC_MSG_CHECKING([pkg-config])
AC_MSG_RESULT([no])])

PKG_PROG_PKG_CONFIG

dnl If the pkg-config autoconf support isn't installed, define its
dnl autoconf macro to disable any packages depending on it.
m4_define_default([PKG_CHECK_MODULES],
[AC_MSG_CHECKING([$1])
AC_MSG_RESULT([no])
$4])

# Checks for programs.
AC_PROG_CXX(g++ CC c++ cxx)
AC_PROG_RANLIB
AC_PATH_PROG(AR, ar, no)
if [[ $AR = "no" ]] ; then
    AC_MSG_ERROR("Could not find ar - needed to create a library");
fi

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(sys/ioctl.h unistd.h)

# Check for debug
AC_MSG_CHECKING(whether to compile debug version)
AC_ARG_ENABLE(debug,
  [  --enable-debug = enable various debug output],
  [AC_SUBST( cppflag, [-D__RTAUDIO_DEBUG__] ) AC_SUBST( cxxflag, [-g] ) AC_SUBST( object_path, [Debug] ) AC_MSG_RESULT(yes)],
  [AC_SUBST( cppflag, [] ) AC_SUBST( cxxflag, [-O2] ) AC_SUBST( object_path, [Release] ) AC_MSG_RESULT(no)])


# Checks for functions
AC_CHECK_FUNC(gettimeofday, [cppflag="$cppflag -DHAVE_GETTIMEOFDAY"], )

# For -I and -D flags
CPPFLAGS="$CPPFLAGS $cppflag"

# For debugging and optimization ... overwrite default because it has both -g and -O2
#CXXFLAGS="$CXXFLAGS $cxxflag"
CXXFLAGS="$cxxflag"

# Check compiler and use -Wall if gnu.
if [test $GXX = "yes" ;] then
  AC_SUBST( cxxflag, [-Wall] )
fi

CXXFLAGS="$CXXFLAGS $cxxflag"

AC_CANONICAL_HOST

AC_SUBST( sharedlib, ["librtaudio.so"] )
AC_SUBST( sharedname, ["librtaudio.so.\$(RELEASE)"] )
AC_SUBST( libflags, ["-shared -Wl,-soname,\$(SHARED).\$(MAJOR) -o \$(SHARED).\$(RELEASE)"] )
case $host in
  *-apple*)
  AC_SUBST( sharedlib, ["librtaudio.dylib"] )
  AC_SUBST( sharedname, ["librtaudio.\$(RELEASE).dylib"] )
  AC_SUBST( libflags, ["-dynamiclib -o librtaudio.\$(RELEASE).dylib"] )
esac

# Checks for package options and external software
AC_SUBST( api, [""] )
AC_SUBST( req, [""] )
AC_MSG_CHECKING(for audio API)
case $host in
  *-*-netbsd*)
    AC_MSG_RESULT(using OSS)
    api="$api -D__LINUX_OSS__"
    LIBS="$LIBS -lossaudio"
    AC_CHECK_LIB(pthread, pthread_create, , AC_MSG_ERROR(RtAudio requires the pthread library!))
  ;;

  *-*-linux*)
  AC_ARG_WITH(jack, [  --with-jack = choose JACK server support (mac and linux only)], [
    api="$api -D__UNIX_JACK__"
    AC_MSG_RESULT(using JACK)
    AC_CHECK_LIB(jack, jack_client_open, , AC_MSG_ERROR(JACK support requires the jack library!))
    AC_CHECK_LIB(asound, snd_pcm_open, , AC_MSG_ERROR(Jack support also requires the asound library!))], )

  # Look for ALSA flag
  AC_ARG_WITH(alsa, [  --with-alsa = choose native ALSA API support (linux only)], [
    api="$api -D__LINUX_ALSA__"
    req="$req alsa"
    AC_MSG_RESULT(using ALSA)
    AC_CHECK_LIB(asound, snd_pcm_open, , AC_MSG_ERROR(ALSA support requires the asound library!))], )

  # Look for PULSE flag
  AC_ARG_WITH(pulse, [  --with-pulse = choose PulseAudio API support (linux only)], [
    api="$api -D__LINUX_PULSE__"
    req="$req libpulse-simple"
    AC_MSG_RESULT(using PulseAudio)
    PKG_CHECK_MODULES([PULSE], [libpulse-simple], , AC_MSG_ERROR(PulseAudio support requires the pulse-simple library!))
        LIBS="$LIBS `pkg-config --libs libpulse-simple`" ], )

  # Look for OSS flag
  AC_ARG_WITH(oss, [  --with-oss = choose OSS API support (linux only)], [
    api="$api -D__LINUX_OSS__"
    AC_MSG_RESULT(using OSS)], )

  # If no audio api flags specified, use ALSA
  if [test "$api" == "";] then
    AC_MSG_RESULT(using ALSA)
    AC_SUBST( api, [-D__LINUX_ALSA__] )
    req="$req alsa"
    AC_CHECK_LIB(asound, snd_pcm_open, , AC_MSG_ERROR(ALSA support requires the asound library!))
  fi

  AC_CHECK_LIB(pthread, pthread_create, , AC_MSG_ERROR(RtAudio requires the pthread library!))
  ;;

  *-apple*)
  AC_ARG_WITH(jack, [  --with-jack = choose JACK server support (unix only)], [
    api="$api -D__UNIX_JACK__"
    AC_MSG_RESULT(using JACK)
    AC_CHECK_LIB(jack, jack_client_open, , AC_MSG_ERROR(JACK support requires the jack library!))], )

#    AC_CHECK_HEADER(jack/jack.h, [], [AC_MSG_ERROR(Jack header file not found!)] )
#    LIBS="$LIBS -framework jackmp" ], )


  # Look for Core flag
  AC_ARG_WITH(core, [  --with-core = choose CoreAudio API support (mac only)], [
    api="$api -D__MACOSX_CORE__"
    AC_MSG_RESULT(using CoreAudio)
    AC_CHECK_HEADER(CoreAudio/CoreAudio.h, [], [AC_MSG_ERROR(CoreAudio header files not found!)] )
    LIBS="$LIBS -framework CoreAudio -framework CoreFoundation" ], )

  # If no audio api flags specified, use CoreAudio
  if [test "$api" == ""; ] then
    AC_SUBST( api, [-D__MACOSX_CORE__] )
    AC_MSG_RESULT(using CoreAudio)
    AC_CHECK_HEADER(CoreAudio/CoreAudio.h,
      [],
      [AC_MSG_ERROR(CoreAudio header files not found!)] )
    AC_SUBST( LIBS, ["-framework CoreAudio -framework CoreFoundation"] )
  fi

  AC_CHECK_LIB(pthread, pthread_create, , AC_MSG_ERROR(RtAudio requires the pthread library!))
  ;;

  *-mingw32*)
  AC_ARG_WITH(asio, [  --with-asio = choose ASIO API support (windoze only)], [
    api="$api -D__WINDOWS_ASIO__"
    AC_MSG_RESULT(using ASIO)
    AC_SUBST( objects, ["asio.o asiodrivers.o asiolist.o iasiothiscallresolver.o"] ) ], )

  # Look for DirectSound flag
  AC_ARG_WITH(ds, [  --with-ds = choose DirectSound API support (windoze only)], [
    api="$api -D__WINDOWS_DS__"
    AC_MSG_RESULT(using DirectSound)
    LIBS="-ldsound -lwinmm $LIBS" ], )

  # If no audio api flags specified, use DirectSound
  if [test "$api" == "";] then
    AC_SUBST( api, [-D__WINDOWS_DS__] )
    AC_MSG_RESULT(using DirectSound)
    LIBS="-ldsound -lwinmm $LIBS"
  fi

  LIBS="-lole32 $LIBS"
  ;;

  *)
  # Default case for unknown realtime systems.
  AC_MSG_ERROR(Unknown system type for realtime support!)
  ;;
esac

CPPFLAGS="$CPPFLAGS $api"

AC_OUTPUT

chmod oug+x rtaudio-config
