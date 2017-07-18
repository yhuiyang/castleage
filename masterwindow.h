#ifndef MASTERWINDOW_H
#define MASTERWINDOW_H

#include <QMainWindow>

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
};

#endif // MASTERWINDOW_H
