// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Clipboard.h"

#include <stdexcept>

#include <climits>

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xfixes.h>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <QDebug>

using namespace X11;

const std::string Clipboard::ATOMS_NAME[] = {
    "CPRT_TARGETS",
    "CPRT_PROPERTY",
    "CPRT_PROPERTIES",

    "ATOM",
    "ATOM_PAIR",
    "CLIPBOARD",
    "TARGETS",
    "MULTIPLE",
    "STRING",
    "UTF8_STRING",
    "TEXT",
};

Clipboard::Clipboard(ClipboardObserver *observer, QObject *parent)
    : X11(parent)
    , ClipboardBase(observer)
    , m_printingProperty(false) {
    m_dummyWindow = xcb_generate_id(m_conn);
    uint32_t valueList[] = {XCB_BACK_PIXMAP_NONE};
    auto cookie = xcb_create_window_checked(m_conn,
                                            m_screen->root_depth,
                                            m_dummyWindow,
                                            m_screen->root,
                                            0,
                                            0,
                                            1,
                                            1,
                                            0,
                                            XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                            m_screen->root_visual,
                                            XCB_CW_BACK_PIXMAP,
                                            valueList);
    auto *err = xcb_request_check(m_conn, cookie);
    if (err != nullptr) {
        qWarning() << fmt::format("failed to create window: {}", err->error_code).data();
    }

    qInfo() << fmt::format("clipboard window id: {}", m_dummyWindow).data();

    m_selection = getAtom(ATOM_LIST::CLIPBOARD);

    initXfixesExtension();
}

Clipboard::~Clipboard() {
    xcb_destroy_window(m_conn, m_dummyWindow);
    xcb_flush(m_conn);
}

xcb_atom_t Clipboard::getAtom(ATOM_LIST atom, bool onlyIfExists) {
    return getAtom(ATOMS_NAME[static_cast<unsigned int>(atom)], onlyIfExists);
}

xcb_atom_t Clipboard::getAtom(const char *name, bool onlyIfExists) {
    return getAtom(std::string(name), onlyIfExists);
}

xcb_atom_t Clipboard::getAtom(const std::string &name, bool onlyIfExists) {
    auto it = m_atoms.find(name);
    if (it != m_atoms.end()) {
        return it->second;
    }

    auto reply = XCB_REPLY(xcb_intern_atom, m_conn, onlyIfExists, name.length(), name.c_str());
    qWarning() << fmt::format("ATOM: {}, {}", name, reply->atom).data();
    m_atoms.emplace(name, reply->atom);
    return reply->atom;
}

void Clipboard::initXfixesExtension() {
    m_xfixes = xcb_get_extension_data(m_conn, &xcb_xfixes_id);
    if (!m_xfixes->present) {
        qWarning("xfixes is not present");
        return;
    }

    xcb_generic_error_t *err = nullptr;
    auto reply = XCB_REPLY(xcb_xfixes_query_version,
                           m_conn,
                           XCB_XFIXES_MAJOR_VERSION,
                           XCB_XFIXES_MINOR_VERSION);
    if (err != nullptr) {
        qWarning() << fmt::format("xcb_xfixes_query_version: {}", err->error_code).data();
        return;
    }

    qDebug()
        << fmt::format("xfixes version {}.{}", reply->major_version, reply->minor_version).data();
    qDebug() << fmt::format("xfixes first event: {}", m_xfixes->first_event).data();

    xcb_void_cookie_t cookie = xcb_xfixes_select_selection_input_checked(
        m_conn,
        m_screen->root,
        m_selection,
        XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER);
    err = xcb_request_check(m_conn, cookie);
    if (err != nullptr) {
        qWarning()
            << fmt::format("xcb_xfixes_select_selection_input_checked: {}", err->error_code).data();
    }
}

