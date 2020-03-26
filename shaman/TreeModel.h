#pragma once

#include <QtWidgets/QTreeView>

class TreeItem
{
	QVector<QVariant> m_Cols;
	QVector<TreeItem*> m_childItems;
	TreeItem* m_parentItem;

public:
	explicit TreeItem(const QVector<QVariant>& data, TreeItem* parent)
		: m_Cols(data), m_parentItem(parent)
	{}

	explicit TreeItem(QVector<QVariant>&& data, TreeItem* parent)
		: m_Cols(std::move(data)), m_parentItem(parent)
	{}

	~TreeItem()
	{
		qDeleteAll(m_childItems);
	}

	void appendChild(TreeItem* item)
	{
		m_childItems.append(item);
	}

	TreeItem* appendItem(QVector<QVariant>&& cols)
	{
		TreeItem* item = new TreeItem(std::move(cols), this);
		appendChild(item);
		return item;
	}

	TreeItem* child(int row)
	{
		if (row < 0 || row >= m_childItems.size())
			return nullptr;
		return m_childItems.at(row);
	}

	int childCount() const
	{
		return m_childItems.count();
	}

	int columnCount() const
	{
		return m_Cols.count();
	}

	QVariant data(int column) const
	{
		if (column < 0 || column >= m_Cols.size())
			return QVariant();
		return m_Cols.at(column);
	}

	TreeItem* parentItem()
	{
		return m_parentItem;
	}

	int row() const
	{
		if (m_parentItem)
			return m_parentItem->m_childItems.indexOf(const_cast<TreeItem*>(this));

		return 0;
	}
};

class TreeModel : public QAbstractItemModel
{
public:
	using Super = QAbstractItemModel;

	TreeModel(QVector<QVariant>&& cols);
	~TreeModel() override;

	TreeItem* appendItem(QVector<QVariant>&& cols)
	{
		TreeItem* item = new TreeItem(std::move(cols), m_rootItem);
		m_rootItem->appendChild(item);
		return item;
	}

protected:
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& child) const override;

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	TreeItem* m_rootItem;
};
