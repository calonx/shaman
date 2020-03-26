
#include <windows.h>

#include <QtCore/QFile.h>
#include <QtCore/QSet.h>
#include <QtCore/QTextStream.h>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLayout>

#include <rapidjson/document.h>

#include "shaman.h"
#include "TreeModel.h"
#include "MainWidget.h"

#include <stdio.h>

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
	{ kShow, EventColumns::kProcessName,  GetEventColumnLabel(EventColumns::kProcessName) },
	{ kShow, EventColumns::kTimeOfDay,    GetEventColumnLabel(EventColumns::kTimeOfDay) },
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

template<int N>
constexpr unsigned __int64 Sid(const char s[N])
{
	unsigned __int64 sid = 0;
	for (int i = 0; i < std::size(s); ++i)
	{
		sid = sid * 101 + s[i];
	}
	return sid;
}

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

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		LogLine("Usage: shaman.exe <src_path>");
		return 1;
	}
	QString src_path = argv[1];

	LogLine("src_path = %s\n", src_path.toUtf8().cbegin());
	QFile file(src_path);
	if (!file.open(QIODevice::OpenModeFlag::ReadOnly))
	{
		LogLine("Failed to open '%s'", src_path.toUtf8().cbegin());
		return 1;
	}

	QByteArray file_data = file.readAll();

	rapidjson::Document doc;
	doc.Parse(QString(file_data).toUtf8().constData());

	auto obj_root = doc.GetObject();
	auto obj_procmon = obj_root.FindMember("procmon")->value.GetObject();
	auto obj_eventlist = obj_procmon.FindMember("eventlist")->value.GetObject();
	auto obj_processlist = obj_procmon.FindMember("processlist")->value.GetObject();

	auto arr_process = obj_processlist.FindMember("process")->value.GetArray();
	auto arr_event = obj_eventlist.FindMember("event")->value.GetArray();

	QVector<QVariant> process_cols;
	for (const ProcessColumn& col : s_ProcessColumns)
		process_cols.append(col.label);
	TreeModel process_model(std::move(process_cols));

	QVector<QVariant> event_cols;
	for (const EventColumn& col : s_EventColumns)
		event_cols.append(col.label);
	TreeModel event_model(std::move(event_cols));

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

	QHash<QString, TreeItem*> proc_map;

	for (auto& v : arr_event)
	{
		QVector<QVariant> parent_props;
		QVector<QVariant> props;
		for (const auto& col : s_EventColumns)
		{
			props.push_back(QVariant());
			parent_props.push_back(QVariant());
			
			auto m = v.FindMember(col.label.toUtf8().constData());
			if (m == v.MemberEnd())
				continue;

			if (parent_props.length() == 1)
			{
				parent_props.last() = QString(m->value.GetString());
			}
			else
			{
				props.last() = QString(m->value.GetString());
			}
		}

		QString group_str = parent_props.first().toString().toLower();
		TreeItem* item = proc_map[group_str];
		if (!item)
		{
			item = event_model.appendItem(std::move(parent_props));
			proc_map.insert(group_str, item);
		}
		item->appendItem(std::move(props));
	}

	QApplication app(argc, argv);

	QTreeView tree;
	//tree.setUniformRowHeights(true);
	tree.setAnimated(false);
	tree.setWindowTitle(QObject::tr("shaman"));

	tree.setModel(&event_model);
	//tree.setModel(&process_model);
	//for (int i = 0; i < std::size(s_ProcessColumns); ++i)
	//{
	//	tree.setColumnHidden(i, s_ProcessColumns[i].visibility == kHide);
	//}

	//tree.setModel(&process_model);
	for (int i = 0; i < std::size(s_EventColumns); ++i)
	{
		tree.setColumnHidden(i, s_EventColumns[i].visibility == kHide);
	}

	printf("Row count: %d\n", tree.model()->rowCount());
	tree.expandAll();
	tree.setSortingEnabled(true);
	tree.show();

	// run the application and return execs() return value/code
	return app.exec();
}