void Clipboard::handleEvent(std::shared_ptr<xcb_generic_event_t> event) {
    auto response_type = event->response_type & ~0x80;
    qDebug() << fmt::format("event: {}", response_type).data();
    if (response_type == m_xfixes->first_event + XCB_XFIXES_SELECTION_NOTIFY) {
        auto ev = reinterpret_cast<xcb_xfixes_selection_notify_event_t *>(event.get());
        if (ev->owner != m_dummyWindow) {
            reset();
            printPropertyTargets();
        }
    } else {
        switch (response_type) {
        case XCB_SELECTION_REQUEST: {
            auto ev = std::reinterpret_pointer_cast<xcb_selection_request_event_t>(event);
            handleXcbSelectionRequest(ev);
            break;
        }
        case XCB_SELECTION_NOTIFY: {
            auto ev = std::reinterpret_pointer_cast<xcb_selection_notify_event_t>(event);
            // if (ev->requestor != m_dummyWindow) {
            handleXcbSelectionNotify(ev);
            // }
            break;
        }
        }
    }
}

void Clipboard::reset() {
    m_cachedTargets.clear();
    m_cachedProperties.clear();
    m_sectionPropertyNotifyCb.clear();
}

std::vector<char> Clipboard::readProperty(xcb_window_t requestor, xcb_atom_t property) {
    // Don't read anything, just get the size of the property data
    auto reply = XCB_REPLY(xcb_get_property,
                           m_conn,
                           false,
                           requestor,
                           property,
                           XCB_GET_PROPERTY_TYPE_ANY,
                           0,
                           0);
    if (!reply || reply->type == XCB_NONE) {
        qWarning("no reply");
        return {};
    }

    decltype(reply->bytes_after) buffOffset = 0, offset = 0;
    auto bytesLeft = reply->bytes_after;
    qInfo() << fmt::format("target {} size: {}", getTargetName(property), bytesLeft).data();
    std::vector<char> buff(bytesLeft);

    while (bytesLeft > 0) {
        reply = XCB_REPLY(xcb_get_property,
                          m_conn,
                          false,
                          m_dummyWindow,
                          property,
                          XCB_GET_PROPERTY_TYPE_ANY,
                          offset,
                          UINT_MAX / 4);
        if (!reply || reply->type == XCB_NONE) {
            qWarning("no reply");
            break;
        }

        bytesLeft = reply->bytes_after;
        char *data = static_cast<char *>(xcb_get_property_value(reply.get()));
        int length = xcb_get_property_value_length(reply.get());

        if ((buffOffset + length) > buff.size()) {
            qWarning("buffer overflow");
            length = buff.size() - buffOffset;
            // escape loop
            bytesLeft = 0;
        }

        memcpy(buff.data() + buffOffset, data, length);
        buffOffset += length;

        if (bytesLeft) {
            offset += length / 4;
        }
    }

    return buff;
}

void Clipboard::printPropertyTargets() {
    auto cookie = xcb_convert_selection_checked(m_conn,
                                                m_dummyWindow,
                                                m_selection,
                                                getAtom(ATOM_LIST::TARGETS),
                                                getAtom(ATOM_LIST::CPRT_TARGETS),
                                                XCB_CURRENT_TIME);
    auto *err = xcb_request_check(m_conn, cookie);
    if (err != nullptr) {
        qWarning() << fmt::format("xcb_convert_selection_checked: {}", err->error_code).data();
        return;
    }
}

/**
 * 通过设置 CPRT_PROPERTY 属性，并通过 xcb_convert_selection 通知剪切板所有者。
 * 剪切板所有者读取 CPRT_PROPERTY 属性知道我们想要获取的 target，然后设置回来。
 * 因此，需要等上一次获取的属性值返回后，才能继续获取，否则覆盖了 CPRT_PROPERTY
 * 可能导致获取错误
 */
void Clipboard::printProperty(xcb_atom_t atom) {
    m_penddingPrint.push_back(atom);
    if (m_printingProperty) {
        return;
    }

    m_printingProperty = true;
    printNextPenddingProperty();
}

