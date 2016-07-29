#include "imagecolorpickerwidget.h"
#include "ui_imagecolorpickerwidget.h"

#include <QString>
#include <QFileInfo>
#include <QDesktopWidget>

using namespace Tiled;
using namespace Tiled::Internal;

ImageColorPickerWidget::ImageColorPickerWidget(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::imageColorPickerWidget)
{
    mUi->setupUi(this);
    connect(mUi->imageArea, SIGNAL(mouseMoved(QMouseEvent*)), SLOT(onMouseMove(QMouseEvent*)));
    connect(mUi->imageArea, SIGNAL(mousePressed(QMouseEvent*)), SLOT(onMousePress(QMouseEvent*)));
    connect(mUi->imageArea, SIGNAL(mouseReleased(QMouseEvent*)), SLOT(onMouseRelease(QMouseEvent*)));
    mPreviewIcon = QPixmap(128, 32);
}

ImageColorPickerWidget::~ImageColorPickerWidget()
{
    delete mUi;
}

bool ImageColorPickerWidget::selectColor(const QString &image)
{
    QPixmap pix(image);
    if (!pix.isNull())
    {
        QString labelText = mTitle;
        mImage = pix.toImage();
        mScaleX = 1;
        mScaleY = 1;

        QRectF rct = findScreen();
        double maxW = rct.width() * (2.0/3.0), maxH = rct.height() * (2.0/3.0);

        if (mImage.width() > maxW || mImage.height() > maxH)
        {
            pix = pix.scaled((int)maxW, (int)maxH, Qt::KeepAspectRatio, Qt::FastTransformation);
            mScaleX = (double)qMin(mImage.width(), pix.width()) / (double)qMax(mImage.width(), pix.width());
            mScaleY = (double)qMin(mImage.height(), pix.height()) / (double)qMax(mImage.height(), pix.height());
            labelText = QLatin1String("%1 (%2X)");
            labelText = labelText.arg(mTitle).arg(QString::number(qMin(mScaleX, mScaleY), 'f', 1));
        }

        mUi->imageArea->setPixmap(pix);
        mUi->imageArea->adjustSize();
        mUi->imageBox->setTitle(labelText);
        show();

        return true;
    }
    return false;
}

void ImageColorPickerWidget::onMouseMove(QMouseEvent* event)
{
    if (!mImage.isNull())
    {
        mPreviewColor = mImage.pixelColor(event->pos().x() / mScaleX, event->pos().y() / mScaleY);
        if(!mPreviewColor.isValid())
            mPreviewColor = mSelectedColor;

        mPreviewIcon.fill(mPreviewColor);
        mUi->preview->setPixmap(mPreviewIcon);
    }
    else
    {
        mPreviewColor = mSelectedColor;
    }

    event->accept();
}

void ImageColorPickerWidget::onMousePress(QMouseEvent * event)
{

}

void ImageColorPickerWidget::onMouseRelease(QMouseEvent * event)
{
    if(event->button() == Qt::MouseButton::LeftButton)
    {
        if(!mImage.isNull())
        {
            mSelectedColor = mPreviewColor;
            emit colorSelected(mSelectedColor);
            this->close();
        }
    }
    else if(event->button() == Qt::RightButton)
    {
        this->close();
    }
}

void ImageColorPickerWidget::resizeEvent(QResizeEvent *event)
{
    move(
           findScreen().center() - rect().center()
        );
}

QRect ImageColorPickerWidget::findScreen() const
{
    QDesktopWidget wind;
    return wind.availableGeometry(wind.screenNumber(this));
}
