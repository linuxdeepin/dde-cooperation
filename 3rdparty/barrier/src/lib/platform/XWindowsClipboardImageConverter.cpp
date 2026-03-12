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

#include "platform/XWindowsClipboardImageConverter.h"
#include "base/Log.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_LINEAR
#include "stb_image.h"

#include <climits>
#include <memory>

// BMP file header helpers
static inline UInt32 fromLEU32(const UInt8* data)
{
    return static_cast<UInt32>(data[0]) |
           (static_cast<UInt32>(data[1]) <<  8) |
           (static_cast<UInt32>(data[2]) << 16) |
           (static_cast<UInt32>(data[3]) << 24);
}

static void toLE(UInt8*& dst, UInt16 src)
{
    dst[0] = static_cast<UInt8>(src & 0xffu);
    dst[1] = static_cast<UInt8>((src >> 8) & 0xffu);
    dst += 2;
}

static void toLE(UInt8*& dst, UInt32 src)
{
    dst[0] = static_cast<UInt8>(src & 0xffu);
    dst[1] = static_cast<UInt8>((src >>  8) & 0xffu);
    dst[2] = static_cast<UInt8>((src >> 16) & 0xffu);
    dst[3] = static_cast<UInt8>((src >> 24) & 0xffu);
    dst += 4;
}

// Image format detected from magic bytes
enum class ImageFormat { Unknown, PNG, JPEG, BMP };

// RAII deleter for stb_image allocated memory
struct ImageDeleter {
    void operator()(unsigned char* p) const {
        if (p) stbi_image_free(p);
    }
};

// Magic bytes signature check helpers
// PNG: 8-byte signature starting with 0x89 'P' 'N' 'G'
static inline bool isPNGSignature(const UInt8* data) {
    return data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G';
}

// JPEG: SOI (Start of Image) marker 0xFF 0xD8, typically followed by 0xFF
static inline bool isJPEGSignature(const UInt8* data) {
    return data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF;
}

// BMP: 2-byte signature 'B' 'M' (ASCII "BM")
static inline bool isBMPSignature(const UInt8* data) {
    return data[0] == 'B' && data[1] == 'M';
}

// Detect image format from magic bytes (file signatures)
static ImageFormat detectImageFormat(const std::string& data)
{
    if (data.size() < 8) {
        return ImageFormat::Unknown;
    }
    const UInt8* raw = reinterpret_cast<const UInt8*>(data.data());

    if (isPNGSignature(raw)) {
        return ImageFormat::PNG;
    }
    if (isJPEGSignature(raw)) {
        return ImageFormat::JPEG;
    }
    // Minimum valid BMP: 14-byte file header + 40-byte BITMAPINFOHEADER = 54 bytes
    if (isBMPSignature(raw) && data.size() >= 54) {
        return ImageFormat::BMP;
    }
    return ImageFormat::Unknown;
}

// Convert any image format to BMP DIB (BITMAPINFOHEADER + BGR pixel data)
static std::string convertToBMPDIB(const std::string& imageData)
{
    if (imageData.empty()) {
        return {};
    }

    // Check size to prevent truncation when casting to int
    if (imageData.size() > static_cast<size_t>(INT_MAX)) {
        LOG((CLOG_WARN "Image data too large: %zu bytes", imageData.size()));
        return {};
    }

    // Decode image using stb_image with RAII memory management
    int width, height, channels;
    std::unique_ptr<unsigned char[], ImageDeleter> pixels(
        stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(imageData.data()),
            static_cast<int>(imageData.size()),
            &width, &height, &channels, 4)  // Request RGBA output
    );

    if (!pixels) {
        LOG((CLOG_WARN "Failed to decode image: %s", stbi_failure_reason()));
        return {};
    }

    // Validate image dimensions to prevent memory issues
    // MAX_IMAGE_DIMENSION = 16384 (16K) chosen because:
    // - 16K x 16K x 4 bytes (RGBA) = 1GB, a reasonable upper limit for clipboard
    // - Matches common GPU texture size limits
    // - Prevents integer overflow in rowSize calculation (width * 3 must fit in int)
    const int MAX_IMAGE_DIMENSION = 16384;
    if (width <= 0 || height <= 0 ||
        width > MAX_IMAGE_DIMENSION || height > MAX_IMAGE_DIMENSION) {
        return {};
    }

    // Build BMP DIB manually
    // DIB = BITMAPINFOHEADER (40 bytes) + pixel data (BGR, bottom-up, 4-byte aligned)

    // Check for integer overflow when calculating row size
    if (width > (INT_MAX - 3) / 3) {
        return {};
    }
    int rowSize = ((width * 3 + 3) / 4) * 4;  // 4-byte aligned
    int imageSize = rowSize * height;
    // Check for overflow when calculating total size
    if (imageSize > INT_MAX - 40 || imageSize / height != rowSize) {
        return {};
    }
    size_t dibSize = 40 + static_cast<size_t>(imageSize);

    std::vector<UInt8> result(dibSize);
    UInt8* dst = result.data();

    // BITMAPINFOHEADER (40 bytes)
    toLE(dst, static_cast<UInt32>(40));        // biSize
    toLE(dst, static_cast<UInt32>(width));     // biWidth
    toLE(dst, static_cast<UInt32>(height));    // biHeight (positive = bottom-up)
    toLE(dst, static_cast<UInt16>(1));         // biPlanes
    toLE(dst, static_cast<UInt16>(24));        // biBitCount (24bpp BGR)
    toLE(dst, static_cast<UInt32>(0));         // biCompression (BI_RGB)
    toLE(dst, static_cast<UInt32>(imageSize)); // biSizeImage
    toLE(dst, static_cast<UInt32>(0));         // biXPelsPerMeter
    toLE(dst, static_cast<UInt32>(0));         // biYPelsPerMeter
    toLE(dst, static_cast<UInt32>(0));         // biClrUsed
    toLE(dst, static_cast<UInt32>(0));         // biClrImportant

    // Pixel data (BGR, bottom-up) - stb_image returns top-down, BMP needs bottom-up
    // Pre-compute strides to reduce in-loop calculations
    const int srcStride = width * 4;
    const unsigned char* srcRow = pixels.get() + (height - 1) * srcStride;
    UInt8* dstRow = dst;

    for (int y = 0; y < height; y++) {
        const unsigned char* src = srcRow;
        UInt8* dstPixel = dstRow;
        for (int x = 0; x < width; x++) {
            dstPixel[0] = src[2];  // B
            dstPixel[1] = src[1];  // G
            dstPixel[2] = src[0];  // R
            src += 4;
            dstPixel += 3;
        }
        // Row padding (already zero from vector initialization)
        srcRow -= srcStride;
        dstRow += rowSize;
    }

    return std::string(reinterpret_cast<const char*>(result.data()), result.size());
}