void Clipboard::printNextPenddingProperty() {
    xcb_atom_t atom = m_penddingPrint.front();
    m_penddingPrint.pop_front();

    auto cookie = xcb_convert_selection_checked(m_conn,
                                                m_dummyWindow,
                                                m_selection,
                                                atom,
                                                getAtom(ATOM_LIST::CPRT_PROPERTY),
                                                XCB_CURRENT_TIME);
    auto *err = xcb_request_check(m_conn, cookie);
    if (err != nullptr) {
        qWarning() << fmt::format("xcb_convert_selection_checked: {}", err->error_code).data();
        return;
    }
}

void Clipboard::printProperties(const std::vector<xcb_atom_t> &pairs) {
    {
        auto cookie = xcb_change_property_checked(m_conn,
                                                  XCB_PROP_MODE_REPLACE,
                                                  m_dummyWindow,
                                                  getAtom(ATOM_LIST::CPRT_PROPERTIES),
                                                  getAtom(ATOM_LIST::ATOM_PAIR),
                                                  8 * sizeof(xcb_atom_t),
                                                  pairs.size(),
                                                  pairs.data());
        auto *err = xcb_request_check(m_conn, cookie);
        if (err != nullptr) {
            qWarning() << fmt::format("xcb_change_property_checked: {}", err->error_code).data();
            return;
        }
    }
    {
        auto cookie = xcb_convert_selection_checked(m_conn,
                                                    m_dummyWindow,
                                                    m_selection,
                                                    getAtom(ATOM_LIST::MULTIPLE),
                                                    getAtom(ATOM_LIST::CPRT_PROPERTIES),
                                                    XCB_CURRENT_TIME);
        auto *err = xcb_request_check(m_conn, cookie);
        if (err != nullptr) {
            qWarning() << fmt::format("xcb_convert_selection_checked: {}", err->error_code).data();
            return;
        }
    }
}

std::vector<xcb_atom_t> Clipboard::getTargets() {
    std::vector<xcb_atom_t> targets;

    auto buff = readProperty(m_dummyWindow, getAtom(ATOM_LIST::CPRT_TARGETS));
    size_t atomsCnt = buff.size() / 4;
    xcb_atom_t *atoms = reinterpret_cast<xcb_atom_t *>(buff.data());

    for (size_t i = 0; i < atomsCnt; i++) {
        targets.emplace_back(atoms[i]);
    }

    return targets;
}

std::vector<std::string> Clipboard::getTargetsName(const std::vector<xcb_atom_t> &atoms) {
    std::vector<std::string> targets;

    xcb_get_atom_name_cookie_t cookies[atoms.size()];
    for (size_t i = 0; i < atoms.size(); i++) {
        cookies[i] = xcb_get_atom_name_unchecked(m_conn, atoms[i]);
    }

    for (size_t i = 0; i < atoms.size(); i++) {
        auto reply = std::unique_ptr<xcb_get_atom_name_reply_t>(
            xcb_get_atom_name_reply(m_conn, cookies[i], nullptr));
        if (!reply) {
            qWarning("no reply");
            continue;
        }

        targets.emplace_back(
            std::string{xcb_get_atom_name_name(reply.get()),
                        static_cast<size_t>(xcb_get_atom_name_name_length(reply.get()))});
    }

    return targets;
}

std::string Clipboard::getTargetName(xcb_atom_t atom) {
    auto cookie = xcb_get_atom_name_unchecked(m_conn, atom);
    auto reply = std::unique_ptr<xcb_get_atom_name_reply_t>(
        xcb_get_atom_name_reply(m_conn, cookie, nullptr));
    if (!reply) {
        qWarning("no reply");
        return "";
    }

    return std::string{xcb_get_atom_name_name(reply.get()),
                       static_cast<size_t>(xcb_get_atom_name_name_length(reply.get()))};
}

