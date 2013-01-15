# Process this file with autoconf to produce a configure script.
AC_INIT(RtMidi, 2.0, gary@music.mcgill.ca, rtmidi)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(RtMidi.cpp)
AC_CONFIG_FILES(rtmidi-config Makefile tests/Makefile)

# Fill GXX with something before test.
AC_SUBST( GXX, ["no"] )

# Checks for programs.
AC_PROG_CXX(g++ CC c++ cxx)
AC_PROG_RANLIB
AC_PATH_PROG(AR, ar, no)
if [[ $AR = "no" ]] ; then
    AC_MSG_ERROR("Could not find ar - needed to create a library");
fi

# Checks for header files.
AC_HEADER_STDC
#AC_CHECK_HEADERS(sys/ioctl.h unistd.h)

# Check for debug
AC_MSG_CHECKING(whether to compile debug version)
AC_ARG_ENABLE(debug,
  [  --enable-debug = enable various debug output],
  [AC_SUBST( cppflag, [-D__RTMIDI_DEBUG__] ) AC_SUBST( cxxflag, [-g] ) AC_SUBST( object_path, [Debug] ) AC_MSG_RESULT(yes)],
  [AC_SUBST( cppflag, [] ) AC_SUBST( cxxflag, [-O3] ) AC_SUBST( object_path, [Release] ) AC_MSG_RESULT(no)])

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

# Checks for package options and external software
AC_CANONICAL_HOST

AC_SUBST( sharedlib, ["librtmidi.so"] )
AC_SUBST( sharedname, ["librtmidi.so.\$(RELEASE)"] )
AC_SUBST( libflags, ["-shared -Wl,-soname,\$(SHARED).\$(MAJOR) -o \$(SHARED).\$(RELEASE)"] )
case $host in
  *-apple*)
  AC_SUBST( sharedlib, ["librtmidi.dylib"] )
  AC_SUBST( sharedname, ["librtmidi.\$(RELEASE).dylib"] )
  AC_SUBST( libflags, ["-dynamiclib -o librtmidi.\$(RELEASE).dylib"] )
esac

AC_SUBST( api, [""] )
AC_MSG_CHECKING(for MIDI API)
case $host in
  *-*-linux*)
  AC_ARG_WITH(jack, [  --with-jack = choose JACK server support (mac and linux only)], [
  api="$api -D__UNIX_JACK__"
  AC_MSG_RESULT(using JACK)
  AC_CHECK_LIB(jack, jack_client_open, , AC_MSG_ERROR(JACK support requires the jack library!))], )

  # Look for ALSA flag
  AC_ARG_WITH(alsa, [  --with-alsa = choose native ALSA sequencer API support (linux only)], [
    api="$api -D__LINUX_ALSA__"
    AC_MSG_RESULT(using ALSA)
    AC_CHECK_LIB(asound, snd_seq_open, , AC_MSG_ERROR(ALSA support requires the asound library!))], )

  if [test "$api" == "";] then
    AC_MSG_RESULT(using ALSA)
    AC_SUBST( api, [-D__LINUX_ALSA__] )
    AC_CHECK_LIB(asound, snd_seq_open, , AC_MSG_ERROR(ALSA sequencer support requires the asound library!))
  fi

  # Checks for pthread library.
  AC_CHECK_LIB(pthread, pthread_create, , AC_MSG_ERROR(RtMidi requires the pthread library!))
  ;;

  *-apple*)
  AC_ARG_WITH(jack, [  --with-jack = choose JACK server support (mac and linux only)], [
  api="$api -D__UNIX_JACK__"
  AC_MSG_RESULT(using JACK)
  AC_CHECK_LIB(jack, jack_client_open, , AC_MSG_ERROR(JACK support requires the jack library!))], )

  # Look for Core flag
  AC_ARG_WITH(core, [  --with-core = choose CoreMidi API support (mac only)], [
    api="$api -D__MACOSX_CORE__"
    AC_MSG_RESULT(using CoreMidi)
    AC_CHECK_HEADER(CoreMIDI/CoreMIDI.h, [], [AC_MSG_ERROR(CoreMIDI header files not found!)] )
    LIBS="$LIBS -framework CoreMIDI -framework CoreFoundation -framework CoreAudio" ], )

  # If no api flags specified, use CoreMidi
  if [test "$api" == ""; ] then
    AC_SUBST( api, [-D__MACOSX_CORE__] )
    AC_MSG_RESULT(using CoreMidi)
    AC_CHECK_HEADER(CoreMIDI/CoreMIDI.h,
      [],
      [AC_MSG_ERROR(CoreMIDI header files not found!)] )
    AC_SUBST( LIBS, ["-framework CoreMIDI -framework CoreFoundation -framework CoreAudio"] )
  fi
  ;;

  *-mingw32*)
  # Look for WinMM flag
  AC_ARG_WITH(winmm, [  --with-winmm = choose Windows MultiMedia (MM) API support (windoze only)], [
    api="$api -D__WINDOWS_MM__"
    AC_MSG_RESULT(using WinMM)
    AC_SUBST( LIBS, [-lwinmm] )], )

  AC_ARG_WITH(winks, [  --with-winks = choose kernel streaming support (windoze only)], [
    api="$api -D__WINDOWS_KS__"
    AC_SUBST( LIBS, ["-lsetupapi -lksuser"] )
    AC_MSG_RESULT(using kernel streaming) ], )

  # I can't get the following check to work so just manually add the library
	# or could try the following?  AC_LIB_WINMM([midiOutGetNumDevs])
  # AC_CHECK_LIB(winmm, midiInGetNumDevs, , AC_MSG_ERROR(Windows MIDI support requires the winmm library!) )],)

  # If no api flags specified, use WinMM
  if [test "$api" == "";] then
    AC_SUBST( api, [-D__WINDOWS_MM__] )
    AC_MSG_RESULT(using WinMM)
    AC_SUBST( LIBS, [-lwinmm] )
  fi
  ;;

  *)
  # Default case for unknown realtime systems.
  AC_MSG_ERROR(Unknown system type for MIDI support!)
  ;;
esac

CPPFLAGS="$CPPFLAGS $api"

AC_OUTPUT

chmod oug+x rtmidi-config