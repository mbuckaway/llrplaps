#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "creader.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    Ui::MainWindow *ui;
    QTimer readerCheckTimer;
    QList<LLRPLaps::CReader *> readerList;
private slots:
    void onReaderCheckTimeout(void);
    void onNewTag(const CTagInfo& tagInfo);
    void onNewLogMessage(const QString& message);
};

#endif // MAINWINDOW_H
