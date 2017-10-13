#ifndef GUMBOPARSERTESTBED_H
#define GUMBOPARSERTESTBED_H

#include <QMainWindow>
#include "gumbo.h"
#include "error.h"

class QComboBox;
class QStandardItem;

namespace Ui {
class GumboParserTestbed;
}

class GumboParserTestbed : public QMainWindow
{
    Q_OBJECT

public:
    explicit GumboParserTestbed(QWidget *parent = 0);
    ~GumboParserTestbed();

private:
    void populateTreeView(const GumboOutput *output);
    QString dumpGumboNode(const GumboNode *node);
    QString dumpGumboErrorType(const GumboError *error);
    QString GumboTagText(const GumboTag tag);
    void setupTreeNodeFromGumboNode(QStandardItem *treeNode, const GumboNode *gumboNode);

private slots:
    void on_actionGET_triggered();

    void on_actionPOST_triggered();

private:
    Ui::GumboParserTestbed *ui;
    QComboBox *mComboBoxAccounts;
};

#endif // GUMBOPARSERTESTBED_H
