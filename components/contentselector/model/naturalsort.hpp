#ifndef NATURALSORT_H
#define NATURALSORT_H

#include <QString>

    bool naturalSortLessThanCS( const QString &left, const QString &right );
    [[maybe_unused]] bool naturalSortLessThanCI( const QString &left, const QString &right );
    bool naturalSortGreaterThanCS( const QString &left, const QString &right );
    [[maybe_unused]] bool naturalSortGreaterThanCI( const QString &left, const QString &right );

#endif
