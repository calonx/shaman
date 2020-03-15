
#include <windows.h>

#include <QtCore/QFile.h>
#include <QtCore/QSet.h>
#include <QtCore/QTextStream.h>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLayout>
#include <QtWidgets/QTreeView>
#include <QtTest/QAbstractItemModelTester.h>

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

enum Visibility
{
	kHide,
	kShow,
};

enum class EventColumns
{
	kDetail,
	kOperation,
	kPid,
	kPath,
	kProcessIndex,
	kProcessName,
	kResult,
	kTimeOfDay,

	kItemCount
};

QString GetEventColumnLabel(EventColumns column)
{
	switch (column)
	{
		case EventColumns::kDetail:       return QStringLiteral("Detail");
		case EventColumns::kOperation:    return QStringLiteral("Operation");
		case EventColumns::kPid:          return QStringLiteral("PID");
		case EventColumns::kPath:         return QStringLiteral("Path");
		case EventColumns::kProcessIndex: return QStringLiteral("ProcessIndex");
		case EventColumns::kProcessName:  return QStringLiteral("Process_Name");
		case EventColumns::kResult:       return QStringLiteral("Result");
		case EventColumns::kTimeOfDay:    return QStringLiteral("Time_of_Day");
	}
	return QStringLiteral("<unknown>");
}

struct EventColumn
{
	Visibility visibility;
	EventColumns columnId;
	QString label = QStringLiteral("<unknown>");
};

