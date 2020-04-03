#include "scriptxmlfile.h"

#include "scriptmanager.h"

#include <QCoreApplication>

namespace Tiled {

ScriptXmlNode::ScriptXmlNode(const QDomNode &node, QObject *parent)
    : QObject(parent)
    , mNode(node)
{}

void ScriptXmlNode::clear()
{
    mNode.clear();
}

QString ScriptXmlNode::attribute(const QString &name, const QString &defValue)
{
    QDomElement element = mNode.toElement();
    if (element.isNull()) {
        ScriptManager::instance().throwError(QStringLiteral("Node '%1' is not an element node").arg(mNode.nodeName()));
        return defValue;
    }
    return element.attribute(name, defValue);
}

void ScriptXmlNode::setAttribute(const QString &name, const QString &value)
{
    QDomElement element = mNode.toElement();
    if (element.isNull()) {
        ScriptManager::instance().throwError(QStringLiteral("Node '%1' is not an element node").arg(mNode.nodeName()));
        return;
    }
    element.setAttribute(name, value);
}

bool ScriptXmlNode::hasAttribute(const QString &name) const
{
    QDomElement element = mNode.toElement();
    if (element.isNull()) {
        ScriptManager::instance().throwError(QStringLiteral("Node '%1' is not an element node").arg(mNode.nodeName()));
        return false;
    }
    return element.hasAttribute(name);
}

void ScriptXmlNode::removeAttribute(const QString &name)
{
    QDomElement element = mNode.toElement();
    if (element.isNull()) {
        ScriptManager::instance().throwError(QStringLiteral("Node '%1' is not an element node").arg(mNode.nodeName()));
        return;
    }
    element.removeAttribute(name);
}

QString ScriptXmlNode::tag() const
{
    QDomElement element = mNode.toElement();
    if (element.isNull()) {
        ScriptManager::instance().throwError(QStringLiteral("Node '%1' is not an element node").arg(mNode.nodeName()));
        return {};
    }
    return element.tagName();
}

void ScriptXmlNode::setTag(const QString &name)
{
    QDomElement element = mNode.toElement();
    if (element.isNull()) {
        ScriptManager::instance().throwError(QStringLiteral("Node '%1' is not an element node").arg(mNode.nodeName()));
        return;
    }
    element.setTagName(name);
}

QString ScriptXmlNode::text() const
{
    if (mNode.isText())
        return mNode.toText().data();
    if (mNode.isCDATASection())
        return mNode.toCDATASection().data();
    if (mNode.isCharacterData())
        return mNode.toCharacterData().data();
    ScriptManager::instance().throwError(QStringLiteral("Node '%1' is not a character data node").arg(mNode.nodeName()));
    return {};
}

void ScriptXmlNode::setText(const QString &v)
{
    if (mNode.isText())
        return mNode.toText().setData(v);
    if (mNode.isCDATASection())
        return mNode.toCDATASection().setData(v);
    if (mNode.isCharacterData())
        return mNode.toCharacterData().setData(v);
    ScriptManager::instance().throwError(QStringLiteral("Node '%1' is not a character data node").arg(mNode.nodeName()));
    return;
}

ScriptXmlNode::NodeType ScriptXmlNode::type() const
{
    if (mNode.isElement())
        return NodeType::Element;
    if (mNode.isText())
        return NodeType::Text;
    if (mNode.isCDATASection())
        return NodeType::CDATA;
    if (mNode.isDocument())
        return NodeType::Root;
    return NodeType::Other;
}

QVariantMap ScriptXmlNode::attributes() const
{
    if (!mNode.isElement()) {
        ScriptManager::instance().throwError(QStringLiteral("Node '%1' is not an element node").arg(mNode.nodeName()));
        return {};
    }

    QVariantMap map;
    auto attributes = mNode.attributes();
    for (int i = 0; i < attributes.length(); i++) {
        auto attribute = attributes.item(i);
        map.insert(attribute.nodeName(), QVariant(attribute.nodeValue()));
    }
    return map;
}

QList<QObject*> ScriptXmlNode::children() const
{
    QList<QObject*> list;
    auto children = mNode.childNodes();
    for (int i = 0; i < children.length(); i++) {
        auto child = children.at(i);
        if (!child.isNull())
            list.append(new ScriptXmlNode(child));
    }
    return list;
}

bool ScriptXmlNode::hasAttributes() const
{
    return mNode.hasAttributes();
}

bool ScriptXmlNode::hasChildren() const
{
    return mNode.hasChildNodes();
}

ScriptXmlNode *ScriptXmlNode::parent() const
{
    auto parent = mNode.parentNode();
    if (parent.isNull())
        return nullptr;
    return checkIfNull(parent);
}

ScriptXmlFile *ScriptXmlNode::root() const
{
    auto document = mNode.toDocument();
    if (!mNode.isNull())
        return new ScriptXmlFile(document);
    auto root = mNode.ownerDocument();
    if (root.isNull())
        return nullptr;
    else
        return new ScriptXmlFile(root);
}

ScriptXmlNode *ScriptXmlNode::firstChild(const QString &tagName)
{
    if (tagName.isEmpty())
        return checkIfNull(mNode.firstChild());
    return checkIfNull(mNode.firstChildElement(tagName));
}

ScriptXmlNode *ScriptXmlNode::lastChild(const QString &tagName) const
{
    if (tagName.isEmpty())
        return checkIfNull(mNode.lastChild());
    return checkIfNull(mNode.lastChildElement(tagName));
}

ScriptXmlNode *ScriptXmlNode::previousSibling(const QString &tagName) const
{
    if (tagName.isEmpty())
        return checkIfNull(mNode.previousSibling());
    return checkIfNull(mNode.previousSiblingElement(tagName));
}

ScriptXmlNode *ScriptXmlNode::nextSibling(const QString &tagName) const
{
    if (tagName.isEmpty())
        return checkIfNull(mNode.nextSibling());
    return checkIfNull(mNode.nextSiblingElement(tagName));
}

ScriptXmlNode *ScriptXmlNode::appendChild(ScriptXmlNode *newChild)
{
    if (!newChild) {
        ScriptManager::instance().throwError(QStringLiteral("First argument is undefined"));
        return nullptr;
    }

    return checkIfNull(mNode.appendChild(importNode(newChild)));
}

ScriptXmlNode *ScriptXmlNode::insertBefore(ScriptXmlNode *newChild, ScriptXmlNode *refChild)
{
    if (!newChild) {
        ScriptManager::instance().throwError(QStringLiteral("First argument is undefined"));
        return {};
    }

    if (!refChild) {
        ScriptManager::instance().throwError(QStringLiteral("Second argument is undefined"));
        return {};
    }

    return checkIfNull(mNode.insertBefore(importNode(newChild), refChild->mNode));
}

ScriptXmlNode *ScriptXmlNode::insertAfter(ScriptXmlNode *newChild, ScriptXmlNode *refChild)
{
    if (!newChild) {
        ScriptManager::instance().throwError(QStringLiteral("First argument is undefined"));
        return {};
    }

    if (!refChild) {
        ScriptManager::instance().throwError(QStringLiteral("Second argument is undefined"));
        return {};
    }

    return checkIfNull(mNode.insertAfter(importNode(newChild), refChild->mNode));
}

ScriptXmlNode *ScriptXmlNode::replaceChild(ScriptXmlNode *newChild, ScriptXmlNode *oldChild)
{
    if (!newChild) {
        ScriptManager::instance().throwError(QStringLiteral("First argument is undefined"));
        return {};
    }

    if (!oldChild) {
        ScriptManager::instance().throwError(QStringLiteral("Second argument is undefined"));
        return {};
    }

    return checkIfNull(mNode.replaceChild(importNode(newChild), oldChild->mNode));
}

ScriptXmlNode *ScriptXmlNode::removeChild(ScriptXmlNode *oldChild)
{
    if (!oldChild) {
        ScriptManager::instance().throwError(QStringLiteral("First argument is undefined"));
        return {};
    }

    return checkIfNull(mNode.removeChild(oldChild->mNode));
}


ScriptXmlFile::ScriptXmlFile(const QDomDocument &document, QObject *parent)
    : ScriptXmlNode(document, parent)
{}

ScriptXmlFile::ScriptXmlFile(const QString &source, QObject *parent)
    : ScriptXmlFile(QDomDocument(), parent)
{
    QString error;
    int line;
    int column;
    if (!document().setContent(source, false, &error, &line, &column)) {
        auto message = QStringLiteral("Unable to parse xml: %1 (line %2, column %3)")
                .arg(error).arg(line).arg(column);
        ScriptManager::instance().throwError(message);

        // Return mDocument to a valid state.
        document().setContent({});
    }
}

ScriptXmlNode *ScriptXmlFile::createNode(NodeType type, const QString &data)
{
    switch (type) {
    case NodeType::Element:
        return new ScriptXmlNode(document().createElement(data));
    case NodeType::Text:
        return new ScriptXmlNode(document().createTextNode(data));
    case NodeType::CDATA:
        return new ScriptXmlNode(document().createCDATASection(data));
    case NodeType::Root:
        ScriptManager::instance().throwError(QStringLiteral("Cannot use createNode to make XmlFile instances. Use the constructor instead."));
        return nullptr;
    default:
        ScriptManager::instance().throwError(QStringLiteral("Cannot create node type"));
        return nullptr;
    }
}

QString ScriptXmlFile::writeToString(int indent) const
{
    return document().toString(indent);
}

} // namespace Tiled
