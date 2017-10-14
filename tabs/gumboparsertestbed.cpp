#include <QtWidgets>
#include <QtSql>
#include "gumboparsertestbed.h"
#include "ui_gumboparsertestbed.h"
#include "castleagehttpclient.h"
#include "gumbo.h"
#include "error.h"


// -----------------------------------------------------------------------------
class ComboBoxAccountModel : public QSqlQueryModel {
public:
    ComboBoxAccountModel(QObject *parent = Q_NULLPTR)
        : QSqlQueryModel(parent) {
        QString sql;
        sql.append("SELECT _id, IFNULL(i.ign, 'UnknownIGN') || ' - ' || a.email FROM accounts AS a ");
        sql.append("LEFT JOIN igns AS i ON i.accountId = a._id ");
        sql.append("ORDER BY a.sequence");
        this->setQuery(sql);
    }

    ~ComboBoxAccountModel() {}
};

// -----------------------------------------------------------------------------
GumboParserTestbed::GumboParserTestbed(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::GumboParserTestbed)
{
    ui->setupUi(this);

    ui->toolBar->addWidget(new QLabel("Account", ui->toolBar));
    ui->toolBar->addWidget(mComboBoxAccounts = new QComboBox(ui->toolBar));
    mComboBoxAccounts->setModel(new ComboBoxAccountModel(mComboBoxAccounts));
    mComboBoxAccounts->setModelColumn(1);
}

GumboParserTestbed::~GumboParserTestbed()
{
    delete ui;
}

void GumboParserTestbed::on_actionGET_triggered()
{
    QSqlQueryModel *m = static_cast<QSqlQueryModel *>(mComboBoxAccounts->model());
    int accountId = m->data(m->index(mComboBoxAccounts->currentIndex(), 0)).toInt();

    QString php = ui->lineEditPHP->text();
    if (php.isEmpty() || !php.endsWith(".php")) {
        qWarning() << "Invalid php file name.";
        return;
    }

    QString qs_user = ui->lineEditQS->text();
    QVector<QPair<QString, QString>> qs_gen;
    if (!qs_user.isEmpty()) {
        QStringList pairs = qs_user.split('&');
        for (QString pair : pairs) {
            QStringList kv = pair.split('=');
            if (kv.length() == 2) {
                qs_gen.push_back(QPair<QString, QString>(kv.at(0), kv.at(1)));
            }
        }
    }

    CastleAgeHttpClient client(accountId);
    QByteArray response = client.get_sync(php, qs_gen);
    if (response.isEmpty()) {
        qWarning() << "Empty response...";
        return;
    }

    /* update QPlainTextEdit */
    ui->plainTextEdit->clear();
    ui->plainTextEdit->appendPlainText(QString(response));

    /* parsed by Gumbo */
    GumboOutput *output = gumbo_parse(response.data());
    populateTreeView(output);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
}

void GumboParserTestbed::on_actionPOST_triggered()
{
    QSqlQueryModel *m = static_cast<QSqlQueryModel *>(mComboBoxAccounts->model());
    int accountId = m->data(m->index(mComboBoxAccounts->currentIndex(), 0)).toInt();

    QString php = ui->lineEditPHP->text();
    if (php.isEmpty() || !php.endsWith(".php")) {
        qWarning() << "Invalid php file name.";
        return;
    }

    QString qs_user = ui->lineEditQS->text();
    QVector<QPair<QString, QString>> qs_gen;
    if (!qs_user.isEmpty()) {
        QStringList pairs = qs_user.split('&');
        for (QString pair : pairs) {
            QStringList kv = pair.split('=');
            if (kv.length() == 2) {
                qs_gen.push_back(QPair<QString, QString>(kv.at(0), kv.at(1)));
            }
        }
    }

    QString payload_user = ui->lineEditPayload->text();
    QVector<QPair<QString, QString>> payload_gen;
    if (!payload_user.isEmpty()) {
        QStringList pairs = payload_user.split('&');
        for (QString pair : pairs) {
            QStringList kv = pair.split('=');
            if (kv.length() == 2) {
                payload_gen.push_back(QPair<QString, QString>(kv.at(0), kv.at(1)));
            }
        }
    }

    CastleAgeHttpClient client(accountId);
    QByteArray response = client.post_sync(php, payload_gen, qs_gen);
    if (response.isEmpty()) {
        qWarning() << "Empty response...";
        return;
    }

    /* update QPlainTextEdit */
    ui->plainTextEdit->clear();
    ui->plainTextEdit->appendPlainText(QString(response));

    /* parsed by Gumbo */
    GumboOutput *output = gumbo_parse(response.data());
    populateTreeView(output);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
}

