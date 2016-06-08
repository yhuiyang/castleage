#ifndef IMPORTACCOUNTDIALOG_H
#define IMPORTACCOUNTDIALOG_H

#include <QDialog>

namespace Ui {
class ImportAccountDialog;
}

class ImportAccountDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportAccountDialog(QWidget *parent = 0);
    ~ImportAccountDialog();
    void getAccountData(QString &email, QString &password);

private:
    Ui::ImportAccountDialog *ui;
};

#endif // IMPORTACCOUNTDIALOG_H
