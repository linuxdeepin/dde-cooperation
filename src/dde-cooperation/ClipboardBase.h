// SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef CLIPBOARDBASE_H
#define CLIPBOARDBASE_H

#include <string>
#include <vector>
#include <memory>
#include <functional>

class ClipboardObserver {
public:
    virtual ~ClipboardObserver() = default;

    virtual void onClipboardTargetsChanged(const std::vector<std::string> &targets) = 0;
    virtual bool onReadClipboardContent(const std::string &target) = 0;
};

class ClipboardBase {
public:
    explicit ClipboardBase(ClipboardObserver *observer);
    virtual ~ClipboardBase() = default;

    virtual void newClipboardOwnerTargets(const std::vector<std::string> &targets) = 0;
    virtual void readTargetContent(
        const std::string &target,
        const std::function<void(const std::vector<char> &data)> &callback) = 0;
    virtual void updateTargetContent(const std::string &target, const std::vector<char> &data) = 0;
    virtual bool isFiles() = 0;

protected:
    ClipboardObserver *m_observer;

    void notifyTargetsChanged(const std::vector<std::string> &targets);

private:
};

#endif // !CLIPBOARDBASE_H
