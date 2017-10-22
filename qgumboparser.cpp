#include "qgumboparser.h"

class FindParams
{
public:
    FindParams()
    {
        tag_name = GUMBO_TAG_UNKNOWN;
        attr_name = nullptr;
        attr_value = nullptr;
        attr_match = Equals;
    }
    GumboTag tag_name;
    const char *attr_name;
    const char *attr_value;
    AttrMatch attr_match;
    QList<GumboNode *> nodes_found;
};

// ------------------------------------------------------------------------------------------------------------
QGumboParser::QGumboParser(const char *utf8_buffer)
{
    _gumbo_output = gumbo_parse(utf8_buffer);
}

QGumboParser::QGumboParser(const GumboOptions *options, const char *utf8_buffer, size_t utf8_buffer_length)
{
    _gumbo_output = gumbo_parse_with_options(options, utf8_buffer, utf8_buffer_length);
}

QGumboParser::~QGumboParser()
{
    gumbo_destroy_output(&kGumboDefaultOptions, _gumbo_output);
}

QList<GumboNode *> QGumboParser::findNodes(GumboTag tag_name, const char *attr_name, const char *attr_value, AttrMatch attr_match)
{
    return findNodes(_gumbo_output->root, tag_name, attr_name, attr_value, attr_match);
}

QList<GumboNode *> QGumboParser::findNodes(GumboNode *node, const GumboTag tag_name, const char *attr_name, const char *attr_value, AttrMatch attr_match)
{
    FindParams params;
    params.tag_name = tag_name;
    params.attr_name = attr_name;
    params.attr_value = attr_value;
    params.attr_match = attr_match;
    iterateTree(node, &QGumboParser::find_all_matched_nodes, &params);
    return params.nodes_found;
}

GumboNode * QGumboParser::findNode(GumboTag tag_name, const char *attr_name, const char *attr_value, AttrMatch attr_match)
{
    return findNode(_gumbo_output->root, tag_name, attr_name, attr_value, attr_match);
}

GumboNode * QGumboParser::findNode(GumboNode *node, const GumboTag tag_name, const char *attr_name, const char *attr_value, AttrMatch attr_match)
{
    FindParams params;
    params.tag_name = tag_name;
    params.attr_name = attr_name;
    params.attr_value = attr_value;
    params.attr_match = attr_match;
    return iterateTree(node, &QGumboParser::find_first_matched_node, &params) ? params.nodes_found.at(0) : nullptr;
}

bool QGumboParser::iterateTree(GumboNode *node, iterate_callback callback, void *callback_userdata)
{
    if (!node)
        return false;

    if (node->type != GUMBO_NODE_ELEMENT && node->type != GUMBO_NODE_TEMPLATE)
        return false;

    if (callback(callback_userdata, node))
        return true;

    for (unsigned int i = 0; i < node->v.element.children.length; i++) {
        if (iterateTree(static_cast<GumboNode *>(node->v.element.children.data[i]), callback, callback_userdata))
            return true;
    }

    return false;
}

const char *QGumboParser::attributeValue(const GumboNode *node, const char *name)
{
    if (!node || node->type != GUMBO_NODE_ELEMENT || !name || !strlen(name))
        return nullptr;

    GumboAttribute *attr = gumbo_get_attribute(&node->v.element.attributes, name);
    if (attr)
        return attr->value;
    return nullptr;
}

const GumboNode *QGumboParser::getChildByTag(const GumboNode *node, const GumboTag tag, const int childIndex)
{
    if (!node || node->type != GUMBO_NODE_ELEMENT)
        return nullptr;

    GumboVector children = node->v.element.children;
    for (unsigned int idx = 0, found = 0; idx < children.length; idx++) {
        GumboNode *child = static_cast<GumboNode *>(children.data[idx]);
        if (child->type == GUMBO_NODE_ELEMENT && child->v.element.tag == tag) { // tag matched
            if (found == childIndex)
                return child;
            else
                found++;
        }
    }

    return nullptr;
}

QString QGumboParser::textContent(const GumboNode *node)
{
    QString contents;
    if (!node)
        return contents;

    switch (node->type) {
    case GUMBO_NODE_TEXT:
    case GUMBO_NODE_WHITESPACE:
        return QString::fromUtf8(node->v.text.text);
    case GUMBO_NODE_ELEMENT:
        for (unsigned int i = 0; i < node->v.element.children.length; i++) {
            QString ownText = textContent(static_cast<GumboNode *>(node->v.element.children.data[i]));
            contents.append(ownText);
        }
        break;
    default:
        break;;
    }

    return contents;
}

bool QGumboParser::find_first_matched_node(void *user_data, GumboNode *node)
{
    FindParams *params = (FindParams *) user_data;

    if (!node || node->type != GUMBO_NODE_ELEMENT)
        return false;

    GumboAttribute *attr;
    if (node->v.element.tag == params->tag_name && (attr = gumbo_get_attribute(&node->v.element.attributes, params->attr_name))) {
        if (params->attr_match == Exists) {
            params->nodes_found.append(node);
            return true;
        }
        if (params->attr_match == Equals && equals(attr->value, params->attr_value)) {
            params->nodes_found.append(node);
            return true;
        }
        if (params->attr_match == Contains && contains(attr->value, params->attr_value)) {
            params->nodes_found.append(node);
            return true;
        }
        if (params->attr_match == StartsWith && startsWith(attr->value, params->attr_value)) {
            params->nodes_found.append(node);
            return true;
        }
        if (params->attr_match == EndsWith && endsWith(attr->value, params->attr_value)) {
            params->nodes_found.append(node);
            return true;
        }
    }
    return false;
}

bool QGumboParser::find_all_matched_nodes(void *user_data, GumboNode *node)
{
    FindParams *params = (FindParams *) user_data;

    if (!node || node->type != GUMBO_NODE_ELEMENT)
        return false;

    GumboAttribute *attr;
    if (node->v.element.tag == params->tag_name && (attr = gumbo_get_attribute(&node->v.element.attributes, params->attr_name))) {
        switch (params->attr_match) {
        case Exists:
            params->nodes_found.append(node);
            break;
        case Equals:
            if (equals(attr->value, params->attr_value))
                params->nodes_found.append(node);
            break;
        case Contains:
            if (contains(attr->value, params->attr_value))
                params->nodes_found.append(node);
            break;
        case StartsWith:
            if (startsWith(attr->value, params->attr_value))
                params->nodes_found.append(node);
            break;
        case EndsWith:
            if (endsWith(attr->value, params->attr_value))
                params->nodes_found.append(node);
            break;
        default:
            break;
        }
    }

    return false;
}

bool QGumboParser::equals(const char *_big, const char *_little)
{
    return 0 == strcmp(_big, _little);
}

bool QGumboParser::startsWith(const char *_big, const char *_little)
{
    size_t big_length = strlen(_big);
    size_t little_length = strlen(_little);
    return (big_length >= little_length) && !strncmp(_big, _little, little_length);
}

bool QGumboParser::contains(const char *_big, const char *_little)
{
    size_t big_length = strlen(_big);
    size_t little_length = strlen(_little);
    return (little_length != 0) && (big_length >= little_length) && (strstr(_big, _little) != nullptr);
}

bool QGumboParser::endsWith(const char *_big, const char *_little)
{
    size_t big_length = strlen(_big);
    size_t little_length = strlen(_little);
    return (big_length >= little_length) && !strncmp(_big + big_length - little_length, _little, little_length);
}