void GumboParserTestbed::populateTreeView(const GumboOutput *output)
{
    /* discard old model */
    QAbstractItemModel *old = ui->treeView->model();
    if (old != nullptr)
        old->deleteLater();

    /* model & header */
    QStandardItemModel *model = new QStandardItemModel(ui->treeView);
    QStringList headerLabels;
    headerLabels.append("GumboOutput");
    headerLabels.append("Description");
    model->setHorizontalHeaderLabels(headerLabels);

    // output->document
    QStandardItem *parentDocument = new QStandardItem("document");
    model->appendRow({parentDocument, new QStandardItem(dumpGumboNode(output->document))});
//    parentDocument->appendRow({new QStandardItem("Children size"), new QStandardItem(QString("%1").arg(output->document->v.document.children.length))});
//    parentDocument->appendRow({new QStandardItem("Has doctype"), new QStandardItem(output->document->v.document.has_doctype ? "Yes" : "No")});
//    parentDocument->appendRow({new QStandardItem("Name"), new QStandardItem(QString::fromUtf8(output->document->v.document.name))});
//    parentDocument->appendRow({new QStandardItem("Public identifier"), new QStandardItem(QString::fromUtf8(output->document->v.document.public_identifier))});
//    parentDocument->appendRow({new QStandardItem("System identifier"), new QStandardItem(QString::fromUtf8(output->document->v.document.system_identifier))});
    setupTreeNodeFromGumboNode(parentDocument, output->document);

    // output->error
    QStandardItem *parentError = new QStandardItem("error");
    model->appendRow({parentError, new QStandardItem(QString("%1 errors").arg(output->errors.length))});
    for (unsigned int i = 0; i < output->errors.length; i++) {
        GumboError *e = static_cast<GumboError *>(output->errors.data[i]);
        QStandardItem *errorParent = new QStandardItem(QString("error[%1]").arg(i));
        parentError->appendRow({errorParent, new QStandardItem(dumpGumboErrorType(static_cast<GumboError *>(output->errors.data[i])))});
        errorParent->appendRow({new QStandardItem("line"), new QStandardItem(QString("%1").arg(e->position.line))});
        errorParent->appendRow({new QStandardItem("column"), new QStandardItem(QString("%1").arg(e->position.column))});
        errorParent->appendRow({new QStandardItem("offset"), new QStandardItem(QString("%1").arg(e->position.offset))});
    }

    // output->root
    QStandardItem *parentRoot = new QStandardItem("root");
    model->appendRow({parentRoot, new QStandardItem(dumpGumboNode(output->root))});
    setupTreeNodeFromGumboNode(parentRoot, output->root);

    ui->treeView->setModel(model);
    ui->treeView->resizeColumnToContents(1);
    //ui->treeView->expandAll();
}

