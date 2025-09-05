// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filetransfersettingsdialog.h"
#include "configs/settings/configmanager.h"
#include "configs/dconfig/dconfigmanager.h"
#include "reportlog/reportlogmanager.h"
#include <DStyle>
#include <DGuiApplicationHelper>

#include <QLabel>
#include <QPainter>
#include <QFileDialog>
#include <QStandardPaths>
#include <QPainterPath>

using namespace dfmplugin_cooperation;
DWIDGET_USE_NAMESPACE

FileChooserEdit::FileChooserEdit(QWidget *parent)
    : QWidget(parent)
{
    qInfo() << "Creating FileChooserEdit instance";
    initUI();
}

void FileChooserEdit::setText(const QString &text)
{
    qInfo() << "Setting text for file chooser edit:" << text;
    QFontMetrics fontMetrices(pathLabel->font());
    QString showName = fontMetrices.elidedText(text, Qt::ElideRight, pathLabel->width() - 16);
    if (showName != text) {
        qInfo() << "Text truncated, setting tooltip";
        pathLabel->setToolTip(text);
    }

    pathLabel->setText(showName);
    qInfo() << "Text set to:" << showName;
}

void FileChooserEdit::initUI()
{
    qInfo() << "Initializing file chooser edit UI";
    pathLabel = new QLabel(this);
    pathLabel->setContentsMargins(8, 8, 8, 8);
    pathLabel->setText(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));

    fileChooserBtn = new DSuggestButton(this);
    fileChooserBtn->setIcon(DStyleHelper(style()).standardIcon(DStyle::SP_SelectElement, nullptr));
    fileChooserBtn->setFixedSize(36, 36);
    connect(fileChooserBtn, &DSuggestButton::clicked, this, &FileChooserEdit::onButtonClicked);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(10);
    setLayout(mainLayout);

    mainLayout->addWidget(pathLabel);
    mainLayout->addWidget(fileChooserBtn);
    qInfo() << "File chooser edit UI initialized";
}

void FileChooserEdit::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    QColor color(0, 0, 0, static_cast<int>(255 * 0.08));
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
        color = QColor("#444444");
    }

    painter.setBrush(color);
    painter.drawRoundedRect(pathLabel->rect(), 8, 8);

    QWidget::paintEvent(event);
}

void FileChooserEdit::onButtonClicked()
{
    qInfo() << "File chooser button clicked";
    auto dirPath = QFileDialog::getExistingDirectory(this);
    if (dirPath.isEmpty()) {
        qInfo() << "No directory selected, returning";
        return;
    }

    setText(dirPath);
    emit fileChoosed(dirPath);
    qInfo() << "File path emitted:" << dirPath;
}

BackgroundWidget::BackgroundWidget(QWidget *parent)
    : QFrame(parent)
{
    qInfo() << "Creating BackgroundWidget instance";
}

void BackgroundWidget::setRoundRole(BackgroundWidget::RoundRole role)
{
    qInfo() << "Setting round role for BackgroundWidget";
    this->role = role;
}

void BackgroundWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    const int radius = 8;
    QRect paintRect = this->rect();
    QPainterPath path;

    switch (role) {
    case Top:
        qInfo() << "Painting top rounded background";
        path.moveTo(paintRect.bottomRight());
        path.lineTo(paintRect.topRight() + QPoint(0, radius));
        path.arcTo(QRect(QPoint(paintRect.topRight() - QPoint(radius * 2, 0)),
                         QSize(radius * 2, radius * 2)),
                   0, 90);
        path.lineTo(paintRect.topLeft() + QPoint(radius, 0));
        path.arcTo(QRect(QPoint(paintRect.topLeft()), QSize(radius * 2, radius * 2)), 90, 90);
        path.lineTo(paintRect.bottomLeft());
        path.lineTo(paintRect.bottomRight());
        break;
    case Bottom:
        qInfo() << "Painting bottom rounded background";
        path.moveTo(paintRect.bottomRight() - QPoint(0, radius));
        path.lineTo(paintRect.topRight());
        path.lineTo(paintRect.topLeft());
        path.lineTo(paintRect.bottomLeft() - QPoint(0, radius));
        path.arcTo(QRect(QPoint(paintRect.bottomLeft() - QPoint(0, radius * 2)),
                         QSize(radius * 2, radius * 2)),
                   180, 90);
        path.lineTo(paintRect.bottomLeft() + QPoint(radius, 0));
        path.arcTo(QRect(QPoint(paintRect.bottomRight() - QPoint(radius * 2, radius * 2)),
                         QSize(radius * 2, radius * 2)),
                   270, 90);
        break;
    default:
        qInfo() << "Painting default background";
        break;
    }

    QColor color = DGuiApplicationHelper::instance()->applicationPalette().frameBorder().color();
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
        qInfo() << "Dark theme detected, setting color to #323232";
        color = QColor("#323232");
        color.setAlpha(230);
    }

    painter.fillPath(path, color);
    QFrame::paintEvent(event);
}

FileTransferSettingsDialog::FileTransferSettingsDialog(QWidget *parent)
    : DDialog(parent)
{
    qDebug() << "Creating file transfer settings dialog";
    initUI();
    initConnect();
    qDebug() << "File transfer settings dialog initialized";
}

