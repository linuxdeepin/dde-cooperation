/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2024-2026 UnionTech Software Technology Co., Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "platform/XWindowsClipboard.h"

//! Convert to/from various image formats
/*!
Handles image clipboard data. Supports BMP (strips/adds file header),
PNG, and JPEG formats. Converts all formats to BMP DIB for cross-platform
compatibility.
*/
class XWindowsClipboardImageConverter : public IXWindowsClipboardConverter {
public:
    XWindowsClipboardImageConverter(Display* display, const char* atomName);
    virtual ~XWindowsClipboardImageConverter();

    // IXWindowsClipboardConverter overrides
    virtual IClipboard::EFormat getFormat() const;
    virtual Atom getAtom() const;
    virtual int getDataSize() const;
    virtual std::string fromIClipboard(const std::string&) const;
    virtual std::string toIClipboard(const std::string&) const;

private:
    bool isBMP() const;
    bool isPNG() const;
    bool isJPEG() const;

    Atom m_atom;
    const char* m_atomName;
};
