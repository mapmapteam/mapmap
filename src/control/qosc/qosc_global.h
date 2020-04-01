#include <QtCore/QtGlobal>

#if defined(QOSC_LIBRARY)
#  define QOSC_EXPORT Q_DECL_EXPORT
#else
#  define QOSC_EXPORT Q_DECL_IMPORT
#endif
