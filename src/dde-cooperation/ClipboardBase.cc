// SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ClipboardBase.h"

ClipboardBase::ClipboardBase(ClipboardObserver *observer)
    : m_observer(observer) {
}

void ClipboardBase::notifyTargetsChanged(const std::vector<std::string> &targets) {
    m_observer->onClipboardTargetsChanged(targets);
}