void Clipboard::setTargets(xcb_window_t requestor,
                           xcb_atom_t property,
                           const std::vector<xcb_atom_t> &targets) {
    xcb_change_property(m_conn,
                        XCB_PROP_MODE_REPLACE,
                        requestor,
                        property,
                        getAtom(ATOM_LIST::ATOM),
                        8 * sizeof(xcb_atom_t),
                        targets.size(),
                        targets.data());
}

bool Clipboard::setRequestorPropertyWithClipboardContent(const xcb_atom_t requestor,
                                                         const xcb_atom_t property,
                                                         const xcb_atom_t target) {
    xcb_change_property(m_conn,
                        XCB_PROP_MODE_REPLACE,
                        requestor,
                        property,
                        target,
                        8,
                        m_cachedProperties[target].size(),
                        m_cachedProperties[target].data());
    return true;
}

void Clipboard::notifyRequestor(std::shared_ptr<xcb_selection_request_event_t> event) {
    // Notify the "requestor" that we've already updated the property.
    xcb_selection_notify_event_t notify;
    notify.response_type = XCB_SELECTION_NOTIFY;
    notify.pad0 = 0;
    notify.sequence = 0;
    notify.time = event->time;
    notify.requestor = event->requestor;
    notify.selection = event->selection;
    notify.target = event->target;
    notify.property = event->property;

    xcb_send_event(m_conn,
                   false,
                   event->requestor,
                   XCB_EVENT_MASK_NO_EVENT, // SelectionNotify events go without mask
                   reinterpret_cast<const char *>(&notify));
    xcb_flush(m_conn);
}

void Clipboard::handleXcbSelectionRequest(std::shared_ptr<xcb_selection_request_event_t> event) {
    if (event->target == getAtom("TARGETS")) {
        qDebug() << fmt::format("print TARGETS").data();
        setTargets(event->requestor, event->property, m_cachedTargets);
        notifyRequestor(event);

    } else if (event->target == getAtom("MULTIPLE")) {
        // auto data = readProperty(event->requestor, event->property);
        // xcb_atom_t *pairs = reinterpret_cast<xcb_atom_t *>(data.data());
        // size_t pairCnt = data.size() / sizeof(xcb_atom_t);
        // for (size_t i = 0; i < pairCnt; i += 2) {
        //     xcb_atom_t target = pairs[i];
        //     xcb_atom_t property = pairs[i + 1];

        //     if (!setRequestorPropertyWithClipboardContent(event->requestor, property, target)) {
        //         xcb_change_property(m_conn,
        //                             XCB_PROP_MODE_REPLACE,
        //                             event->requestor,
        //                             event->property,
        //                             XCB_ATOM_NONE,
        //                             0,
        //                             0,
        //                             nullptr);
        //     }
        // }

        // notifyRequestor(event);
    } else {
        auto targetName = getTargetName(event->target);
        qDebug() << fmt::format("print target: {}", targetName).data();
        if (std::find(m_cachedTargets.begin(), m_cachedTargets.end(), event->target) ==
            m_cachedTargets.end()) {
            qWarning() << fmt::format("target {} is not exist", event->target).data();

            setRequestorPropertyWithClipboardContent(event->requestor,
                                                     event->property,
                                                     event->target);
            notifyRequestor(event);
            return;
        }

        auto cb = [this, event]() {
            if (m_cachedProperties.find(event->target) == m_cachedProperties.end()) {
                return false;
            }

            setRequestorPropertyWithClipboardContent(event->requestor,
                                                     event->property,
                                                     event->target);
            notifyRequestor(event);
            return true;
        };

        bool succ = cb();
        if (!succ) {
            if (m_observer->onReadClipboardContent(targetName)) {
                m_propertyUpdatedCb.emplace_back(cb);
            }
        }
    }
}

