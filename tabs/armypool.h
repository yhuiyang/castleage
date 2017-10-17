#ifndef ARMYPOOL_H
#define ARMYPOOL_H

#include <QMainWindow>

namespace Ui {
class ArmyPool;
}

class ArmyPool : public QMainWindow
{
    Q_OBJECT

public:
    explicit ArmyPool(QWidget *parent = 0);
    ~ArmyPool();

private slots:
    void on_actionDownloadArmy_triggered();

    void on_actionUpdateIgnLvl_triggered();

private:
    Ui::ArmyPool *ui;
};

#endif // ARMYPOOL_H