EventColumn s_EventColumns[] =
{
	{ kShow, EventColumns::kTimeOfDay,    GetEventColumnLabel(EventColumns::kTimeOfDay) },
	{ kShow, EventColumns::kProcessName,  GetEventColumnLabel(EventColumns::kProcessName) },
	{ kShow, EventColumns::kOperation,    GetEventColumnLabel(EventColumns::kOperation) },
	{ kShow, EventColumns::kResult,       GetEventColumnLabel(EventColumns::kResult) },
	{ kShow, EventColumns::kPid,          GetEventColumnLabel(EventColumns::kPid) },
	{ kShow, EventColumns::kDetail,       GetEventColumnLabel(EventColumns::kDetail) },
	{ kShow, EventColumns::kPath,         GetEventColumnLabel(EventColumns::kPath) },
	{ kHide, EventColumns::kProcessIndex, GetEventColumnLabel(EventColumns::kProcessIndex) },
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



enum class ProcessColumns
{
	kProcessName,
	kImagePath,
	kCommandLine,
	kCreateTime,
	kDescription,
	kFinishTime,
	kIntegrity,
	kIs64bit,
	kIsVirtualized,
	kOwner,
	kParentProcessId,
	kParentProcessIndex,
	kProcessId,
	kProcessIndex,
	kVersion,
	kCompanyName,
	kAuthenticationId,

	kItemCount
};

QString GetProcessColumnLabel(ProcessColumns column)
{
	switch (column)
	{
		case ProcessColumns::kProcessName:               return QStringLiteral("ProcessName");
		case ProcessColumns::kImagePath:                 return QStringLiteral("ImagePath");
		case ProcessColumns::kCommandLine:               return QStringLiteral("CommandLine");
		case ProcessColumns::kCreateTime:                return QStringLiteral("CreateTime");
		case ProcessColumns::kDescription:               return QStringLiteral("Description");
		case ProcessColumns::kFinishTime:                return QStringLiteral("FinishTime");
		case ProcessColumns::kIntegrity:                 return QStringLiteral("Integrity");
		case ProcessColumns::kIs64bit:                   return QStringLiteral("Is64bit");
		case ProcessColumns::kIsVirtualized:             return QStringLiteral("IsVirtualized");
		case ProcessColumns::kOwner:                     return QStringLiteral("Owner");
		case ProcessColumns::kParentProcessId:           return QStringLiteral("ParentProcessId");
		case ProcessColumns::kParentProcessIndex:        return QStringLiteral("ParentProcessIndex");
		case ProcessColumns::kProcessId:                 return QStringLiteral("ProcessId");
		case ProcessColumns::kProcessIndex:              return QStringLiteral("ProcessIndex");
		case ProcessColumns::kVersion:                   return QStringLiteral("Version");
		case ProcessColumns::kCompanyName:               return QStringLiteral("CompanyName");
		case ProcessColumns::kAuthenticationId:          return QStringLiteral("AuthenticationId");
	}
	return QStringLiteral("<unknown>");
}

struct ProcessColumn
{
	Visibility visibility;
	ProcessColumns columnId;
	QString label = QStringLiteral("<unknown>");
};

ProcessColumn s_ProcessColumns[] =
{
	{ kShow, ProcessColumns::kProcessName,         GetProcessColumnLabel(ProcessColumns::kProcessName) },
	{ kShow, ProcessColumns::kImagePath,           GetProcessColumnLabel(ProcessColumns::kImagePath) },
	{ kShow, ProcessColumns::kCommandLine,         GetProcessColumnLabel(ProcessColumns::kCommandLine) },
	{ kHide, ProcessColumns::kCreateTime,          GetProcessColumnLabel(ProcessColumns::kCreateTime) },
	{ kShow, ProcessColumns::kDescription,         GetProcessColumnLabel(ProcessColumns::kDescription) },
	{ kHide, ProcessColumns::kFinishTime,          GetProcessColumnLabel(ProcessColumns::kFinishTime) },
	{ kHide, ProcessColumns::kIntegrity,           GetProcessColumnLabel(ProcessColumns::kIntegrity) },
	{ kHide, ProcessColumns::kIs64bit,             GetProcessColumnLabel(ProcessColumns::kIs64bit) },
	{ kHide, ProcessColumns::kIsVirtualized,       GetProcessColumnLabel(ProcessColumns::kIsVirtualized) },
	{ kShow, ProcessColumns::kOwner,               GetProcessColumnLabel(ProcessColumns::kOwner) },
	{ kShow, ProcessColumns::kParentProcessId,     GetProcessColumnLabel(ProcessColumns::kParentProcessId) },
	{ kHide, ProcessColumns::kParentProcessIndex,  GetProcessColumnLabel(ProcessColumns::kParentProcessIndex) },
	{ kShow, ProcessColumns::kProcessId,           GetProcessColumnLabel(ProcessColumns::kProcessId) },
	{ kHide, ProcessColumns::kProcessIndex,        GetProcessColumnLabel(ProcessColumns::kProcessIndex) },
	{ kHide, ProcessColumns::kVersion,             GetProcessColumnLabel(ProcessColumns::kVersion) },
	{ kHide, ProcessColumns::kCompanyName,         GetProcessColumnLabel(ProcessColumns::kCompanyName) },
	{ kHide, ProcessColumns::kAuthenticationId,    GetProcessColumnLabel(ProcessColumns::kAuthenticationId) },
};

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

TreeModel::TreeModel(QVector<QVariant>&& cols) :
	Super()
{
	m_rootItem = new TreeItem(std::move(cols), nullptr);
}

TreeModel::~TreeModel()
{
	delete m_rootItem;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	TreeItem* parentItem;

	if (!parent.isValid())
		parentItem = m_rootItem;
	else
		parentItem = static_cast<TreeItem*>(parent.internalPointer());

	TreeItem* childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);
	return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex& index) const
{
	if (!index.isValid())
		return QModelIndex();

	TreeItem* childItem = static_cast<TreeItem*>(index.internalPointer());
	TreeItem* parentItem = childItem->parentItem();

	if (parentItem == m_rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex& parent) const
{
	TreeItem* parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		parentItem = m_rootItem;
	else
		parentItem = static_cast<TreeItem*>(parent.internalPointer());

	return parentItem->childCount();
}

int TreeModel::columnCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
	return m_rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	TreeItem* item = static_cast<TreeItem*>(index.internalPointer());

	return item->data(index.column());
}

Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	return QAbstractItemModel::flags(index);
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return m_rootItem->data(section);

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

	//LogLine("%s\n", QStringRef(&QString(file_data), 0, 100).toString().toUtf8().constData());

	rapidjson::Document doc;
	doc.Parse(QString(file_data).toUtf8().constData());

	//LogLine("%d\n", doc.GetType());
	
	auto obj_root = doc.GetObject();
	auto obj_procmon = obj_root.FindMember("procmon")->value.GetObject();
	auto obj_eventlist = obj_procmon.FindMember("eventlist")->value.GetObject();
	auto obj_processlist = obj_procmon.FindMember("processlist")->value.GetObject();

	auto arr_process = obj_processlist.FindMember("process")->value.GetArray();
	auto arr_event = obj_eventlist.FindMember("event")->value.GetArray();

	//struct Handler
	//{
	//	QSet<QString> seen;
	//	QList<QString> stack;

	//	void Cache(const QString& leaf)
	//	{
	//		stack.append(leaf);
	//		QStringList strings(stack);
	//		QString combined = strings.join('.');
	//		seen.insert(combined);
	//		stack.pop_back();
	//	};

	//	bool Null() { Cache(QStringLiteral("Null")); return true; }
	//	bool Bool(bool b) { Cache(QStringLiteral("Bool")); return true; }
	//	bool Int(int i) { Cache(QStringLiteral("Int")); return true; }
	//	bool Uint(unsigned i) { Cache(QStringLiteral("Uint")); return true; }
	//	bool Int64(int64_t i) { Cache(QStringLiteral("Int64")); return true; }
	//	bool Uint64(uint64_t i) { Cache(QStringLiteral("Uint64")); return true; }
	//	bool Double(double d) { Cache(QStringLiteral("Double")); return true; }
	//	bool RawNumber(const char* str, size_t length, bool copy) { Cache(QStringLiteral("RawNumber")); return true; }
	//	bool String(const char* str, size_t length, bool copy) { Cache(QStringLiteral("String")); return true; }
	//	bool StartObject() { stack.push_back("???"); return true; }
	//	bool Key(const char* str, size_t length, bool copy) { stack.last() = QString(str); return true; }
	//	bool EndObject(size_t memberCount) { stack.pop_back(); return true; }
	//	bool StartArray() { stack.append(QStringLiteral("[]")); return true; }
	//	bool EndArray(size_t elementCount) { stack.pop_back(); return true; }
	//};

	QVector<QVariant> process_cols;
	for (const ProcessColumn& col : s_ProcessColumns)
		process_cols.append(col.label);
	TreeModel process_model(std::move(process_cols));

	QVector<QVariant> event_cols;
	for (const EventColumn& col : s_EventColumns)
		event_cols.append(col.label);
	TreeModel event_model(std::move(event_cols));

	//Handler handler;
	//doc.Accept(handler);

	//QList<QString> seen_sorted = handler.seen.values();
	//seen_sorted.sort();
	//for (const QString& s : seen_sorted)
	//{
	//	LogLine("%s", s.toUtf8().constData());
	//}

	for (auto& v : arr_process)
	{
		QVector<QVariant> props;
		for (const auto& col : s_ProcessColumns)
		{
			auto m = v.FindMember(col.label.toUtf8().constData());
			if (m != v.MemberEnd())
				props.push_back(QString(m->value.GetString()));
			else
				props.push_back(QVariant());
		}
		process_model.appendItem(std::move(props));
	}

	for (auto& v : arr_event)
	{
		QVector<QVariant> props;
		for (const auto& col : s_EventColumns)
		{
			auto m = v.FindMember(col.label.toUtf8().constData());
			if (m != v.MemberEnd())
				props.push_back(QString(m->value.GetString()));
			else
				props.push_back(QVariant());
		}
		event_model.appendItem(std::move(props));
	}

	// Creates an instance of QApplication
	QApplication a(argc, argv);

	QTreeView tree;
	tree.setUniformRowHeights(true);
	tree.setWindowTitle(QObject::tr("ProqMon"));

	//tree.setModel(&process_model);
	//for (int i = 0; i < std::size(s_ProcessColumns); ++i)
	//{
	//	tree.setColumnHidden(i, s_ProcessColumns[i].visibility == kHide);
	//}

	//new QAbstractItemModelTester(&event_model, QAbstractItemModelTester::FailureReportingMode::Fatal, &tree);

	tree.setModel(&event_model);
	for (int i = 0; i < std::size(s_EventColumns); ++i)
	{
		tree.setColumnHidden(i, s_EventColumns[i].visibility == kHide);
	}

	tree.show();

	// run the application and return execs() return value/code
	return a.exec();
}
