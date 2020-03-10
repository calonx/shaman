
#include <windows.h>

#include <QtCore/QFile.h>
#include <QtCore/QSet.h>
#include <QtCore/QTextStream.h>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLayout>
#include <QtWidgets/QTreeView>

#include <rapidjson/document.h>

#include "ProqMon.h"
#include "MainWidget.h"

#include <stdio.h>

/*
	procmon.eventlist.event.[].Detail.Null
	procmon.eventlist.event.[].Detail.String
	procmon.eventlist.event.[].Operation.String
	procmon.eventlist.event.[].PID.String
	procmon.eventlist.event.[].Path.Null
	procmon.eventlist.event.[].Path.String
	procmon.eventlist.event.[].ProcessIndex.String
	procmon.eventlist.event.[].Process_Name.String
	procmon.eventlist.event.[].Result.String
	procmon.eventlist.event.[].Time_of_Day.String
	procmon.processlist.process.[].AuthenticationId.String
	procmon.processlist.process.[].CommandLine.Null
	procmon.processlist.process.[].CommandLine.String
	procmon.processlist.process.[].CompanyName.Null
	procmon.processlist.process.[].CompanyName.String
	procmon.processlist.process.[].CreateTime.String
	procmon.processlist.process.[].Description.Null
	procmon.processlist.process.[].Description.String
	procmon.processlist.process.[].FinishTime.String
	procmon.processlist.process.[].ImagePath.String
	procmon.processlist.process.[].Integrity.Null
	procmon.processlist.process.[].Integrity.String
	procmon.processlist.process.[].Is64bit.String
	procmon.processlist.process.[].IsVirtualized.String
	procmon.processlist.process.[].Owner.Null
	procmon.processlist.process.[].Owner.String
	procmon.processlist.process.[].ParentProcessId.String
	procmon.processlist.process.[].ParentProcessIndex.String
	procmon.processlist.process.[].ProcessId.String
	procmon.processlist.process.[].ProcessIndex.String
	procmon.processlist.process.[].ProcessName.String
	procmon.processlist.process.[].Version.Null
	procmon.processlist.process.[].Version.String
	procmon.processlist.process.[].modulelist.Null
	procmon.processlist.process.[].modulelist.module.[].BaseAddress.String
	procmon.processlist.process.[].modulelist.module.[].Company.Null
	procmon.processlist.process.[].modulelist.module.[].Company.String
	procmon.processlist.process.[].modulelist.module.[].Description.Null
	procmon.processlist.process.[].modulelist.module.[].Description.String
	procmon.processlist.process.[].modulelist.module.[].Path.String
	procmon.processlist.process.[].modulelist.module.[].Size.String
	procmon.processlist.process.[].modulelist.module.[].Timestamp.String
	procmon.processlist.process.[].modulelist.module.[].Version.Null
	procmon.processlist.process.[].modulelist.module.[].Version.String
*/

void Log(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	vprintf(fmt, args);

	char buf[4096];
	vsprintf_s(buf, fmt, args);
	OutputDebugStringA(buf);

	va_end(args);
}

void LogLine(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	
	vprintf(fmt, args);
	printf("\n");

	char buf[4096];
	vsprintf_s(buf, fmt, args);
	OutputDebugStringA(buf);
	OutputDebugStringA("\n");
	
	va_end(args);
}

QString kEventColumns[] =
{
  QStringLiteral("Detail"),
  QStringLiteral("Operation"),
  QStringLiteral("PID"),
  QStringLiteral("Path"),
  QStringLiteral("ProcessIndex"),
  QStringLiteral("Process_Name"),
  QStringLiteral("Result"),
  QStringLiteral("Time_of_Day"),
};

QString kModuleColumns[] =
{
  QStringLiteral("BaseAddress"),
  QStringLiteral("Company"),
  QStringLiteral("Description"),
  QStringLiteral("Path"),
  QStringLiteral("Size"),
  QStringLiteral("Timestamp"),
  QStringLiteral("Version"),
};

