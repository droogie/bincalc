#include "calculator_core.h"

namespace {

uint64_t activeMask(bool bit32)
{
    return bit32 ? 0xffffffffULL : 0xffffffffffffffffULL;
}

uint64_t normalizeWord(uint64_t value, bool bit32)
{
    return value & activeMask(bit32);
}

int64_t wordToSigned(uint64_t value, bool bit32)
{
    return bit32 ? static_cast<int64_t>(static_cast<int32_t>(value))
                 : static_cast<int64_t>(value);
}

}

CalculatorCore::CalculatorCore() = default;

const CalculatorCore::State &CalculatorCore::state() const
{
    return state_;
}

void CalculatorCore::setBit32(bool enabled)
{
    state_.bit32 = enabled;
    state_.x = normalize(state_.x);
    state_.y = normalize(state_.y);
    state_.z = normalize(state_.z);
    state_.t = normalize(state_.t);
}

void CalculatorCore::setX(uint64_t value)
{
    state_.x = normalize(value);
}

void CalculatorCore::setY(uint64_t value)
{
    state_.y = normalize(value);
}

void CalculatorCore::setZ(uint64_t value)
{
    state_.z = normalize(value);
}

void CalculatorCore::setT(uint64_t value)
{
    state_.t = normalize(value);
}

void CalculatorCore::clear()
{
    state_.x = 0;
    state_.y = 0;
    state_.z = 0;
    state_.t = 0;
}

void CalculatorCore::swapEndian()
{
    if (state_.bit32) {
        state_.x = swapUint32(state_.x);
        state_.y = swapUint32(state_.y);
        state_.z = swapUint32(state_.z);
        state_.t = swapUint32(state_.t);
    } else {
        state_.x = swapUint64(state_.x);
        state_.y = swapUint64(state_.y);
        state_.z = swapUint64(state_.z);
        state_.t = swapUint64(state_.t);
    }
}

bool CalculatorCore::apply(Operation op)
{
    const uint64_t mask = activeMask(state_.bit32);
    const uint64_t xWord = state_.x;
    const uint64_t yWord = state_.y;
    const int64_t x = signedValue(xWord);
    const int64_t y = signedValue(yWord);

    switch (op) {
    case Add:
        state_.x = (xWord + yWord) & mask;
        state_.y = state_.z;
        state_.z = state_.t;
        return true;
    case Subtract:
        state_.x = (yWord - xWord) & mask;
        state_.y = state_.z;
        state_.z = state_.t;
        return true;
    case Multiply:
        state_.x = (xWord * yWord) & mask;
        state_.y = state_.z;
        state_.z = state_.t;
        return true;
    case Divide:
        if (!x) return false;
        state_.x = static_cast<uint64_t>(y / x) & mask;
        state_.y = state_.z;
        state_.z = state_.t;
        return true;
    case And:
        state_.x = (yWord & xWord) & mask;
        state_.y = state_.z;
        state_.z = state_.t;
        return true;
    case Or:
        state_.x = (yWord | xWord) & mask;
        state_.y = state_.z;
        state_.z = state_.t;
        return true;
    case Xor:
        state_.x = (yWord ^ xWord) & mask;
        state_.y = state_.z;
        state_.z = state_.t;
        return true;
    case Mod:
        if (!x) return false;
        state_.x = static_cast<uint64_t>(y % x) & mask;
        state_.y = state_.z;
        state_.z = state_.t;
        return true;
    case Not:
        state_.x = normalize(~xWord);
        return true;
    case ShiftLeft:
        state_.x = normalize(xWord << 1);
        return true;
    case ShiftRight:
        state_.x = normalize(static_cast<uint64_t>(x >> 1));
        return true;
    case StackUp: {
        const uint64_t oldT = state_.t;
        state_.t = state_.z;
        state_.z = state_.y;
        state_.y = state_.x;
        state_.x = oldT;
        return true;
    }
    case StackDown: {
        const uint64_t oldX = state_.x;
        state_.x = state_.y;
        state_.y = state_.z;
        state_.z = state_.t;
        state_.t = oldX;
        return true;
    }
    case StackSwap:
        state_.x = yWord;
        state_.y = xWord;
        return true;
    case Enter:
        state_.t = state_.z;
        state_.z = state_.y;
        state_.y = state_.x;
        return true;
    case Clear:
        clear();
        return true;
    }

    return false;
}

int64_t CalculatorCore::signedValue(uint64_t value) const
{
    return wordToSigned(value, state_.bit32);
}

uint64_t CalculatorCore::normalize(uint64_t value) const
{
    return normalizeWord(value, state_.bit32);
}

uint32_t CalculatorCore::swapUint32(uint32_t value)
{
    return ((value >> 24) |
            ((value & 0x00FF0000) >> 8) |
            ((value & 0x0000FF00) << 8) |
            (value << 24));
}

uint64_t CalculatorCore::swapUint64(uint64_t value)
{
    return ((value << 56) |
            ((value << 40) & 0xFF000000000000ULL) |
            ((value << 24) & 0xFF0000000000ULL) |
            ((value << 8) & 0xFF00000000ULL) |
            ((value >> 8) & 0xFF000000ULL) |
            ((value >> 24) & 0xFF0000ULL) |
            ((value >> 40) & 0xFF00ULL) |
            (value >> 56));
}
