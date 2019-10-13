#ifndef QT56_H
#define QT56_H

#if (QT_VERSION < QT_VERSION_CHECK(5, 8, 0))
    /*
     * Q_FALLTHROUGH()
     * This function was introduced in Qt 5.8.
     *
     * /src/corelib/global/qcompilerdetection.h
     */
    #if defined(__cplusplus)
    #if QT_HAS_CPP_ATTRIBUTE(clang::fallthrough)
    #    define Q_FALLTHROUGH() [[clang::fallthrough]]
    #elif QT_HAS_CPP_ATTRIBUTE(gnu::fallthrough)
    #    define Q_FALLTHROUGH() [[gnu::fallthrough]]
    #elif QT_HAS_CPP_ATTRIBUTE(fallthrough)
    #  define Q_FALLTHROUGH() [[fallthrough]]
    #endif
    #endif
    #ifndef Q_FALLTHROUGH
    #  if (defined(Q_CC_GNU) && Q_CC_GNU >= 700) && !defined(Q_CC_INTEL)
    #    define Q_FALLTHROUGH() __attribute__((fallthrough))
    #  else
    #    define Q_FALLTHROUGH() (void)0
    #endif
    #endif
#endif


#endif // QT56_H