QString kProcessColumns[] =
{
	QStringLiteral("ProcessName"),
	QStringLiteral("ImagePath"),
	QStringLiteral("CommandLine"),
	QStringLiteral("CreateTime"),
	QStringLiteral("Description"),
	QStringLiteral("FinishTime"),
	QStringLiteral("Integrity"),
	QStringLiteral("Is64bit"),
	QStringLiteral("IsVirtualized"),
	QStringLiteral("Owner"),
	QStringLiteral("ParentProcessId"),
	QStringLiteral("ParentProcessIndex"),
	QStringLiteral("ProcessId"),
	QStringLiteral("ProcessIndex"),
	QStringLiteral("Version"),
	QStringLiteral("CompanyName"),
	QStringLiteral("AuthenticationId"),
};
	//QList<Module> modulelist;

class ProcessItem
{
	//	QString m_Cols[std::size(kProcessColumns)];
	QVector<QVariant> m_Cols;
	QVector<ProcessItem*> m_childItems;
	ProcessItem* m_parentItem;

public:
	explicit ProcessItem(const QVector<QVariant>& data, ProcessItem* parent)
		: m_Cols(data), m_parentItem(parent)
	{}

	explicit ProcessItem(QVector<QVariant>&& data, ProcessItem* parent)
		: m_Cols(std::move(data)), m_parentItem(parent)
	{}

	~ProcessItem()
	{
		qDeleteAll(m_childItems);
	}

	void appendChild(ProcessItem* item)
	{
		m_childItems.append(item);
	}

	ProcessItem* child(int row)
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

	ProcessItem* parentItem()
	{
		return m_parentItem;
	}

	int row() const
	{
		if (m_parentItem)
			return m_parentItem->m_childItems.indexOf(const_cast<ProcessItem*>(this));

		return 0;
	}
};

class ProcessModel : public QAbstractItemModel
{
public:
	using Super = QAbstractItemModel;

	ProcessModel();
	~ProcessModel() override;

	ProcessItem* appendItem(QVector<QVariant>&& cols)
	{
		ProcessItem* item = new ProcessItem(std::move(cols), m_rootItem);
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

	//struct ProcessEntry
	//{
	//};
	//QVector<ProcessEntry> m_Rows;
	ProcessItem* m_rootItem;
};

ProcessModel::ProcessModel() :
	Super()
{
	QVector<QVariant> cols;
	for (const QString& s : kProcessColumns)
		cols.append(s);
	m_rootItem = new ProcessItem(std::move(cols), nullptr);
}

ProcessModel::~ProcessModel()
{
	delete m_rootItem;
}

QModelIndex ProcessModel::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	ProcessItem* parentItem;

	if (!parent.isValid())
		parentItem = m_rootItem;
	else
		parentItem = static_cast<ProcessItem*>(parent.internalPointer());

	ProcessItem* childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);
	return QModelIndex();
}
QModelIndex ProcessModel::parent(const QModelIndex& index) const
{
	if (!index.isValid())
		return QModelIndex();

	ProcessItem* childItem = static_cast<ProcessItem*>(index.internalPointer());
	ProcessItem* parentItem = childItem->parentItem();

	if (parentItem == m_rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int ProcessModel::rowCount(const QModelIndex& parent) const
{
	ProcessItem* parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		parentItem = m_rootItem;
	else
		parentItem = static_cast<ProcessItem*>(parent.internalPointer());

	return parentItem->childCount();
}

int ProcessModel::columnCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return static_cast<ProcessItem*>(parent.internalPointer())->columnCount();
	return m_rootItem->columnCount();
}

QVariant ProcessModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	ProcessItem* item = static_cast<ProcessItem*>(index.internalPointer());

	return item->data(index.column());
}

Qt::ItemFlags ProcessModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	return QAbstractItemModel::flags(index);
}

QVariant ProcessModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)// && section < std::size(kProcessColumns))
		return m_rootItem->data(section);
	//return QVariant(kProcessColumns[section]);

	return QVariant();
}

