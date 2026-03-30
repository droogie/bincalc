#ifndef BINCALC_H
#define BINCALC_H

#include <array>
#include "calculator_core.h"
#include "numeric_formats.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class BinCalc; }
QT_END_NAMESPACE

class QCheckBox;
class QHBoxLayout;
class QLineEdit;
class QWidget;

class BinCalc : public QMainWindow
{
    Q_OBJECT

public:
    BinCalc(QWidget *parent = nullptr);
    ~BinCalc();

    virtual bool eventFilter(QObject *object, QEvent *event);

private:
    Ui::BinCalc *ui;
    CalculatorCore core_;
    std::array<QLineEdit *, 8> xInputs_ {};
    std::array<QCheckBox *, 64> xBits_ {};
    std::array<QCheckBox *, 64> yBits_ {};
    std::array<QHBoxLayout *, 8> xBytes_ {};
    std::array<QHBoxLayout *, 8> yBytes_ {};
    std::array<QHBoxLayout *, 8> xTopLabelBytes_ {};
    std::array<QHBoxLayout *, 8> xBottomLabelBytes_ {};
    std::array<QHBoxLayout *, 8> yTopLabelBytes_ {};
    std::array<QHBoxLayout *, 8> yBottomLabelBytes_ {};
    QLineEdit *lastModifiedInput_ = nullptr;
    QWidget *dragHandle_ = nullptr;
    bool forceRefresh_ = false;
    bool draggingWindow_ = false;
    bool draggingBits_ = false;
    int lastDraggedBitIndex_ = -1;
    QPoint dragOffset_;

private slots:
    void BuildBitsWindows();
    void InitializeBits();
    void InitializeButtons();
    void InitializeInputs();
    void ButtonPressed();
    void BitsSettingToggled(bool);
    void EndianSettingToggled(bool);
    void BitChanged(bool);
    void InputChanged(const QString&);
    void UpdateXDisplay(uint64_t);
    void UpdateYDisplay(uint64_t);
    bool IsBitSet(uint64_t, uint32_t);
    void UpdateBitsDisplay();
    void SwapEndianDisplay();
    void SetInputValidators(bool);
    void RefreshUi();
    void UpdateStackDisplays();
    int64_t SignedValue(uint64_t) const;
    uint64_t NormalizeValue(uint64_t) const;
    bool IsDarkTheme() const;
    void ApplyTheme();
    void SetupPlatformChrome();
    bool IsXBitCheckbox(QObject *object) const;
    int XBitIndex(QObject *object) const;
    void ApplyDraggedBit(QObject *object);
};
#endif // BINCALC_H
