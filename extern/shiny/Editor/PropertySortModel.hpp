#ifndef SHINY_EDITOR_PROPERTYSORTMODEL_H
#define SHINY_EDITOR_PROPERTYSORTMODEL_H

#include <QSortFilterProxyModel>

namespace sh
{

	class PropertySortModel : public QSortFilterProxyModel
	{
		Q_OBJECT

	public:
		PropertySortModel(QObject* parent);
	protected:
		bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
	};

}

#endif
