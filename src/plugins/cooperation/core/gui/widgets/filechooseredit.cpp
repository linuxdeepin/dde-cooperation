// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filechooseredit.h"

#ifdef linux
#include <DStyle>
#endif

#ifdef DTKWIDGET_CLASS_DSizeMode
#include <DSizeMode>
DWIDGET_USE_NAMESPACE
#endif

#include <QFileDialog>
#include <QHBoxLayout>
#include <QPainter>

using namespace cooperation_core;

FileChooserEdit::FileChooserEdit(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void FileChooserEdit::initUI()
{
    pathLabel = new QLabel(this);
    auto margins = pathLabel->contentsMargins();
    margins.setLeft(8);
    margins.setRight(8);
    pathLabel->setContentsMargins(margins);

    fileChooserBtn = new CooperationSuggestButton(this);
    fileChooserBtn->setFocusPolicy(Qt::NoFocus);
#ifdef linux
    fileChooserBtn->setIcon(DTK_WIDGET_NAMESPACE::DStyleHelper(style()).standardIcon(DTK_WIDGET_NAMESPACE::DStyle::SP_SelectElement, nullptr));
#else
    fileChooserBtn->setStyleSheet(
            "QPushButton {"
            "   background-color: #0098FF;"
            "   border-radius: 10px;"
            "   color: white;"
            "   font-weight: bold;"
            "}");
    fileChooserBtn->setText(" ...");
#endif

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
    QFontMetrics fontMetrices(pathLabel->font());
    QString showName = fontMetrices.elidedText(text, Qt::ElideRight, pathLabel->width() - 16);
    if (showName != text)
        pathLabel->setToolTip(text);

    pathLabel->setText(showName);
}

void FileChooserEdit::onButtonClicked()
{
    auto dirPath = QFileDialog::getExistingDirectory(this);
    if (dirPath.isEmpty())
        return;

    setText(dirPath);
    emit fileChoosed(dirPath);
}

void FileChooserEdit::updateSizeMode()
{
#ifdef DTKWIDGET_CLASS_DSizeMode
    fileChooserBtn->setFixedSize(DSizeModeHelper::element(QSize(24, 24), QSize(36, 36)));
    pathLabel->setFixedHeight(DSizeModeHelper::element(24, 36));

    if (!property("isConnected").toBool()) {
        setProperty("isConnected", true);
        connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, &FileChooserEdit::updateSizeMode);
    }
#else
    fileChooserBtn->setFixedSize(36, 36);
#endif
}

void FileChooserEdit::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    QColor color(0, 0, 0, static_cast<int>(255 * 0.08));
    painter.setBrush(color);
    painter.drawRoundedRect(pathLabel->rect(), 8, 8);

    QWidget::paintEvent(event);
}
