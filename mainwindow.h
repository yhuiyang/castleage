#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public Q_SLOTS:
    void onAddAccount();
    void onRemoveAccount(const QString& email);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
