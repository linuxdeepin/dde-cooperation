// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

template <void (*construct)(), void (*destruct)()>
typename StaticConstructor<construct, destruct>::constructor StaticConstructor<construct, destruct>::instance;

} // namespace BaseKit
