#ifndef CALCULATOR_CORE_H
#define CALCULATOR_CORE_H

#include <cstdint>

class CalculatorCore
{
public:
    struct State {
        uint64_t x = 0;
        uint64_t y = 0;
        uint64_t z = 0;
        uint64_t t = 0;
        bool bit32 = false;
    };

    enum Operation {
        Add,
        Subtract,
        Multiply,
        Divide,
        And,
        Or,
        Xor,
        Mod,
        Not,
        ShiftLeft,
        ShiftRight,
        StackUp,
        StackDown,
        StackSwap,
        Enter,
        Clear
    };

    CalculatorCore();

    const State &state() const;
    void setBit32(bool enabled);
    void setX(uint64_t value);
    void setY(uint64_t value);
    void setZ(uint64_t value);
    void setT(uint64_t value);
    void clear();
    void swapEndian();
    bool apply(Operation op);
    int64_t signedValue(uint64_t value) const;
    uint64_t normalize(uint64_t value) const;

private:
    State state_;

    static uint32_t swapUint32(uint32_t value);
    static uint64_t swapUint64(uint64_t value);
};

#endif
