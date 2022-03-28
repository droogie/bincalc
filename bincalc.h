#ifndef BINCALC_H
#define BINCALC_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class BinCalc; }
QT_END_NAMESPACE

class BinCalc : public QMainWindow
{
    Q_OBJECT

public:
    BinCalc(QWidget *parent = nullptr);
    ~BinCalc();

    virtual void paintEvent(QPaintEvent *event);
    virtual bool eventFilter(QObject *object, QEvent *event);

private:
    Ui::BinCalc *ui;

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
    void convertToCharArray(uchar *arr, int64_t a);
    void SwapEndianDisplay();
};
#endif // BINCALC_H