void GumboParserTestbed::setupTreeNodeFromGumboNode(QStandardItem *treeNode, const GumboNode *gumboNode)
{
    if (!treeNode || !gumboNode)
        return;

    QStandardItem *childrenNode, *childNode;
    GumboVector children;

    switch (gumboNode->type) {
    case GUMBO_NODE_DOCUMENT:
        treeNode->appendRow({new QStandardItem("Has doctype"), new QStandardItem(gumboNode->v.document.has_doctype ? "Yes" : "No")});
        treeNode->appendRow({new QStandardItem("Name"), new QStandardItem(QString::fromUtf8(gumboNode->v.document.name))});
        treeNode->appendRow({new QStandardItem("Public identifier"), new QStandardItem(QString::fromUtf8(gumboNode->v.document.public_identifier))});
        treeNode->appendRow({new QStandardItem("System identifier"), new QStandardItem(QString::fromUtf8(gumboNode->v.document.system_identifier))});
        childrenNode = new QStandardItem("Children");
        children = gumboNode->v.document.children;
        treeNode->appendRow({childrenNode, new QStandardItem(QString("%1").arg(children.length))});
        for (unsigned int i = 0; i < children.length; i++) {
            childNode = new QStandardItem(QString("[%1]").arg(i));
            childrenNode->appendRow({childNode, new QStandardItem(dumpGumboNode(static_cast<GumboNode *>(children.data[i])))});
            setupTreeNodeFromGumboNode(childNode, static_cast<GumboNode *>(children.data[i]));
        }
        break;
    case GUMBO_NODE_ELEMENT:
    case GUMBO_NODE_TEMPLATE:
        //treeNode->appendRow({new QStandardItem("original tag"), new QStandardItem(QString::fromUtf8(gumboNode->v.element.original_tag.data))});
        //treeNode->appendRow({new QStandardItem("original end tag"), new QStandardItem(QString::fromUtf8(gumboNode->v.element.original_end_tag.data))});
        treeNode->appendRow({new QStandardItem("tag"), new QStandardItem(GumboTagText(gumboNode->v.element.tag))});
        //treeNode->appendRow({new QStandardItem("start position"),
        //                     new QStandardItem(QString("line %1, column %2, offset %3")
        //                     .arg(gumboNode->v.element.start_pos.line)
        //                     .arg(gumboNode->v.element.start_pos.column)
        //                     .arg(gumboNode->v.element.start_pos.offset))});
        //treeNode->appendRow({new QStandardItem("end position"),
        //                     new QStandardItem(QString("line %1, column %2, offset %3")
        //                     .arg(gumboNode->v.element.end_pos.line)
        //                     .arg(gumboNode->v.element.end_pos.column)
        //                     .arg(gumboNode->v.element.end_pos.offset))});
        childrenNode = new QStandardItem("Attributes");
        children = gumboNode->v.element.attributes;
        treeNode->appendRow({childrenNode, new QStandardItem(QString("%1").arg(children.length))});
        for (unsigned int i = 0; i < children.length; i++) {
            GumboAttribute * attr = static_cast<GumboAttribute *>(children.data[i]);
            childrenNode->appendRow({new QStandardItem(QString::fromUtf8(attr->name)), new QStandardItem(QString::fromUtf8(attr->value))});
        }
        childrenNode = new QStandardItem("Children");
        children = gumboNode->v.element.children;
        treeNode->appendRow({childrenNode, new QStandardItem(QString("%1").arg(children.length))});
        for (unsigned int i = 0; i < children.length; i++) {
            childNode = new QStandardItem(QString("[%1]").arg(i));
            childrenNode->appendRow({childNode, new QStandardItem(dumpGumboNode(static_cast<GumboNode *>(children.data[i])))});
            setupTreeNodeFromGumboNode(childNode, static_cast<GumboNode *>(children.data[i]));
        }
        break;
    case GUMBO_NODE_TEXT:
    case GUMBO_NODE_CDATA:
    case GUMBO_NODE_COMMENT:
    case GUMBO_NODE_WHITESPACE:
        treeNode->appendRow({new QStandardItem("start position"),
                             new QStandardItem(QString("line %1, column %2, offset %3")
                             .arg(gumboNode->v.text.start_pos.line)
                             .arg(gumboNode->v.text.start_pos.column)
                             .arg(gumboNode->v.text.start_pos.offset))});
        //treeNode->appendRow({new QStandardItem("original text"), new QStandardItem(QString::fromUtf8(gumboNode->v.text.original_text.data))});
        treeNode->appendRow({new QStandardItem("text"), new QStandardItem(QString::fromUtf8(gumboNode->v.text.text))});
        break;
    default:
        return;
    }
}

