#ifndef TAGEDITORDIALOG_H
#define TAGEDITORDIALOG_H

#include <QDialog>

namespace Ui {
class TagEditorDialog;
}

class TagEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TagEditorDialog(const bool isAccountId, const int id, QWidget *parent = 0);
    ~TagEditorDialog();

public slots:
    void onTagChanged(int state);
    virtual void reject();

private:
    void updateWindowTitle();
    void populate();

private:
    const bool _isAccountId;
    const int _id;
    bool _dirty;
    Ui::TagEditorDialog *ui;
};

#endif // TAGEDITORDIALOG_H
