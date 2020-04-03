
#pragma once

#include <QMap>
#include <QObject>
#include <QtXml/QDomDocument>
#include <QVariant>


namespace Tiled {

class ScriptXmlFile;

class ScriptXmlNode: public QObject
{
    Q_OBJECT

    Q_PROPERTY(NodeType type READ type)
    Q_PROPERTY(QVariantMap attributes READ attributes)
    Q_PROPERTY(QList<QObject*> children READ children)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString tag READ tag WRITE setTag)
    Q_PROPERTY(Tiled::ScriptXmlNode* parent READ parent)
    Q_PROPERTY(bool hasAttributes READ hasAttributes)
    Q_PROPERTY(bool hasChildren READ hasChildren)
    Q_PROPERTY(Tiled::ScriptXmlFile *root READ root)

public:
    enum NodeType {
        Element,
        CDATA,
        Text,
        Root,
        Other
    };
    Q_ENUM(NodeType)

    explicit ScriptXmlNode(const QDomNode &node, QObject *parent = nullptr);

    Q_INVOKABLE void clear();
    Q_INVOKABLE QString attribute(const QString &name, const QString &defValue = QString());
    Q_INVOKABLE void setAttribute(const QString &name, const QString &value);
    Q_INVOKABLE bool hasAttribute(const QString &name) const;
    Q_INVOKABLE void removeAttribute(const QString &name);
    Q_INVOKABLE Tiled::ScriptXmlNode *firstChild(const QString &tagName = QString());
    Q_INVOKABLE Tiled::ScriptXmlNode *lastChild(const QString &tagName = QString()) const;
    Q_INVOKABLE Tiled::ScriptXmlNode *previousSibling(const QString &tagName = QString()) const;
    Q_INVOKABLE Tiled::ScriptXmlNode *nextSibling(const QString &tagName = QString()) const;

    Q_INVOKABLE Tiled::ScriptXmlNode *appendChild(Tiled::ScriptXmlNode *newChild);
    Q_INVOKABLE Tiled::ScriptXmlNode *insertBefore(Tiled::ScriptXmlNode *newChild, Tiled::ScriptXmlNode *refChild);
    Q_INVOKABLE Tiled::ScriptXmlNode *insertAfter(Tiled::ScriptXmlNode *newChild, Tiled::ScriptXmlNode *refChild);
    Q_INVOKABLE Tiled::ScriptXmlNode *replaceChild(Tiled::ScriptXmlNode *newChild, Tiled::ScriptXmlNode *oldChild);
    Q_INVOKABLE Tiled::ScriptXmlNode *removeChild(Tiled::ScriptXmlNode *oldChild);

    QString tag() const;
    void setTag(const QString &name);

    QString text() const;
    void setText(const QString &text);

    NodeType type() const;
    QVariantMap attributes() const;
    QList<QObject*> children() const;
    ScriptXmlNode *parent() const;
    bool hasAttributes() const;
    bool hasChildren() const;
    ScriptXmlFile *root() const;

protected:
    inline QDomNode importNode(ScriptXmlNode *node) const;
    inline ScriptXmlNode *checkIfNull(const QDomNode &node) const;

    QDomNode mNode;
};

class ScriptXmlFile: public ScriptXmlNode
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit ScriptXmlFile(const QString &source, QObject *parent = nullptr);
    explicit ScriptXmlFile(const QDomDocument &document, QObject *parent = nullptr);

    Q_INVOKABLE Tiled::ScriptXmlNode *createNode(NodeType type, const QString &data = QString());
    Q_INVOKABLE QString writeToString(int indent = 1) const;

private:
    inline QDomDocument &document();
    inline const QDomDocument &document() const;
};

ScriptXmlNode *ScriptXmlNode::checkIfNull(const QDomNode &node) const
{
   if (node.isNull())
       return nullptr;
   return new ScriptXmlNode(node);
}

QDomNode ScriptXmlNode::importNode(ScriptXmlNode *node) const
{
    if (node->mNode.ownerDocument() != mNode.ownerDocument())
        return mNode.ownerDocument().importNode(node->mNode, true);
    return node->mNode;
}

QDomDocument &ScriptXmlFile::document()
{
    return static_cast<QDomDocument&>(mNode);
}

const QDomDocument &ScriptXmlFile::document() const
{
    return static_cast<const QDomDocument&>(mNode);
}

}

Q_DECLARE_METATYPE(Tiled::ScriptXmlFile*)
Q_DECLARE_METATYPE(Tiled::ScriptXmlNode*)