QString GumboParserTestbed::dumpGumboNode(const GumboNode *node)
{
    QString dump;

    if (!node)
        return dump;

    switch (node->type) {
    case GUMBO_NODE_DOCUMENT: /** Document node.  v will be a GumboDocument. */
        dump.append("Document");
        break;
    case GUMBO_NODE_ELEMENT: /** Element node.  v will be a GumboElement. */
        dump.append("Element");
        break;
    case GUMBO_NODE_TEXT: /** Text node.  v will be a GumboText. */
        dump.append("Text");
        break;
    case GUMBO_NODE_CDATA: /** CDATA node. v will be a GumboText. */
        dump.append("CDATA");
        break;
    case GUMBO_NODE_COMMENT: /** Comment node.  v will be a GumboText, excluding comment delimiters. */
        dump.append("Comment");
        break;
    case GUMBO_NODE_WHITESPACE: /** Text node, where all contents is whitespace.  v will be a GumboText. */
        dump.append("Whitespace");
        break;
    case GUMBO_NODE_TEMPLATE:
        /** Template node.  This is separate from GUMBO_NODE_ELEMENT because many
         * client libraries will want to ignore the contents of template nodes, as
         * the spec suggests.  Recursing on GUMBO_NODE_ELEMENT will do the right thing
         * here, while clients that want to include template contents should also
         * check for GUMBO_NODE_TEMPLATE.  v will be a GumboElement.  */
        dump.append("Template");
        break;
    default:
        dump.append("???");
        break;
    }

    return dump;
}