int main(int argc, char** argv)
{
	//if (argc < 3)
	if (argc < 2)
	{
		//printf("Usage: ProqMon.exe <src_path> <dst_path>");
		LogLine("Usage: ProqMon.exe <src_path>");
		return 1;
	}
	QString src_path = argv[1];
	//QString dst_path = argv[2];

	LogLine("src_path = %s\n", src_path.toUtf8().cbegin());
	//Log("dst_path = %s\n", dst_path);
	QFile file(src_path);
	if (!file.open(QIODevice::OpenModeFlag::ReadOnly))
	{
		LogLine("Failed to open '%s'", src_path.toUtf8().cbegin());
		return 1;
	}

	QByteArray file_data = file.readAll();

	LogLine("%s\n", QStringRef(&QString(file_data), 0, 100).toString().toUtf8().constData());

	rapidjson::Document doc;
	doc.Parse(QString(file_data).toUtf8().constData());

	LogLine("%d\n", doc.GetType());

	ProcessModel model;
	
	auto obj_root = doc.GetObject();
	auto obj_procmon = obj_root.FindMember("procmon")->value.GetObject();
	auto obj_eventlist = obj_procmon.FindMember("eventlist")->value.GetObject();
	auto obj_processlist = obj_procmon.FindMember("processlist")->value.GetObject();

	auto arr_process = obj_processlist.FindMember("process")->value.GetArray();

	struct Handler
	{
		QSet<QString> seen;
		QList<QString> stack;

		void Cache(const QString& leaf)
		{
			stack.append(leaf);
			QStringList strings(stack);
			QString combined = strings.join('.');
			seen.insert(combined);
			stack.pop_back();
		};

		bool Null() { Cache(QStringLiteral("Null")); return true; }
		bool Bool(bool b) { Cache(QStringLiteral("Bool")); return true; }
		bool Int(int i) { Cache(QStringLiteral("Int")); return true; }
		bool Uint(unsigned i) { Cache(QStringLiteral("Uint")); return true; }
		bool Int64(int64_t i) { Cache(QStringLiteral("Int64")); return true; }
		bool Uint64(uint64_t i) { Cache(QStringLiteral("Uint64")); return true; }
		bool Double(double d) { Cache(QStringLiteral("Double")); return true; }
		bool RawNumber(const char* str, size_t length, bool copy) { Cache(QStringLiteral("RawNumber")); return true; }
		bool String(const char* str, size_t length, bool copy) { Cache(QStringLiteral("String")); return true; }
		bool StartObject() { stack.push_back("???"); return true; }
		bool Key(const char* str, size_t length, bool copy) { stack.last() = QString(str); return true; }
		bool EndObject(size_t memberCount) { stack.pop_back(); return true; }
		bool StartArray() { stack.append(QStringLiteral("[]")); return true; }
		bool EndArray(size_t elementCount) { stack.pop_back(); return true; }
	};

	Handler handler;
	doc.Accept(handler);

	QList<QString> seen_sorted = handler.seen.values();
	seen_sorted.sort();
	for (const QString& s : seen_sorted)
	{
		LogLine("%s", s.toUtf8().constData());
	}
	for (auto& v : arr_process)
	{
		QVector<QVariant> props;
		for (auto& col_name : kProcessColumns)
		{
			auto m = v.FindMember(col_name.toUtf8().constData());
			if (m != v.MemberEnd())
				props.push_back(QString(m->value.GetString()));
			else
				props.push_back(QVariant());
		}
		model.appendItem(std::move(props));
	}

	//return 0;

	// Creates an instance of QApplication
	QApplication a(argc, argv);

	// This is our MainWidget class containing our GUI and functionality
	//MainWidget w;
	//w.show(); // Show main window

	QTreeView tree;
	tree.setModel(&model);
	tree.setWindowTitle(QObject::tr("ProqMon"));
	tree.show();

	// run the application and return execs() return value/code
	return a.exec();
}
