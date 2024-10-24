#ifndef Q1ORM_GLOBAL_H
#define Q1ORM_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(Q1ORM_LIBRARY)
#  define Q1ORM_EXPORT Q_DECL_EXPORT
#else
#  define Q1ORM_EXPORT Q_DECL_IMPORT
#endif

#endif // Q1ORM_GLOBAL_H
