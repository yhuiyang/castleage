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

private:
    Ui::TagManager *ui;
};

#endif // TAGMANAGER_H