void Clipboard::handleXcbSelectionNotify(std::shared_ptr<xcb_selection_notify_event_t> event) {
    if (event->property == getAtom(ATOM_LIST::CPRT_TARGETS)) {
        m_cachedTargets = getTargets();
        auto targetsName = getTargetsName(m_cachedTargets);
        qDebug() << fmt::format("targets: {}", fmt::join(targetsName, ", ")).data();

        notifyTargetsChanged(targetsName);

    } else if (event->property == getAtom(ATOM_LIST::CPRT_PROPERTY)) {
        m_cachedProperties[event->target] = readProperty(event->requestor, event->property);
        qDebug() << fmt::format("target: {}, content: [{}] {}",
                                getTargetName(event->target),
                                m_cachedProperties[event->target].size(),
                                m_cachedProperties[event->target])
                        .data();

        m_sectionPropertyNotifyCb.remove_if([](auto &cb) -> bool { return cb(); });

        if (m_penddingPrint.empty()) {
            m_printingProperty = false;
        } else {
            printNextPenddingProperty();
        }

    } else if (event->property == getAtom(ATOM_LIST::CPRT_PROPERTIES)) {
        auto data = readProperty(m_dummyWindow, getAtom(ATOM_LIST::CPRT_PROPERTIES));
        xcb_atom_t *pairs = reinterpret_cast<xcb_atom_t *>(data.data());
        size_t pairCnt = data.size() / sizeof(xcb_atom_t);
        for (size_t i = 0; i < pairCnt; i += 2) {
            xcb_atom_t target = pairs[i];
            xcb_atom_t property = pairs[i + 1];

            m_cachedProperties[target] = readProperty(m_dummyWindow, property);
            qDebug() << fmt::format("target: {}, content: {}",
                                    getTargetName(target),
                                    m_cachedProperties[target])
                            .data();
        }

        m_sectionPropertyNotifyCb.remove_if([](auto &cb) -> bool { return cb(); });
    }
}

bool Clipboard::ownSelection() {
    xcb_void_cookie_t cookie = xcb_set_selection_owner_checked(m_conn,
                                                               m_dummyWindow,
                                                               m_selection,
                                                               XCB_CURRENT_TIME);
    auto err = std::unique_ptr<xcb_generic_error_t>(xcb_request_check(m_conn, cookie));
    if (err) {
        return false;
    }

    return true;
}

void Clipboard::newClipboardOwnerTargets(const std::vector<std::string> &targets) {
    decltype(m_cachedTargets) targets_;
    targets_.reserve(targets.size());
    for (const std::string &target : targets) {
        targets_.emplace_back(getAtom(target));
    }

    reset();
    m_cachedTargets.swap(targets_);
    ownSelection();
}

void Clipboard::readTargetContent(
    const std::string &target,
    const std::function<void(const std::vector<char> &data)> &callback) {
    xcb_atom_t targetAtom = getAtom(target);
    auto iter = m_cachedProperties.find(targetAtom);
    if (iter != m_cachedProperties.end()) {
        callback(iter->second);
        return;
    }

    m_sectionPropertyNotifyCb.emplace_back([this, callback, targetAtom]() -> bool {
        auto iter = m_cachedProperties.find(targetAtom);
        if (iter == m_cachedProperties.end()) {
            return false;
        }

        callback(iter->second);
        return true;
    });
    printProperty(targetAtom);
}

void Clipboard::updateTargetContent(const std::string &target, const std::vector<char> &data) {
    auto targetAtom = getAtom(target);
    m_cachedProperties[targetAtom] = data;

    m_propertyUpdatedCb.remove_if([](auto &cb) -> bool { return cb(); });
}

bool Clipboard::isFiles() {
    auto fileAtom = getAtom("x-special/gnome-copied-files");
    for (auto atom : m_cachedTargets) {
        if (atom == fileAtom) {
            return true;
        }
    }

    return false;
}
