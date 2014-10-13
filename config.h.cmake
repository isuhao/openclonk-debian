/* Generate minidumps on crash */
#cmakedefine HAVE_DBGHELP 1

/* Define to 1 if you have the <direct.h> header file. */
#cmakedefine HAVE_DIRECT_H 1

/* The backtrace function is declared in execinfo.h and works */
#cmakedefine HAVE_EXECINFO_H 1

/* Define to 1 if you have the <history.h> header file. */
#cmakedefine HAVE_HISTORY_H 1

/* Define if you have the iconv() function. */
#cmakedefine HAVE_ICONV 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the <io.h> header file. */
#cmakedefine HAVE_IO_H 1

/* Define to 1 if you have the <langinfo.h> header file. */
#cmakedefine HAVE_LANGINFO_H 1

/* Define if you have a readline compatible library */
#cmakedefine HAVE_LIBREADLINE 1

/* Define to 1 if you have the <locale.h> header file. */
#cmakedefine HAVE_LOCALE_H 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have support for nullptr. */
#cmakedefine HAVE_NULLPTR 1

/* Define to 1 if you have the <poll.h> header file. */
#cmakedefine HAVE_POLL_H 1

/* Define if you have POSIX threads libraries and header files. */
#cmakedefine HAVE_PTHREAD 1

/* Define to 1 if you have the <readline.h> header file. */
#cmakedefine HAVE_READLINE_H 1

/* Define if your readline library has \`add_history' */
#cmakedefine HAVE_READLINE_HISTORY 1

/* Define to 1 if you have the <readline/history.h> header file. */
#cmakedefine HAVE_READLINE_HISTORY_H 1

/* Define to 1 if you have the <readline/readline.h> header file. */
#cmakedefine HAVE_READLINE_READLINE_H 1

/* C++ Compiler has rvalue references, a C++0x feature. */
#cmakedefine HAVE_RVALUE_REF 1

/* Define to 1 if you have SDL. */
#cmakedefine HAVE_SDL 1

/* Define to 1 if you have the <share.h> header file. */
#cmakedefine HAVE_SHARE_H 1

/* Define to 1 if you have the <signal.h> header file. */
#cmakedefine HAVE_SIGNAL_H 1

/* Define to 1 if your compiler supports static_assert */
#cmakedefine HAVE_STATIC_ASSERT 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the <sys/eventfd.h> header file. */
#cmakedefine HAVE_SYS_EVENTFD_H 1

/* Define to 1 if you have the <sys/file.h> header file. */
#cmakedefine HAVE_SYS_FILE_H 1

/* Define to 1 if you have the <sys/inotify.h> header file. */
#cmakedefine HAVE_SYS_INOTIFY_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#cmakedefine HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/timerfd.h> header file. */
#cmakedefine HAVE_SYS_TIMERFD_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to 1 if you have the `vasprintf' function. */
#cmakedefine HAVE_VASPRINTF 1

/* Define to 1 if you have the `__mingw_vasprintf' function. */
#cmakedefine HAVE___MINGW_VASPRINTF 1

#cmakedefine HAVE_VFW32

/* Define to 1 if you have the <X11/extensions/Xrandr.h> header file. */
#cmakedefine HAVE_X11_EXTENSIONS_XRANDR_H 1

/* Define to 1 if you have the <X11/keysym.h> header file. */
#cmakedefine HAVE_X11_KEYSYM_H 1

/* Define as const if the declaration of iconv() needs const. */
#define ICONV_CONST @ICONV_CONST@

/* compile without debug options */
#cmakedefine NDEBUG 1

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
#cmakedefine NO_MINUS_C_MINUS_O 1

/* Define to the address where bug reports for this package should be sent. */
#cmakedefine PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#cmakedefine PACKAGE_NAME

/* Define to the full name and version of this package. */
#cmakedefine PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#cmakedefine PACKAGE_TARNAME

/* Define to the version of this package. */
#cmakedefine PACKAGE_VERSION

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
#cmakedefine PTHREAD_CREATE_JOINABLE

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* dedicated server mode */
#cmakedefine USE_CONSOLE 1

/* MP3 music */
#cmakedefine USE_MP3 1

/* Define to 1 if SDL is used for the main loop */
#cmakedefine USE_SDL_MAINLOOP 1

/* Define to 1 if the X Window System is used */
#cmakedefine USE_X11 1

/* Enable automatic update system */
#cmakedefine WITH_AUTOMATIC_UPDATE 1

/* Developer mode */
#cmakedefine WITH_DEVELOPER_MODE 1

/* Define to 1 if you want to use Boost.Regex instead of <regex>. */
#cmakedefine USE_BOOST_REGEX 1

/* Glib */
#cmakedefine WITH_GLIB 1

/* Define to 1 if the X Window System is missing or not being used. */
#cmakedefine X_DISPLAY_MISSING 1

/* compile with debug options */
#cmakedefine _DEBUG 1

/* Define to 1 if you have support for precompiled headers */
#cmakedefine HAVE_PRECOMPILED_HEADERS 1

/* Use Apple Cocoa for the UI */
#cmakedefine USE_COCOA 1

/* Select an audio provider */
#define AUDIO_TK_NONE 0
#define AUDIO_TK_OPENAL 1
#define AUDIO_TK_FMOD 2
#define AUDIO_TK_SDL_MIXER 3
#define AUDIO_TK AUDIO_TK_${Audio_TK_UPPER}