QString GumboParserTestbed::dumpGumboErrorType(const GumboError *error)
{
    if (!error)
        return "";
    switch (error->type) {
    case GUMBO_ERR_UTF8_INVALID:
        return "GUMBO_ERR_UTF8_INVALID";
    case GUMBO_ERR_UTF8_TRUNCATED:
        return "GUMBO_ERR_UTF8_TRUNCATED";
    case GUMBO_ERR_UTF8_NULL:
        return "GUMBO_ERR_UTF8_NULL";
    case GUMBO_ERR_NUMERIC_CHAR_REF_NO_DIGITS:
        return "GUMBO_ERR_NUMERIC_CHAR_REF_NO_DIGITS";
    case GUMBO_ERR_NUMERIC_CHAR_REF_WITHOUT_SEMICOLON:
        return "GUMBO_ERR_NUMERIC_CHAR_REF_WITHOUT_SEMICOLON";
    case GUMBO_ERR_NUMERIC_CHAR_REF_INVALID:
        return "GUMBO_ERR_NUMERIC_CHAR_REF_INVALID";
    case GUMBO_ERR_NAMED_CHAR_REF_WITHOUT_SEMICOLON:
        return "GUMBO_ERR_NAMED_CHAR_REF_WITHOUT_SEMICOLON";
    case GUMBO_ERR_NAMED_CHAR_REF_INVALID:
        return "GUMBO_ERR_NAMED_CHAR_REF_INVALID";
    case GUMBO_ERR_TAG_STARTS_WITH_QUESTION:
        return "GUMBO_ERR_TAG_STARTS_WITH_QUESTION";
    case GUMBO_ERR_TAG_EOF:
        return "GUMBO_ERR_TAG_EOF";
    case GUMBO_ERR_TAG_INVALID:
        return "GUMBO_ERR_TAG_INVALID";
    case GUMBO_ERR_CLOSE_TAG_EMPTY:
        return "GUMBO_ERR_CLOSE_TAG_EMPTY";
    case GUMBO_ERR_CLOSE_TAG_EOF:
        return "GUMBO_ERR_CLOSE_TAG_EOF";
    case GUMBO_ERR_CLOSE_TAG_INVALID:
        return "GUMBO_ERR_CLOSE_TAG_INVALID";
    case GUMBO_ERR_SCRIPT_EOF:
        return "GUMBO_ERR_SCRIPT_EOF";
    case GUMBO_ERR_ATTR_NAME_EOF:
        return "GUMBO_ERR_ATTR_NAME_EOF";
    case GUMBO_ERR_ATTR_NAME_INVALID:
        return "GUMBO_ERR_ATTR_NAME_INVALID";
    case GUMBO_ERR_ATTR_DOUBLE_QUOTE_EOF:
        return "GUMBO_ERR_ATTR_DOUBLE_QUOTE_EOF";
    case GUMBO_ERR_ATTR_SINGLE_QUOTE_EOF:
        return "GUMBO_ERR_ATTR_SINGLE_QUOTE_EOF";
    case GUMBO_ERR_ATTR_UNQUOTED_EOF:
        return "GUMBO_ERR_ATTR_UNQUOTED_EOF";
    case GUMBO_ERR_ATTR_UNQUOTED_RIGHT_BRACKET:
        return "GUMBO_ERR_ATTR_UNQUOTED_RIGHT_BRACKET";
    case GUMBO_ERR_ATTR_UNQUOTED_EQUALS:
        return "GUMBO_ERR_ATTR_UNQUOTED_EQUALS";
    case GUMBO_ERR_ATTR_AFTER_EOF:
        return "GUMBO_ERR_ATTR_AFTER_EOF";
    case GUMBO_ERR_ATTR_AFTER_INVALID:
        return "GUMBO_ERR_ATTR_AFTER_INVALID";
    case GUMBO_ERR_DUPLICATE_ATTR:
        return "GUMBO_ERR_DUPLICATE_ATTR";
    case GUMBO_ERR_SOLIDUS_EOF:
        return "GUMBO_ERR_SOLIDUS_EOF";
    case GUMBO_ERR_SOLIDUS_INVALID:
        return "GUMBO_ERR_SOLIDUS_INVALID";
    case GUMBO_ERR_DASHES_OR_DOCTYPE:
        return "GUMBO_ERR_DASHES_OR_DOCTYPE";
    case GUMBO_ERR_COMMENT_EOF:
        return "GUMBO_ERR_COMMENT_EOF";
    case GUMBO_ERR_COMMENT_INVALID:
        return "GUMBO_ERR_COMMENT_INVALID";
    case GUMBO_ERR_COMMENT_BANG_AFTER_DOUBLE_DASH:
        return "GUMBO_ERR_COMMENT_BANG_AFTER_DOUBLE_DASH";
    case GUMBO_ERR_COMMENT_DASH_AFTER_DOUBLE_DASH:
        return "GUMBO_ERR_COMMENT_DASH_AFTER_DOUBLE_DASH";
    case GUMBO_ERR_COMMENT_SPACE_AFTER_DOUBLE_DASH:
        return "GUMBO_ERR_COMMENT_SPACE_AFTER_DOUBLE_DASH";
    case GUMBO_ERR_COMMENT_END_BANG_EOF:
        return "GUMBO_ERR_COMMENT_END_BANG_EOF";
    case GUMBO_ERR_DOCTYPE_EOF:
        return "GUMBO_ERR_DOCTYPE_EOF";
    case GUMBO_ERR_DOCTYPE_INVALID:
        return "GUMBO_ERR_DOCTYPE_INVALID";
    case GUMBO_ERR_DOCTYPE_SPACE:
        return "GUMBO_ERR_DOCTYPE_SPACE";
    case GUMBO_ERR_DOCTYPE_RIGHT_BRACKET:
        return "GUMBO_ERR_DOCTYPE_RIGHT_BRACKET";
    case GUMBO_ERR_DOCTYPE_SPACE_OR_RIGHT_BRACKET:
        return "GUMBO_ERR_DOCTYPE_SPACE_OR_RIGHT_BRACKET";
    case GUMBO_ERR_DOCTYPE_END:
        return "GUMBO_ERR_DOCTYPE_END";
    case GUMBO_ERR_PARSER:
        return "GUMBO_ERR_PARSER";
    case GUMBO_ERR_UNACKNOWLEDGED_SELF_CLOSING_TAG:
        return "GUMBO_ERR_UNACKNOWLEDGED_SELF_CLOSING_TAG";
    default:
        return "GUMBO_ERR_??????";
    }
}

