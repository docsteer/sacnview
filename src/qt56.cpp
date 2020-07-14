#include "qt56.h"

#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    QTextStream &Qt::endl(QTextStream &s) { return s << '\n' << flush; }
#endif