//
// XWindowsClipboardImageConverter
//

XWindowsClipboardImageConverter::XWindowsClipboardImageConverter(
                Display* display, const char* atomName) :
    m_atom(XInternAtom(display, atomName, False)),
    m_atomName(atomName)
{
}

XWindowsClipboardImageConverter::~XWindowsClipboardImageConverter()
{
}

IClipboard::EFormat
XWindowsClipboardImageConverter::getFormat() const
{
    return IClipboard::kBitmap;
}

Atom
XWindowsClipboardImageConverter::getAtom() const
{
    return m_atom;
}

int
XWindowsClipboardImageConverter::getDataSize() const
{
    return 8;
}

bool
XWindowsClipboardImageConverter::isBMP() const
{
    return (strcmp(m_atomName, "image/bmp") == 0 ||
            strcmp(m_atomName, "image/x-bmp") == 0 ||
            strcmp(m_atomName, "image/x-MS-bmp") == 0);
}

bool
XWindowsClipboardImageConverter::isPNG() const
{
    return (strcmp(m_atomName, "image/png") == 0);
}

bool
XWindowsClipboardImageConverter::isJPEG() const
{
    return (strcmp(m_atomName, "image/jpeg") == 0 ||
            strcmp(m_atomName, "image/jpg") == 0);
}

std::string
XWindowsClipboardImageConverter::fromIClipboard(const std::string& data) const
{
    // This direction is rarely used: internal clipboard (BMP DIB) -> X11
    // Only support BMP output for simplicity
    if (data.empty() || !isBMP()) {
        return {};
    }

    // BMP needs file header added (DIB -> BMP file)
    if (data.size() <= 40) {
        return {};
    }
    UInt8 header[14];
    UInt8* dst = header;
    toLE(dst, static_cast<UInt16>(0x4D42));  // 'BM'
    toLE(dst, static_cast<UInt32>(14 + data.size()));
    toLE(dst, static_cast<UInt16>(0));
    toLE(dst, static_cast<UInt16>(0));
    toLE(dst, static_cast<UInt32>(14 + 40));
    return std::string(reinterpret_cast<const char*>(header), 14) + data;
}

std::string
XWindowsClipboardImageConverter::toIClipboard(const std::string& data) const
{
    // Main direction: X11 clipboard (PNG/JPEG/BMP) -> internal (BMP DIB) -> remote
    if (data.empty()) {
        return {};
    }

    ImageFormat format = detectImageFormat(data);
    if (format == ImageFormat::Unknown) {
        return {};
    }

    if (format == ImageFormat::BMP) {
        // BMP needs file header stripped (BMP file -> DIB)
        // Validate: size and magic bytes (redundant but defensive)
        const UInt8* raw = reinterpret_cast<const UInt8*>(data.data());
        if (data.size() < 14 + 40 || raw[0] != 'B' || raw[1] != 'M') {
            return {};
        }
        UInt32 offset = fromLEU32(raw + 10);
        // Validate offset is within valid bounds
        if (offset < 14 + 40 || offset > data.size()) {
            return {};
        }
        if (offset == 14 + 40) {
            return data.substr(14);
        }
        return data.substr(14, 40) + data.substr(offset, data.size() - offset);
    }

    // For PNG/JPEG: convert to BMP DIB format for cross-platform compatibility
    return convertToBMPDIB(data);
}
