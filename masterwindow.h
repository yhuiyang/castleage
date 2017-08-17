#ifndef MASTERWINDOW_H
#define MASTERWINDOW_H

#include <QMainWindow>

class QTabWidget;

class MasterWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MasterWindow(QWidget *parent = nullptr);
    ~MasterWindow();
    QSize sizeHint() const override;

protected:
    void closeEvent(QCloseEvent *event) override;

signals:

public slots:

private:
    QToolBar *createToolBar();

private:
    QTabWidget *mTabWidget;
};

#endif // MASTERWINDOW_H
