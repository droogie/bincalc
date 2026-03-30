#include "numeric_formats.h"

#include <QLocale>
#include <cmath>
#include <cctype>
#include <cstring>
#include <limits>

namespace {

uint64_t normalizeWord(uint64_t value, bool bit32)
{
    return bit32 ? (value & 0xffffffffULL) : value;
}

int64_t wordToSigned(uint64_t value, bool bit32)
{
    return bit32 ? static_cast<int64_t>(static_cast<int32_t>(value))
                 : static_cast<int64_t>(value);
}

}

namespace NumericFormats {

int fractionalBitsForFixed(bool bit32)
{
    return bit32 ? 16 : 32;
}

int fractionalBitsForFract(bool bit32)
{
    return bit32 ? 31 : 63;
}

QString formatFloatText(uint64_t value, bool bit32)
{
    if (bit32) {
        const uint32_t bits = static_cast<uint32_t>(value);
        float fp = 0.0f;
        std::memcpy(&fp, &bits, sizeof(fp));
        if (std::isnan(fp)) return "nan";
        if (std::isinf(fp)) return std::signbit(fp) ? "-inf" : "inf";
        if (fp == 0.0f && std::signbit(fp)) return "-0";
        return QLocale::c().toString(fp, 'g', std::numeric_limits<float>::max_digits10);
    }

    double fp = 0.0;
    std::memcpy(&fp, &value, sizeof(fp));
    if (std::isnan(fp)) return "nan";
    if (std::isinf(fp)) return std::signbit(fp) ? "-inf" : "inf";
    if (fp == 0.0 && std::signbit(fp)) return "-0";
    return QLocale::c().toString(fp, 'g', std::numeric_limits<double>::max_digits10);
}

uint64_t parseFloatBits(const QString &input, bool bit32, bool *ok)
{
    const QString trimmed = input.trimmed().toLower();
    double parsed = 0.0;

    if (trimmed == "inf" || trimmed == "+inf" || trimmed == "infinity" || trimmed == "+infinity") {
        parsed = std::numeric_limits<double>::infinity();
        *ok = true;
    } else if (trimmed == "-inf" || trimmed == "-infinity") {
        parsed = -std::numeric_limits<double>::infinity();
        *ok = true;
    } else if (trimmed == "nan" || trimmed == "+nan" || trimmed == "-nan") {
        parsed = std::numeric_limits<double>::quiet_NaN();
        *ok = true;
    } else {
        parsed = QLocale::c().toDouble(input, ok);
        if (!*ok) {
            return 0;
        }
    }

    if (bit32) {
        const float fp = static_cast<float>(parsed);
        uint32_t bits = 0;
        std::memcpy(&bits, &fp, sizeof(bits));
        return bits;
    }

    uint64_t bits = 0;
    std::memcpy(&bits, &parsed, sizeof(bits));
    return bits;
}

QString formatScaledFixed(uint64_t value, bool bit32, int fractionalBits)
{
    const qint64 signedValue = wordToSigned(value, bit32);
    const long double scale = std::ldexp(1.0L, fractionalBits);
    const long double scaled = static_cast<long double>(signedValue) / scale;
    return QLocale::c().toString(static_cast<double>(scaled), 'g', 18);
}

uint64_t parseScaledFixed(const QString &input, bool bit32, int fractionalBits, bool *ok)
{
    const double parsed = QLocale::c().toDouble(input, ok);
    if (!*ok || !std::isfinite(parsed)) {
        return 0;
    }

    const long double scaled = static_cast<long double>(parsed) * std::ldexp(1.0L, fractionalBits);
    const long double minValue = bit32
        ? static_cast<long double>(std::numeric_limits<int32_t>::min())
        : static_cast<long double>(std::numeric_limits<int64_t>::min());
    const long double maxValue = bit32
        ? static_cast<long double>(std::numeric_limits<int32_t>::max())
        : static_cast<long double>(std::numeric_limits<int64_t>::max());

    if (scaled < minValue || scaled > maxValue) {
        *ok = false;
        return 0;
    }

    const int64_t raw = static_cast<int64_t>(std::llround(scaled));
    return normalizeWord(static_cast<uint64_t>(raw), bit32);
}

uint64_t packChars(const QString &input, bool bit32)
{
    const QByteArray bytes = input.toUtf8();
    const qsizetype maxChars = bit32 ? qsizetype(4) : qsizetype(8);
    const int count = std::min(static_cast<int>(bytes.size()), static_cast<int>(maxChars));
    uint64_t value = 0;

    for (int i = 0; i < count; ++i) {
        value |= static_cast<uint64_t>(static_cast<unsigned char>(bytes[i])) << (8 * i);
    }

    return value;
}

QString unpackChars(uint64_t value, bool bit32)
{
    const int count = bit32 ? 4 : 8;
    char ascii[9] = {0};

    for (int i = 0; i < count; ++i) {
        const unsigned char ch = static_cast<unsigned char>((value >> (8 * i)) & 0xffu);
        ascii[i] = std::isprint(ch) ? static_cast<char>(ch) : '.';
    }

    return QString::fromLatin1(ascii, count);
}

}
