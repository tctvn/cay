#ifndef MINIZ_EXPORT_H
#define MINIZ_EXPORT_H

#if defined(_WIN32)
#  if defined(MINIZ_SHARED_LIB)
#    define MINIZ_EXPORT __declspec(dllexport)
#  else
#    define MINIZ_EXPORT
#  endif
#else
#  define MINIZ_EXPORT
#endif

#endif