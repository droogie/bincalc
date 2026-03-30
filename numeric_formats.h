#ifndef NUMERIC_FORMATS_H
#define NUMERIC_FORMATS_H

#include <QString>
#include <cstdint>

namespace NumericFormats {

int fractionalBitsForFixed(bool bit32);
int fractionalBitsForFract(bool bit32);
QString formatFloatText(uint64_t value, bool bit32);
uint64_t parseFloatBits(const QString &input, bool bit32, bool *ok);
QString formatScaledFixed(uint64_t value, bool bit32, int fractionalBits);
uint64_t parseScaledFixed(const QString &input, bool bit32, int fractionalBits, bool *ok);
uint64_t packChars(const QString &input, bool bit32);
QString unpackChars(uint64_t value, bool bit32);

}

#endif
