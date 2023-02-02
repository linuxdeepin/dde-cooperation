// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef X11_CLIPBOARD_H
#define X11_CLIPBOARD_H

#include <vector>
#include <list>
#include <queue>
#include <string>
#include <unordered_map>

#include "X11.h"

#include <xcb/xcb.h>

#include "../ClipboardBase.h"

namespace X11 {

class Clipboard : public X11, public ClipboardBase {
public:
    explicit Clipboard(ClipboardObserver *observer, QObject *parent = nullptr);
    virtual ~Clipboard();

    virtual void handleEvent(std::shared_ptr<xcb_generic_event_t> event) override;

    virtual void newClipboardOwnerTargets(const std::vector<std::string> &targets) override;
    virtual void readTargetContent(
        const std::string &target,
        const std::function<void(const std::vector<char> &data)> &callback) override;
    virtual void updateTargetContent(const std::string &target,
                                     const std::vector<char> &data) override;
    virtual bool isFiles() override;

private:
    xcb_window_t m_dummyWindow;

    uint8_t m_xinput2OPCode;
    const struct xcb_query_extension_reply_t *m_xfixes;

    std::unordered_map<std::string, xcb_atom_t> m_atoms;

    xcb_atom_t m_selection;

    enum class ATOM_LIST {
        CPRT_TARGETS = 0,
        CPRT_PROPERTY,
        CPRT_PROPERTIES,

        ATOM,
        ATOM_PAIR,
        CLIPBOARD,
        TARGETS,
        MULTIPLE,
        STRING,
        UTF8_STRING,
        TEXT,
    };
    static const std::string ATOMS_NAME[];

    std::vector<xcb_atom_t> m_cachedTargets;
    std::unordered_map<xcb_atom_t, std::vector<char>> m_cachedProperties;

    std::list<std::function<bool()>> m_sectionPropertyNotifyCb;
    std::list<std::function<bool()>> m_propertyUpdatedCb;

    std::list<xcb_atom_t> m_penddingPrint;
    bool m_printingProperty;

    xcb_screen_t *screenOfDisplay(int screen);

    xcb_atom_t getAtom(ATOM_LIST atom, bool onlyIfExists = false);
    xcb_atom_t getAtom(const char *name, bool onlyIfExists = false);
    xcb_atom_t getAtom(const std::string &name, bool onlyIfExists = false);

    void initRandrExtension();
    void initXinputExtension();
    void initXfixesExtension();

    void reset();

    std::vector<char> readProperty(xcb_window_t requestor, xcb_atom_t property);

    void printPropertyTargets();
    void printProperty(xcb_atom_t atom);
    void printNextPenddingProperty();
    void printProperties(const std::vector<xcb_atom_t> &pairs);
    std::vector<xcb_atom_t> getTargets();
    std::vector<std::string> getTargetsName(const std::vector<xcb_atom_t> &atoms);
    std::string getTargetName(xcb_atom_t atoms);
    void setTargets(xcb_window_t requestor,
                    xcb_atom_t property,
                    const std::vector<xcb_atom_t> &targets);
    bool setRequestorPropertyWithClipboardContent(const xcb_atom_t requestor,
                                                  const xcb_atom_t property,
                                                  const xcb_atom_t target);
    bool ownSelection();
    void notifyRequestor(std::shared_ptr<xcb_selection_request_event_t> event);

    void handleXcbSelectionRequest(std::shared_ptr<xcb_selection_request_event_t> event);
    void handleXcbSelectionNotify(std::shared_ptr<xcb_selection_notify_event_t> event);
};

} // namespace X11

#endif // !X11_CLIPBOARD_H
