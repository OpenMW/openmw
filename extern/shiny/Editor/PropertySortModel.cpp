#include "PropertySortModel.hpp"

#include "Query.hpp"

#include <iostream>
sh::PropertySortModel::PropertySortModel(QObject *parent)
	: QSortFilterProxyModel(parent)
{
}

bool sh::PropertySortModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	if (left.data(Qt::UserRole+1).toInt() != 0 && right.data(Qt::UserRole+1).toInt() != 0)
	{
		int sourceL = left.data(Qt::UserRole+1).toInt();
		int sourceR = right.data(Qt::UserRole+1).toInt();

		if (sourceL > sourceR)
			return true;
		else if (sourceR > sourceL)
			return false;
	}

	int typeL = left.data(Qt::UserRole).toInt();
	int typeR = right.data(Qt::UserRole).toInt();

	if (typeL > typeR)
		return true;
	else if (typeR > typeL)
		return false;

	QString nameL = left.data().toString();
	QString nameR = right.data().toString();
	return nameL > nameR;
}
