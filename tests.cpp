#include "calculator_core.h"
#include "numeric_formats.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string &message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

uint32_t floatBits(float value)
{
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

uint64_t doubleBits(double value)
{
    uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void testCoreStackMath()
{
    CalculatorCore core;
    core.setBit32(true);
    core.setX(2);
    core.setY(5);
    core.setZ(9);
    core.setT(12);

    require(core.apply(CalculatorCore::Add), "add should succeed");
    require(core.state().x == 7, "add should store result in x");
    require(core.state().y == 9, "add should drop stack y <- z");
    require(core.state().z == 12, "add should drop stack z <- t");

    require(core.apply(CalculatorCore::Enter), "enter should succeed");
    require(core.state().x == 7, "enter should preserve x");
    require(core.state().y == 7, "enter should copy x into y");
    require(core.state().z == 9, "enter should push previous y into z");
    require(core.state().t == 12, "enter should push previous z into t");
}

void testCoreStackManipulation()
{
    CalculatorCore core;
    core.setBit32(true);
    core.setX(1);
    core.setY(2);
    core.setZ(3);
    core.setT(4);

    require(core.apply(CalculatorCore::Enter), "enter should succeed");
    require(core.state().x == 1 && core.state().y == 1 && core.state().z == 2 && core.state().t == 3,
            "enter should lift stack");

    require(core.apply(CalculatorCore::StackSwap), "swap should succeed");
    require(core.state().x == 1 && core.state().y == 1, "swap should exchange x and y");

    require(core.apply(CalculatorCore::StackDown), "stack down should succeed");
    require(core.state().x == 1 && core.state().y == 2 && core.state().z == 3 && core.state().t == 1,
            "stack down should rotate registers toward x");

    require(core.apply(CalculatorCore::StackUp), "stack up should succeed");
    require(core.state().x == 1 && core.state().y == 1 && core.state().z == 2 && core.state().t == 3,
            "stack up should rotate registers toward t");
}

void testCoreBitWidthAndEndian()
{
    CalculatorCore core;
    core.setBit32(false);
    core.setX(0x1122334455667788ULL);
    core.swapEndian();
    require(core.state().x == 0x8877665544332211ULL, "64-bit endian swap should reverse bytes");

    core.setBit32(true);
    core.setX(0x1122334455667788ULL);
    require(core.state().x == 0x55667788ULL, "bit32 mode should normalize x");
    core.swapEndian();
    require(core.state().x == 0x88776655ULL, "32-bit endian swap should reverse low 32 bits");
}

void testFloatFormats()
{
    bool ok = false;

    const uint64_t parsed32 = NumericFormats::parseFloatBits("1.5", true, &ok);
    require(ok, "float32 parse should succeed");
    require(parsed32 == floatBits(1.5f), "float32 parse should produce IEEE bits");

    const uint64_t parsed64 = NumericFormats::parseFloatBits("1.5", false, &ok);
    require(ok, "float64 parse should succeed");
    require(parsed64 == doubleBits(1.5), "float64 parse should produce IEEE bits");

    require(NumericFormats::formatFloatText(floatBits(-0.0f), true) == "-0", "float32 should preserve negative zero");
    require(NumericFormats::formatFloatText(doubleBits(-0.0), false) == "-0", "float64 should preserve negative zero");

    const QString nanText = NumericFormats::formatFloatText(floatBits(std::numeric_limits<float>::quiet_NaN()), true);
    require(nanText == "nan", "float32 should format nan");

    require(NumericFormats::formatFloatText(floatBits(std::numeric_limits<float>::infinity()), true) == "inf",
            "float32 should format positive infinity");
    require(NumericFormats::formatFloatText(doubleBits(-std::numeric_limits<double>::infinity()), false) == "-inf",
            "float64 should format negative infinity");

    const uint64_t parsedInf = NumericFormats::parseFloatBits("inf", false, &ok);
    require(ok, "float64 inf parse should succeed");
    require(parsedInf == doubleBits(std::numeric_limits<double>::infinity()), "float64 inf parse should produce IEEE bits");

    const uint64_t parsedNegZero = NumericFormats::parseFloatBits("-0", true, &ok);
    require(ok, "float32 negative zero parse should succeed");
    require(parsedNegZero == floatBits(-0.0f), "float32 negative zero should preserve sign bit");

    require(NumericFormats::formatFloatText(0x0000ffffULL, true).startsWith("9.183"),
            "0xffff should have expected float32 reinterpretation");
    require(NumericFormats::formatFloatText(0x000000000000ffffULL, false).startsWith("3.2378592100206092e-319"),
            "0xffff should have expected float64 reinterpretation");
}

void testFixedFormats()
{
    bool ok = false;

    const uint64_t q16 = NumericFormats::parseScaledFixed("1.5", true, NumericFormats::fractionalBitsForFixed(true), &ok);
    require(ok, "Q16.16 parse should succeed");
    require(q16 == 0x00018000ULL, "Q16.16 parse should scale correctly");
    require(NumericFormats::formatScaledFixed(q16, true, NumericFormats::fractionalBitsForFixed(true)).startsWith("1.5"),
            "Q16.16 format should round-trip");

    const uint64_t q31 = NumericFormats::parseScaledFixed("-0.5", true, NumericFormats::fractionalBitsForFract(true), &ok);
    require(ok, "Q1.31 parse should succeed");
    require(q31 == 0xC0000000ULL, "Q1.31 parse should encode signed fraction");

    const uint64_t maxQ16 = NumericFormats::parseScaledFixed("32767.9999847412109375", true, NumericFormats::fractionalBitsForFixed(true), &ok);
    require(ok, "Q16.16 max parse should succeed");
    require(maxQ16 == 0x7fffffffULL, "Q16.16 max parse should hit top signed value");

    NumericFormats::parseScaledFixed("32768", true, NumericFormats::fractionalBitsForFixed(true), &ok);
    require(!ok, "Q16.16 overflow should fail");

    const uint64_t minQ31 = NumericFormats::parseScaledFixed("-1", true, NumericFormats::fractionalBitsForFract(true), &ok);
    require(ok, "Q1.31 -1 parse should succeed");
    require(minQ31 == 0x80000000ULL, "Q1.31 -1 should map to signed minimum");

    const uint64_t maxQ31 = NumericFormats::parseScaledFixed("0.9999999995343387", true, NumericFormats::fractionalBitsForFract(true), &ok);
    require(ok, "Q1.31 near-max parse should succeed");
    require(maxQ31 == 0x7fffffffULL, "Q1.31 near-max should map to signed maximum");
}

void testCharPacking()
{
    const uint64_t packed32 = NumericFormats::packChars("ABCD", true);
    require(packed32 == 0x44434241ULL, "char packing should use low-endian byte order");
    require(NumericFormats::unpackChars(packed32, true) == "ABCD", "char unpack should round-trip 32-bit chars");

    const uint64_t packed64 = NumericFormats::packChars("Hi", false);
    require(NumericFormats::unpackChars(packed64, false).startsWith("Hi"), "char unpack should preserve prefix");
}

}

int main()
{
    try {
        testCoreStackMath();
        testCoreStackManipulation();
        testCoreBitWidthAndEndian();
        testFloatFormats();
        testFixedFormats();
        testCharPacking();
    } catch (const std::exception &ex) {
        std::cerr << "test failure: " << ex.what() << '\n';
        return 1;
    }

    std::cout << "all tests passed\n";
    return 0;
}
