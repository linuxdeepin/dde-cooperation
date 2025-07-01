// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filechooseredit.h"
#include "common/log.h"

#ifdef linux
#    include <DStyle>
#endif

#ifdef DTKWIDGET_CLASS_DSizeMode
#    include <DSizeMode>
DWIDGET_USE_NAMESPACE
#endif

#include <QFileDialog>
#include <QHBoxLayout>
#include <QPainter>

using namespace cooperation_core;

FileChooserEdit::FileChooserEdit(QWidget *parent)
    : QWidget(parent)
{
    DLOG << "Initializing file chooser edit";
    initUI();
    DLOG << "Initialization completed";
}

void FileChooserEdit::initUI()
{
#ifdef linux
    DLOG << "Initializing UI for Linux";
    pathLabel = new CooperationLineEdit(this);
    pathLabel->setClearButtonEnabled(false);
    pathLabel->lineEdit()->setReadOnly(true);
    fileChooserBtn = new CooperationSuggestButton(this);
    fileChooserBtn->setIcon(DTK_WIDGET_NAMESPACE::DStyleHelper(style()).standardIcon(DTK_WIDGET_NAMESPACE::DStyle::SP_SelectElement, nullptr));
#else
    DLOG << "Initializing UI for non-Linux";
    pathLabel = new QLabel(this);
    auto margins = pathLabel->contentsMargins();
    margins.setLeft(8);
    margins.setRight(8);
    pathLabel->setContentsMargins(margins);
    fileChooserBtn = new FileChooserBtn(this);
    fileChooserBtn->setStyleSheet(
            "QPushButton {"
            "   background-color: #0098FF;"
            "   border-radius: 10px;"
            "   color: white;"
            "   font-weight: bold;"
            "}");
    DLOG << "File chooser button style set";
#endif
    fileChooserBtn->setFocusPolicy(Qt::NoFocus);
    connect(fileChooserBtn, &QPushButton::clicked, this, &FileChooserEdit::onButtonClicked);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(10);
    setLayout(mainLayout);

    mainLayout->addWidget(pathLabel);
    mainLayout->addWidget(fileChooserBtn);

    updateSizeMode();
}

void FileChooserEdit::setText(const QString &text)
{
    DLOG << "Setting text:" << text.toStdString();
    QFontMetrics fontMetrices(pathLabel->font());
    QString showName = fontMetrices.elidedText(text, Qt::ElideRight, pathLabel->width() - 16);
    if (showName != text) {
        DLOG << "Text truncated, setting tooltip";
        pathLabel->setToolTip(text);
    }

    pathLabel->setText(showName);
    DLOG << "Text set to:" << showName.toStdString();
}

void FileChooserEdit::onButtonClicked()
{
    DLOG << "Opening file dialog";
    auto dirPath = QFileDialog::getExistingDirectory(this);
    if (dirPath.isEmpty()) {
        DLOG << "No directory selected";
        return;
    }

    if (!QFileInfo(dirPath).isWritable() || QDir(dirPath).entryInfoList().isEmpty()) {
        DLOG << "Invalid directory selected:" << dirPath.toStdString();
        InformationDialog dialog;
        dialog.exec();
        onButtonClicked();
        return;
    }

    DLOG << "Valid directory selected:" << dirPath.toStdString();
    setText(dirPath);
    emit fileChoosed(dirPath);
}

void FileChooserEdit::updateSizeMode()
{
    DLOG << "Updating size mode";
#ifdef DTKWIDGET_CLASS_DSizeMode
    fileChooserBtn->setFixedSize(DSizeModeHelper::element(QSize(24, 24), QSize(36, 36)));
    pathLabel->setFixedHeight(DSizeModeHelper::element(24, 36));
    DLOG << "Size mode elements updated";

    if (!property("isConnected").toBool()) {
        DLOG << "Connecting size mode signals";
        setProperty("isConnected", true);
        connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, &FileChooserEdit::updateSizeMode);
    }
#else
    fileChooserBtn->setFixedSize(36, 36);
    DLOG << "Using fixed size for non-DTK environment";
#endif
}

void FileChooserEdit::paintEvent(QPaintEvent *event)
{
#ifndef linux
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    QColor color(0, 0, 0, static_cast<int>(255 * 0.08));
    painter.setBrush(color);
    painter.drawRoundedRect(pathLabel->rect(), 8, 8);
#endif
    QWidget::paintEvent(event);
}
InformationDialog::InformationDialog(QWidget *parent)
    : CooperationDialog(parent)
{
    initUI();
}

void InformationDialog::closeEvent(QCloseEvent *event)
{
    CooperationAbstractDialog::closeEvent(event);
}

void InformationDialog::initUI()
{
    setFixedSize(380, 234);
    setContentsMargins(0, 0, 0, 0);
    QWidget *contentWidget = new QWidget(this);
    QPushButton *okBtn = new QPushButton(this);
    okBtn->setText(tr("OK"));
    connect(okBtn, &QPushButton::clicked, this, &InformationDialog::close);

#ifdef linux
    DLOG << "Initializing InformationDialog UI for Linux";
    setIcon(QIcon::fromTheme("dde-cooperation"));
    setTitle(tr("the file save location is invalid"));
    addContent(contentWidget);
#else
    DLOG << "Initializing InformationDialog UI for non-Linux";
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addSpacing(30);
    layout->addWidget(contentWidget);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    titleLabel = new CooperationLabel(this);
    titleLabel->setText(tr("the file save location is invalid"));
    titleLabel->setAlignment(Qt::AlignCenter);
#endif
    iconLabel = new CooperationLabel(this);
    msgLabel = new CooperationLabel(this);
    msgLabel->setAlignment(Qt::AlignCenter);
    msgLabel->setText(tr("This path is a read-only directory. Please choose a different location for saving the file."));
    msgLabel->setWordWrap(true);
    iconLabel = new CooperationLabel(this);
    iconLabel->setAlignment(Qt::AlignHCenter);
    QIcon icon(QString(":/icons/deepin/builtin/icons/transfer_fail_128px.svg"));
    iconLabel->setPixmap(icon.pixmap(48, 48));
    QVBoxLayout *vLayout = new QVBoxLayout(contentWidget);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->addWidget(titleLabel, Qt::AlignTop);
    vLayout->addWidget(iconLabel);
    vLayout->addWidget(msgLabel, Qt::AlignVCenter);
    vLayout->addWidget(okBtn, 0, Qt::AlignBottom);
}
