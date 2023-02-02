// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ClipboardBase.h"

ClipboardBase::ClipboardBase(ClipboardObserver *observer)
    : m_observer(observer) {
}

void ClipboardBase::notifyTargetsChanged(const std::vector<std::string> &targets) {
    m_observer->onClipboardTargetsChanged(targets);
}
