/*================================================================
*   Created: LiXingang 
*   Description
*
================================================================*/
#ifndef __OS_DEF_H_
#define __OS_DEF_H_
#ifdef __cplusplus
extern "C"
{
#endif

#if defined(__APPLE__) && defined(__GNUC__)   
#  define Q_OS_MACX   
#elif defined(__MACOSX__)   
#  define Q_OS_MACX   
#elif defined(macintosh)   
#  define Q_OS_MAC9   
#elif defined(__CYGWIN__)   
#  define Q_OS_CYGWIN   
#elif defined(MSDOS) || defined(_MSDOS)   
#  define Q_OS_MSDOS   
#elif defined(__OS2__)   
#  if defined(__EMX__)   
#    define Q_OS_OS2EMX   
#  else   
#    define Q_OS_OS2   
#  endif   
#elif !defined(SAG_COM) && (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))   
#  define Q_OS_WIN32   
#  define Q_OS_WIN64   
#elif !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))   
#  define Q_OS_WIN32   
#elif defined(__MWERKS__) && defined(__INTEL__)   
#  define Q_OS_WIN32   
#elif defined(__sun) || defined(sun)   
#  define Q_OS_SOLARIS   
#elif defined(hpux) || defined(__hpux)   
#  define Q_OS_HPUX   
#elif defined(__ultrix) || defined(ultrix)   
#  define Q_OS_ULTRIX   
#elif defined(sinix)   
#  define Q_OS_RELIANT   
#elif defined(__linux__) || defined(__linux)   
#  define Q_OS_LINUX   
#elif defined(__FreeBSD__)   
#  define Q_OS_FREEBSD   
#  define Q_OS_BSD4   
#elif defined(__NetBSD__)   
#  define Q_OS_NETBSD   
#  define Q_OS_BSD4   
#elif defined(__OpenBSD__)   
#  define Q_OS_OPENBSD   
#  define Q_OS_BSD4   
#elif defined(__bsdi__)   
#  define Q_OS_BSDI   
#  define Q_OS_BSD4   
#elif defined(__sgi)   
#  define Q_OS_IRIX   
#elif defined(__osf__)   
#  define Q_OS_OSF   
#elif defined(_AIX)   
#  define Q_OS_AIX   
#elif defined(__Lynx__)   
#  define Q_OS_LYNX   
#elif defined(__GNU_HURD__)   
#  define Q_OS_HURD   
#elif defined(__DGUX__)   
#  define Q_OS_DGUX   
#elif defined(__QNXNTO__)   
#  define Q_OS_QNX6   
#elif defined(__QNX__)   
#  define Q_OS_QNX   
#elif defined(_SEQUENT_)   
#  define Q_OS_DYNIX   
#elif defined(_SCO_DS)                   /* SCO OpenServer 5 + GCC */   
#  define Q_OS_SCO   
#elif defined(__USLC__)                  /* all SCO platforms + UDK or OUDK */   
#  define Q_OS_UNIXWARE   
#  define Q_OS_UNIXWARE7   
#elif defined(__svr4__) && defined(i386) /* Open UNIX 8 + GCC */   
#  define Q_OS_UNIXWARE   
#  define Q_OS_UNIXWARE7   
#else   
#  error "error"
#endif   
  
#if defined(Q_OS_MAC9) || defined(Q_OS_MACX)   
#  define Q_OS_MAC   
#endif   
  
#if defined(Q_OS_MAC9) || defined(Q_OS_MSDOS) || defined(Q_OS_OS2) || defined(Q_OS_WIN32) || defined(Q_OS_WIN64)   
#  undef Q_OS_UNIX   
#elif !defined(Q_OS_UNIX)   
#  define Q_OS_UNIX   
#endif  

#ifdef Q_OS_UNIX
#  define IN_UNIXLIKE
#endif

#ifdef Q_OS_LINUX
#  define IN_LINUX
#endif

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
#  define IN_WINDOWS
#endif

#ifdef Q_OS_MAC
#  define IN_MAC
#endif

#ifdef __cplusplus
}
#endif
#endif //OS_DEF_H_
