#include "templategroupdocument.h"

#include "templategroupformat.h"

using namespace Tiled;
using namespace Tiled::Internal;

TemplateGroupDocument::TemplateGroupDocument(TemplateGroup *templateGroup, const QString &fileName)
    : Document(TemplateGroupDocumentType, fileName)
    , mTemplateGroup(templateGroup)
{
}

TemplateGroupDocument::~TemplateGroupDocument()
{
}

bool TemplateGroupDocument::save(const QString &fileName, QString *error)
{
}

TemplateGroupDocument *TemplateGroupDocument::load(const QString &fileName,
                                                   TemplateGroupFormat *format,
                                                   QString *error)
{
}

FileFormat *TemplateGroupDocument::writerFormat() const
{
}

QString TemplateGroupDocument::displayName() const
{
}
