#ifndef TAGMANAGER_H
#define TAGMANAGER_H

#include <QMainWindow>


namespace Ui {
class TagManager;
}

class TagManager : public QMainWindow
{
    Q_OBJECT

public:
    explicit TagManager(QWidget *parent = 0);
    ~TagManager();
    void handleTagCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    void refreshNotInSelectedTagAccountList();
    void refreshInSelectedTagAccountList();

private slots:
    void on_actionAddTag_triggered();

    void on_pushButtonAddTag_clicked();

    void on_pushButtonRemoveTag_clicked();

private:
    Ui::TagManager *ui;
    int mSelectedTagId;
};

#endif // TAGMANAGER_H
