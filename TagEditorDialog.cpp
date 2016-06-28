#include <QtSql>
#include <QCheckBox>
#include <QSpacerItem>
#include "TagEditorDialog.h"
#include "ui_TagEditorDialog.h"

TagEditorDialog::TagEditorDialog(const bool isAccountId, const int id, QWidget *parent) :
    QDialog(parent),
    _isAccountId(isAccountId),
    _id(id),
    ui(new Ui::TagEditorDialog)
{
    _dirty = false;
    ui->setupUi(this);
    updateWindowTitle();
    populate();
}

TagEditorDialog::~TagEditorDialog()
{
    delete ui;
}

void TagEditorDialog::onTagChanged(int state)
{
    Q_UNUSED(state);
    _dirty = true;
}

void TagEditorDialog::reject()
{
    /* this method is called when user clicks 'X' button or presses 'ESC' key to close the dialog. */

    int tagsUpdated = 0;
    int dataId;

    if (_dirty) {
        QSqlDatabase::database().transaction();
        QSqlQuery sql;
        /* remove old mapping */
        if (_isAccountId)
            sql.exec("DELETE FROM account_tag_mapping WHERE accountId = " + QString::number(_id));
        else
            sql.exec("DELETE FROM account_tag_mapping WHERE tagId = " + QString::number(_id));
        tagsUpdated++;

        /* add new mapping */
        sql.prepare("INSERT INTO account_tag_mapping VALUES (:accountId, :tagId)");
        QObjectList children = ui->scrollAreaWidgetContents->children();
        for (QObject *child : children) {
            QCheckBox *checkBox = qobject_cast<QCheckBox *>(child);
            if (checkBox != nullptr && checkBox->isChecked()) {
                dataId = checkBox->property("dataId").toInt();
                sql.bindValue(":accountId", _isAccountId ? _id : dataId);
                sql.bindValue(":tagId", _isAccountId ? dataId : _id);
                sql.exec();
                tagsUpdated++;
            }
        }
        QSqlDatabase::database().commit();
    }

    /* setup the result code. So, the caller can check the code return by QDialog::exec() */
    done(tagsUpdated);
}

void TagEditorDialog::populate()
{
    QSqlQuery sql;
    if (_isAccountId) {
        sql.prepare("SELECT t.id, t.name, (t.id IN (SELECT m.tagId FROM account_tag_mapping AS m WHERE m.accountId = :accountId)) FROM tags AS t");
        sql.bindValue(":accountId", _id);
    } else {
        sql.prepare("SELECT a.id, ifnull(i.ign, 'UnknownIgn') || ' - ' || a.email, (a.id IN (SELECT m.accountId FROM account_tag_mapping AS m WHERE m.tagId = :tagId)) FROM accounts AS a "
                    "LEFT JOIN igns AS i ON a.id = i.accountId");
        sql.bindValue(":tagId", _id);
    }

    int dataId;
    QString displayName;
    bool checked;
    QCheckBox *checkBox;
    bool ok = sql.exec();
    if (ok) {
        while (sql.next()) {
            dataId = sql.value(0).toInt();
            displayName = sql.value(1).toString();
            checked = sql.value(2).toBool();

            checkBox = new QCheckBox(displayName, ui->scrollAreaWidgetContents);
            checkBox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
            checkBox->setProperty("dataId", dataId); // accountId or tagId
            ui->scrollAreaWidgetsLayout->addWidget(checkBox);

            connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(onTagChanged(int)));
        }
        /* add spacer to occupy extra space */
        ui->scrollAreaWidgetsLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding));
    }
}

void TagEditorDialog::updateWindowTitle()
{
    QSqlQuery sql;

    if (_isAccountId) {
        sql.prepare("SELECT ifnull(i.ign, 'UnknonwIgn') || ' - ' || a.email FROM accounts AS a "
                    "LEFT JOIN igns AS i ON a.id = i.accountId "
                    "WHERE a.id = :accountId");
        sql.bindValue(":accountId", _id);
        if (sql.exec() && sql.next())
            this->setWindowTitle("Tags owned by " + sql.value(0).toString());
    } else {
        sql.prepare("SELECT name FROM tags WHERE id = :tagId");
        sql.bindValue(":tagId", _id);
        if (sql.exec() && sql.next())
            this->setWindowTitle("Accounts own this tag: " + sql.value(0).toString());
    }
}
