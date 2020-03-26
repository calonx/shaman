#pragma once

#include <QtWidgets\QWidget.h>

class QPushButton;
class QTextBrowser;

class MainWidget : public QWidget
{
	Q_OBJECT

public:
	explicit MainWidget(QWidget* parent = nullptr);
	~MainWidget();

private:
	QPushButton*	button_;
	QTextBrowser*	textBrowser_;
};
