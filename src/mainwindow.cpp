// mainwindow.cpp
//

#include <QTimer>
#include <QMessageBox>


#include "creader.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    try {

        // Open connection to reader

        int verbose = 9;
        readerList.append(new CReader("192.168.36.210", verbose));
        for (int i=0; i<readerList.size(); i++) {
            connect(readerList[0], &CReader::newTag, this, &MainWindow::onNewTag);
            connect(readerList[0], &CReader::newLogMessage, this, &MainWindow::onNewLogMessage);
        }


        // Start timer that will poll reader

        connect(&readerCheckTimer, &QTimer::timeout, this, &MainWindow::onReaderCheckTimeout);
        readerCheckTimer.setInterval(500);
        readerCheckTimer.start();

    }
    catch (QString s) {
        QMessageBox::critical(this, "fcvtc", s);
    }
    catch (...) {
        QMessageBox::critical(this, "fcvtc", "Unexpected exception");
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    for (int i=0; i<readerList.size(); i++) {
        delete readerList[i];
    }
    readerList.clear();
}


void MainWindow::onReaderCheckTimeout(void) {
    readerCheckTimer.stop();
    for (int i=0; i<readerList.size(); i++) {
        readerList[i]->ProcessRecentChipsSeen();
    }
    readerCheckTimer.start();
}


void MainWindow::onNewTag(const CTagInfo& tagInfo) {
    printf("%d %llu: %02x %02x %02x %02x %02x %02x\n", tagInfo.AntennaId, tagInfo.getTimeStampUSec(), tagInfo.data[0], tagInfo.data[1], tagInfo.data[2], tagInfo.data[3], tagInfo.data[4], tagInfo.data[5]);
    fflush(stdout);
}


void MainWindow::onNewLogMessage(const QString& s) {
    printf("%s\n", s.toLatin1().data());
    fflush(stdout);
}
