
#include <QtWidgets\QGridLayout.h>
#include <QtWidgets\QPushButton.h>
#include <QtWidgets\QTextBrowser.h>
#include "MainWidget.h"

MainWidget::MainWidget(QWidget* parent) :
	QWidget(parent)
{
	button_ = new QPushButton(tr("Push Me!"));
	textBrowser_ = new QTextBrowser();

	QGridLayout* layout = new QGridLayout();
	layout->addWidget(button_, 0, 0);
	layout->addWidget(textBrowser_, 1, 0);
	setLayout(layout);
	setWindowTitle(tr("Initializing..."));
}

MainWidget::~MainWidget()
{
	delete button_;
	delete textBrowser_;
}