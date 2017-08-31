#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include <QDialog>

namespace Ui {
class InputDialog;
}

class InputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InputDialog(QWidget *parent = 0, QString inputHint = QString(), QString inputLabel = QString());
    ~InputDialog();

    QString getInput();

private:
    Ui::InputDialog *ui;
};

#endif // INPUTDIALOG_H
