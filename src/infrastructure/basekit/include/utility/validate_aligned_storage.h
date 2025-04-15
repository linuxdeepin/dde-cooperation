// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEKIT_UTILITY_VALIDATE_ALIGNED_STORAGE_H
#define BASEKIT_UTILITY_VALIDATE_ALIGNED_STORAGE_H

namespace BaseKit {

//! Aligned storage validator
template <size_t ImplSize, size_t ImplAlign, size_t StorageSize, size_t StorageAlign, class Enable = void>
class ValidateAlignedStorage;

//! \cond DOXYGEN_SKIP
//! Aligned storage validator (specialization)
template <size_t ImplSize, const size_t ImplAlign, size_t StorageSize, size_t StorageAlign>
class ValidateAlignedStorage<ImplSize, ImplAlign, StorageSize, StorageAlign, typename std::enable_if<(StorageSize >= ImplSize) && ((StorageAlign % ImplAlign) == 0)>::type> {};
//! \endcond

} // namespace BaseKit

#endif // BASEKIT_UTILITY_VALIDATE_ALIGNED_STORAGE_H