QString GumboParserTestbed::GumboTagText(const GumboTag tag)
{
    switch (tag) {
    case GUMBO_TAG_HTML: return "GUMBO_TAG_HTML";
    case GUMBO_TAG_HEAD: return "GUMBO_TAG_HEAD";
    case GUMBO_TAG_TITLE: return "GUMBO_TAG_TITLE";
    case GUMBO_TAG_BASE: return "GUMBO_TAG_BASE";
    case GUMBO_TAG_LINK: return "GUMBO_TAG_LINK";
    case GUMBO_TAG_META: return "GUMBO_TAG_META";
    case GUMBO_TAG_STYLE: return "GUMBO_TAG_STYLE";
    case GUMBO_TAG_SCRIPT: return "GUMBO_TAG_SCRIPT";
    case GUMBO_TAG_NOSCRIPT: return "GUMBO_TAG_NOSCRIPT";
    case GUMBO_TAG_TEMPLATE: return "GUMBO_TAG_TEMPLATE";
    case GUMBO_TAG_BODY: return "GUMBO_TAG_BODY";
    case GUMBO_TAG_ARTICLE: return "GUMBO_TAG_ARTICLE";
    case GUMBO_TAG_SECTION: return "GUMBO_TAG_SECTION";
    case GUMBO_TAG_NAV: return "GUMBO_TAG_NAV";
    case GUMBO_TAG_ASIDE: return "GUMBO_TAG_ASIDE";
    case GUMBO_TAG_H1: return "GUMBO_TAG_H1";
    case GUMBO_TAG_H2: return "GUMBO_TAG_H2";
    case GUMBO_TAG_H3: return "GUMBO_TAG_H3";
    case GUMBO_TAG_H4: return "GUMBO_TAG_H4";
    case GUMBO_TAG_H5: return "GUMBO_TAG_H5";
    case GUMBO_TAG_H6: return "GUMBO_TAG_H6";
    case GUMBO_TAG_HGROUP: return "GUMBO_TAG_HGROUP";
    case GUMBO_TAG_HEADER: return "GUMBO_TAG_HEADER";
    case GUMBO_TAG_FOOTER: return "GUMBO_TAG_FOOTER";
    case GUMBO_TAG_ADDRESS: return "GUMBO_TAG_ADDRESS";
    case GUMBO_TAG_P: return "GUMBO_TAG_P";
    case GUMBO_TAG_HR: return "GUMBO_TAG_HR";
    case GUMBO_TAG_PRE: return "GUMBO_TAG_PRE";
    case GUMBO_TAG_BLOCKQUOTE: return "GUMBO_TAG_BLOCKQUOTE";
    case GUMBO_TAG_OL: return "GUMBO_TAG_OL";
    case GUMBO_TAG_UL: return "GUMBO_TAG_UL";
    case GUMBO_TAG_LI: return "GUMBO_TAG_LI";
    case GUMBO_TAG_DL: return "GUMBO_TAG_DL";
    case GUMBO_TAG_DT: return "GUMBO_TAG_DT";
    case GUMBO_TAG_DD: return "GUMBO_TAG_DD";
    case GUMBO_TAG_FIGURE: return "GUMBO_TAG_FIGURE";
    case GUMBO_TAG_FIGCAPTION: return "GUMBO_TAG_FIGCAPTION";
    case GUMBO_TAG_MAIN: return "GUMBO_TAG_MAIN";
    case GUMBO_TAG_DIV: return "GUMBO_TAG_DIV";
    case GUMBO_TAG_A: return "GUMBO_TAG_A";
    case GUMBO_TAG_EM: return "GUMBO_TAG_EM";
    case GUMBO_TAG_STRONG: return "GUMBO_TAG_STRONG";
    case GUMBO_TAG_SMALL: return "GUMBO_TAG_SMALL";
    case GUMBO_TAG_S: return "GUMBO_TAG_S";
    case GUMBO_TAG_CITE: return "GUMBO_TAG_CITE";
    case GUMBO_TAG_Q: return "GUMBO_TAG_Q";
    case GUMBO_TAG_DFN: return "GUMBO_TAG_DFN";
    case GUMBO_TAG_ABBR: return "GUMBO_TAG_ABBR";
    case GUMBO_TAG_DATA: return "GUMBO_TAG_DATA";
    case GUMBO_TAG_TIME: return "GUMBO_TAG_TIME";
    case GUMBO_TAG_CODE: return "GUMBO_TAG_CODE";
    case GUMBO_TAG_VAR: return "GUMBO_TAG_VAR";
    case GUMBO_TAG_SAMP: return "GUMBO_TAG_SAMP";
    case GUMBO_TAG_KBD: return "GUMBO_TAG_KBD";
    case GUMBO_TAG_SUB: return "GUMBO_TAG_SUB";
    case GUMBO_TAG_SUP: return "GUMBO_TAG_SUP";
    case GUMBO_TAG_I: return "GUMBO_TAG_I";
    case GUMBO_TAG_B: return "GUMBO_TAG_B";
    case GUMBO_TAG_U: return "GUMBO_TAG_U";
    case GUMBO_TAG_MARK: return "GUMBO_TAG_MARK";
    case GUMBO_TAG_RUBY: return "GUMBO_TAG_RUBY";
    case GUMBO_TAG_RT: return "GUMBO_TAG_RT";
    case GUMBO_TAG_RP: return "GUMBO_TAG_RP";
    case GUMBO_TAG_BDI: return "GUMBO_TAG_BDI";
    case GUMBO_TAG_BDO: return "GUMBO_TAG_BDO";
    case GUMBO_TAG_SPAN: return "GUMBO_TAG_SPAN";
    case GUMBO_TAG_BR: return "GUMBO_TAG_BR";
    case GUMBO_TAG_WBR: return "GUMBO_TAG_WBR";
    case GUMBO_TAG_INS: return "GUMBO_TAG_INS";
    case GUMBO_TAG_DEL: return "GUMBO_TAG_DEL";
    case GUMBO_TAG_IMAGE: return "GUMBO_TAG_IMAGE";
    case GUMBO_TAG_IMG: return "GUMBO_TAG_IMG";
    case GUMBO_TAG_IFRAME: return "GUMBO_TAG_IFRAME";
    case GUMBO_TAG_EMBED: return "GUMBO_TAG_EMBED";
    case GUMBO_TAG_OBJECT: return "GUMBO_TAG_OBJECT";
    case GUMBO_TAG_PARAM: return "GUMBO_TAG_PARAM";
    case GUMBO_TAG_VIDEO: return "GUMBO_TAG_VIDEO";
    case GUMBO_TAG_AUDIO: return "GUMBO_TAG_AUDIO";
    case GUMBO_TAG_SOURCE: return "GUMBO_TAG_SOURCE";
    case GUMBO_TAG_TRACK: return "GUMBO_TAG_TRACK";
    case GUMBO_TAG_CANVAS: return "GUMBO_TAG_CANVAS";
    case GUMBO_TAG_MAP: return "GUMBO_TAG_MAP";
    case GUMBO_TAG_AREA: return "GUMBO_TAG_AREA";
    case GUMBO_TAG_MATH: return "GUMBO_TAG_MATH";
    case GUMBO_TAG_MI: return "GUMBO_TAG_MI";
    case GUMBO_TAG_MO: return "GUMBO_TAG_MO";
    case GUMBO_TAG_MN: return "GUMBO_TAG_MN";
    case GUMBO_TAG_MS: return "GUMBO_TAG_MS";
    case GUMBO_TAG_MTEXT: return "GUMBO_TAG_MTEXT";
    case GUMBO_TAG_MGLYPH: return "GUMBO_TAG_MGLYPH";
    case GUMBO_TAG_MALIGNMARK: return "GUMBO_TAG_MALIGNMARK";
    case GUMBO_TAG_ANNOTATION_XML: return "GUMBO_TAG_ANNOTATION_XML";
    case GUMBO_TAG_SVG: return "GUMBO_TAG_SVG";
    case GUMBO_TAG_FOREIGNOBJECT: return "GUMBO_TAG_FOREIGNOBJECT";
    case GUMBO_TAG_DESC: return "GUMBO_TAG_DESC";
    case GUMBO_TAG_TABLE: return "GUMBO_TAG_TABLE";
    case GUMBO_TAG_CAPTION: return "GUMBO_TAG_CAPTION";
    case GUMBO_TAG_COLGROUP: return "GUMBO_TAG_COLGROUP";
    case GUMBO_TAG_COL: return "GUMBO_TAG_COL";
    case GUMBO_TAG_TBODY: return "GUMBO_TAG_TBODY";
    case GUMBO_TAG_THEAD: return "GUMBO_TAG_THEAD";
    case GUMBO_TAG_TFOOT: return "GUMBO_TAG_TFOOT";
    case GUMBO_TAG_TR: return "GUMBO_TAG_TR";
    case GUMBO_TAG_TD: return "GUMBO_TAG_TD";
    case GUMBO_TAG_TH: return "GUMBO_TAG_TH";
    case GUMBO_TAG_FORM: return "GUMBO_TAG_FORM";
    case GUMBO_TAG_FIELDSET: return "GUMBO_TAG_FIELDSET";
    case GUMBO_TAG_LEGEND: return "GUMBO_TAG_LEGEND";
    case GUMBO_TAG_LABEL: return "GUMBO_TAG_LABEL";
    case GUMBO_TAG_INPUT: return "GUMBO_TAG_INPUT";
    case GUMBO_TAG_BUTTON: return "GUMBO_TAG_BUTTON";
    case GUMBO_TAG_SELECT: return "GUMBO_TAG_SELECT";
    case GUMBO_TAG_DATALIST: return "GUMBO_TAG_DATALIST";
    case GUMBO_TAG_OPTGROUP: return "GUMBO_TAG_OPTGROUP";
    case GUMBO_TAG_OPTION: return "GUMBO_TAG_OPTION";
    case GUMBO_TAG_TEXTAREA: return "GUMBO_TAG_TEXTAREA";
    case GUMBO_TAG_KEYGEN: return "GUMBO_TAG_KEYGEN";
    case GUMBO_TAG_OUTPUT: return "GUMBO_TAG_OUTPUT";
    case GUMBO_TAG_PROGRESS: return "GUMBO_TAG_PROGRESS";
    case GUMBO_TAG_METER: return "GUMBO_TAG_METER";
    case GUMBO_TAG_DETAILS: return "GUMBO_TAG_DETAILS";
    case GUMBO_TAG_SUMMARY: return "GUMBO_TAG_SUMMARY";
    case GUMBO_TAG_MENU: return "GUMBO_TAG_MENU";
    case GUMBO_TAG_MENUITEM: return "GUMBO_TAG_MENUITEM";
    case GUMBO_TAG_APPLET: return "GUMBO_TAG_APPLET";
    case GUMBO_TAG_ACRONYM: return "GUMBO_TAG_ACRONYM";
    case GUMBO_TAG_BGSOUND: return "GUMBO_TAG_BGSOUND";
    case GUMBO_TAG_DIR: return "GUMBO_TAG_DIR";
    case GUMBO_TAG_FRAME: return "GUMBO_TAG_FRAME";
    case GUMBO_TAG_FRAMESET: return "GUMBO_TAG_FRAMESET";
    case GUMBO_TAG_NOFRAMES: return "GUMBO_TAG_NOFRAMES";
    case GUMBO_TAG_ISINDEX: return "GUMBO_TAG_ISINDEX";
    case GUMBO_TAG_LISTING: return "GUMBO_TAG_LISTING";
    case GUMBO_TAG_XMP: return "GUMBO_TAG_XMP";
    case GUMBO_TAG_NEXTID: return "GUMBO_TAG_NEXTID";
    case GUMBO_TAG_NOEMBED: return "GUMBO_TAG_NOEMBED";
    case GUMBO_TAG_PLAINTEXT: return "GUMBO_TAG_PLAINTEXT";
    case GUMBO_TAG_RB: return "GUMBO_TAG_RB";
    case GUMBO_TAG_STRIKE: return "GUMBO_TAG_STRIKE";
    case GUMBO_TAG_BASEFONT: return "GUMBO_TAG_BASEFONT";
    case GUMBO_TAG_BIG: return "GUMBO_TAG_BIG";
    case GUMBO_TAG_BLINK: return "GUMBO_TAG_BLINK";
    case GUMBO_TAG_CENTER: return "GUMBO_TAG_CENTER";
    case GUMBO_TAG_FONT: return "GUMBO_TAG_FONT";
    case GUMBO_TAG_MARQUEE: return "GUMBO_TAG_MARQUEE";
    case GUMBO_TAG_MULTICOL: return "GUMBO_TAG_MULTICOL";
    case GUMBO_TAG_NOBR: return "GUMBO_TAG_NOBR";
    case GUMBO_TAG_SPACER: return "GUMBO_TAG_SPACER";
    case GUMBO_TAG_TT: return "GUMBO_TAG_TT";
    case GUMBO_TAG_RTC: return "GUMBO_TAG_RTC";
    case GUMBO_TAG_UNKNOWN: return "GUMBO_TAG_UNKNOWN";
    case GUMBO_TAG_LAST: return "GUMBO_TAG_LAST";
    default: return "<\?\?\?>";
    }
}
