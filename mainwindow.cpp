#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filesystemmodel.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	ui->_filesView->setUniformRowHeights(true);
	ui->_filesView->setModel(new FilesystemModel(this));
}

MainWindow::~MainWindow()
{
	delete ui;
}
