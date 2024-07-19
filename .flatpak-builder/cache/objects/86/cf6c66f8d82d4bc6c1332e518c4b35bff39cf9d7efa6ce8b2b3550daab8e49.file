
#ifndef POPPLER_QT6_EXPORT_H
#define POPPLER_QT6_EXPORT_H

#ifdef POPPLER_QT6_STATIC_DEFINE
#  define POPPLER_QT6_EXPORT
#  define POPPLER_QT6_NO_EXPORT
#else
#  ifndef POPPLER_QT6_EXPORT
#    ifdef poppler_qt6_EXPORTS
        /* We are building this library */
#      define POPPLER_QT6_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define POPPLER_QT6_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef POPPLER_QT6_NO_EXPORT
#    define POPPLER_QT6_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef POPPLER_QT6_DEPRECATED
#  define POPPLER_QT6_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef POPPLER_QT6_DEPRECATED_EXPORT
#  define POPPLER_QT6_DEPRECATED_EXPORT POPPLER_QT6_EXPORT POPPLER_QT6_DEPRECATED
#endif

#ifndef POPPLER_QT6_DEPRECATED_NO_EXPORT
#  define POPPLER_QT6_DEPRECATED_NO_EXPORT POPPLER_QT6_NO_EXPORT POPPLER_QT6_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef POPPLER_QT6_NO_DEPRECATED
#    define POPPLER_QT6_NO_DEPRECATED
#  endif
#endif

#endif /* POPPLER_QT6_EXPORT_H */