void FileTransferSettingsDialog::initUI()
{
    qDebug() << "Initializing file transfer settings UI";

    setIcon(QIcon::fromTheme("dde-file-manager"));
    setTitle(tr("File transfer settings"));
    setFixedWidth(400);
    setContentsMargins(0, 0, 0, 0);
    qDebug() << "Dialog basic properties set";

    QWidget *contentWidget = new QWidget(this);
    mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 10, 0, 10);
    mainLayout->setSpacing(1);
    contentWidget->setLayout(mainLayout);
    addContent(contentWidget);
    qDebug() << "Main content layout created";

    fileChooserEdit = new FileChooserEdit(this);
    qDebug() << "File chooser edit created";

    comBox = new DComboBox(this);
    QStringList items { tr("Everyone in the same LAN"),
                        tr("Only those who are collaborating are allowed"),
                        tr("Not allow") };
    comBox->addItems(items);
    comBox->setFocusPolicy(Qt::NoFocus);
    qDebug() << "Transfer mode combo box created with options";

    addItem(tr("Allows the following users to send files to me"), comBox, 0);
    addItem(tr("File save location"), fileChooserEdit, 1);
    qInfo() << "File transfer settings UI initialized";
}

void FileTransferSettingsDialog::initConnect()
{
    connect(comBox, qOverload<int>(&DComboBox::currentIndexChanged), this, &FileTransferSettingsDialog::onComBoxValueChanged);
    connect(fileChooserEdit, &FileChooserEdit::fileChoosed, this, &FileTransferSettingsDialog::onFileChoosered);
}

void FileTransferSettingsDialog::loadConfig()
{
    qDebug() << "Loading file transfer settings";
#ifdef linux
    auto value = DConfigManager::instance()->value(kDefaultCfgPath, "cooperation.transfer.mode", 0);
    int mode = value.toInt();
    mode = (mode < 0) ? 0 : (mode > 2) ? 2 : mode;
    comBox->setCurrentIndex(mode);
    qDebug() << "Loaded transfer mode from dconfig:" << mode;
#else
    qDebug() << "Non-Linux platform, loading transfer mode from ConfigManager";
    auto value = ConfigManager::instance()->appAttribute("GenericAttribute", "TransferMode");
    comBox->setCurrentIndex(value.isValid() ? value.toInt() : 0);
#endif

    value = ConfigManager::instance()->appAttribute("GenericAttribute", "StoragePath");
    fileChooserEdit->setText(value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
}

void FileTransferSettingsDialog::addItem(const QString &text, QWidget *widget, int indexPos)
{
    qInfo() << "Adding item with text:" << text;
    BackgroundWidget *bgWidget = new BackgroundWidget(this);
    switch (indexPos) {
    case 0:
        qInfo() << "Setting round role to Top for index 0";
        bgWidget->setRoundRole(BackgroundWidget::Top);
        break;
    case 1:
        qInfo() << "Setting round role to Bottom for index 1";
        bgWidget->setRoundRole(BackgroundWidget::Bottom);
        break;
    default:
        qInfo() << "Unknown index position, no round role set";
        break;
    }

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setContentsMargins(10, 10, 10, 10);
    vLayout->setSpacing(10);
    bgWidget->setLayout(vLayout);

    QLabel *label = new QLabel(text, this);

    // 设置打点省略显示，使用固定宽度计算
    QFontMetrics fontMetrics(label->font());
    int availableWidth = 360;  // 对话框宽度400px减去左右边距
    QString elidedText = fontMetrics.elidedText(text, Qt::ElideRight, availableWidth);
    label->setText(elidedText);
    if (elidedText != text) {
        label->setToolTip(text);  // 如果文本被截断，设置完整文本为工具提示
    }

    vLayout->addWidget(label);
    vLayout->addWidget(widget);

    mainLayout->addWidget(bgWidget);
    qInfo() << "Added item with text:" << text;
}

void FileTransferSettingsDialog::onFileChoosered(const QString &fileName)
{
    ConfigManager::instance()->setAppAttribute("GenericAttribute", "StoragePath", fileName);
    qInfo() << "Saved storage path to config:" << fileName;
}

void FileTransferSettingsDialog::onComBoxValueChanged(int index)
{
#ifdef linux
    qInfo() << "Linux platform, setting cooperation.transfer.mode to:" << index;
    DConfigManager::instance()->setValue(kDefaultCfgPath, "cooperation.transfer.mode", index);
    bool status = index == 2 ? false : true;
    QVariantMap data;
    data.insert("enableFileDelivery", status);
    deepin_cross::ReportLogManager::instance()->commit("CooperationStatus", data);
    qInfo() << "Reported transfer mode change to log system";
#else
    qInfo() << "Non-Linux platform, setting GenericAttribute.TransferMode to:" << index;
    ConfigManager::instance()->setAppAttribute("GenericAttribute", "TransferMode", index);
    qInfo() << "Saved transfer mode to config:" << index;
#endif
}

void FileTransferSettingsDialog::showEvent(QShowEvent *e)
{
    qInfo() << "File transfer settings dialog shown";
    loadConfig();
    DDialog::showEvent(e);
}
