// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef X11_DISPLAY_H
#define X11_DISPLAY_H

#include <memory>

#include "../DisplayBase.h"
#include "X11.h"

#include <xcb/xcb.h>

namespace X11 {

class Display : public X11, public DisplayBase {
public:
    explicit Display(Manager *manager, QObject *parent = nullptr);
    virtual ~Display();

    virtual void handleEvent(std::shared_ptr<xcb_generic_event_t> event) override;

protected:
    virtual void hideMouse(bool hide) override;
    virtual void moveMouse(uint16_t x, uint16_t y) override;

private:
    uint8_t m_xinput2OPCode;
    const struct xcb_query_extension_reply_t *m_xfixes;

    void initRandrExtension();
    void initXinputExtension();
    void initXfixesExtension();
};

} // namespace X11

#endif // !X11_DISPLAY_H
