#ifndef QGUMBOPARSER_H
#define QGUMBOPARSER_H

#include <QList>
#include "gumbo.h"

typedef enum {
    Exists,
    Equals,
    Contains,
    StartsWith,
    EndsWith
} AttrMatch;

typedef bool (*iterate_callback) (void *userdata, GumboNode *node);

class QGumboParser
{
public:
    QGumboParser(const char *utf8_buffer);
    QGumboParser(const GumboOptions *options, const char *utf8_buffer, size_t utf8_buffer_length);
    virtual ~QGumboParser();

    /* search node(s) */
    QList<GumboNode *> findNodes(GumboTag tag_name, const char *attr_name, const char *attr_value, AttrMatch attr_match = Equals);
    QList<GumboNode *> findNodes(GumboNode *node, const GumboTag tag_name, const char *attr_name, const char *attr_value, AttrMatch attr_match = Equals);
    GumboNode * findNode(GumboTag tag_name, const char *attr_name, const char *attr_value, AttrMatch attr_match = Equals);
    GumboNode * findNode(GumboNode *node, const GumboTag tag_name, const char *attr_name, const char *attr_value, AttrMatch attr_match = Equals);

    /* tree walking */
    bool iterateTree(GumboNode *node, iterate_callback callback, void *callback_userdata);

    /* node attribute query */
    const static char *attributeValue(const GumboNode *node, const char *name);

    /* node child query */
    const static GumboNode *getChildByTag(const GumboNode *node, const GumboTag tag, const int childIndex);

    /* node text */
    static QString textContent(const GumboNode *node);

private:
    static bool find_first_matched_node(void *user_data, GumboNode *node);
    static bool find_all_matched_nodes(void *user_data, GumboNode *node);

    /* string compare helpers */
    static bool equals(const char *_big, const char *_little);
    static bool startsWith(const char *_big, const char *_little);
    static bool contains(const char *_big, const char *_little);
    static bool endsWith(const char *_big, const char *_little);

private:
    GumboOutput *_gumbo_output;
};

#endif // QGUMBOPARSER_H
