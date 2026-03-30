#include "bincalc.h"
#include "ui_bincalc.h"
#include <QApplication>
#include <QCheckBox>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <unistd.h>
#include <QList>
#include <QScreen>

#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define UNUSED(expr) do { (void)(expr); } while (0)

enum g_button_enum { button_add,
                     button_subtract,
                     button_multiply,
                     button_divide,
                     button_and,
                     button_or,
                     button_xor,
                     button_mod,
                     button_not,
                     button_shift_left,
                     button_shift_right,
                     button_stack_up,
                     button_stack_down,
                     button_stack_swap,
                     button_enter,
                     button_clear,
                   };

enum g_input_enum {
    input_int,
    input_hex,
    input_fixed,
    input_float,
    input_uint,
    input_octal,
    input_fract,
    input_chars
};

using namespace NumericFormats;

namespace {

g_button_enum buttonFromObjectName(const QString &name)
{
    if (name == "button_add") return button_add;
    if (name == "button_subtract") return button_subtract;
    if (name == "button_multiply") return button_multiply;
    if (name == "button_divide") return button_divide;
    if (name == "button_and") return button_and;
    if (name == "button_or") return button_or;
    if (name == "button_xor") return button_xor;
    if (name == "button_mod") return button_mod;
    if (name == "button_not") return button_not;
    if (name == "button_shift_left") return button_shift_left;
    if (name == "button_shift_right") return button_shift_right;
    if (name == "button_stack_up") return button_stack_up;
    if (name == "button_stack_down") return button_stack_down;
    if (name == "button_stack_swap") return button_stack_swap;
    if (name == "button_enter") return button_enter;
    return button_clear;
}

g_input_enum inputFromObjectName(const QString &name)
{
    if (name == "input_x_int") return input_int;
    if (name == "input_x_hex") return input_hex;
    if (name == "input_x_fixed") return input_fixed;
    if (name == "input_x_float") return input_float;
    if (name == "input_x_uint") return input_uint;
    if (name == "input_x_octal") return input_octal;
    if (name == "input_x_fract") return input_fract;
    return input_chars;
}

} // namespace

BinCalc::BinCalc(QWidget *parent): QMainWindow(parent), ui(new Ui::BinCalc) {
    ui->setupUi(this);
    core_.setBit32(false);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                   Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    // //QList<QScreen> screens = QGuiApplication::screens();
    // const auto screens = QGuiApplication::screens();
    // for (int i=0; i < screens.count(); i++) {
    //     printf("%d x %d\n", screens[i]->geometry().x(), screens[i]->geometry().y());
    // }
    
    QApplication::instance()->installEventFilter(this);

    setWindowTitle("Binary Calculator");
    const QSize initialSize(width() * 2, height());
    resize(initialSize);
    setMinimumHeight(600);
    dragHandle_ = new QWidget(this);
    dragHandle_->setObjectName("drag_handle");
    dragHandle_->setMinimumHeight(20);
    dragHandle_->setMaximumHeight(20);
    dragHandle_->setCursor(Qt::OpenHandCursor);
    dragHandle_->setStyleSheet(
        "QWidget#drag_handle {"
        " background: #d8d8d8;"
        " border-bottom: 1px solid #a8a8a8;"
        "}"
    );
    auto *dragLayout = new QVBoxLayout(dragHandle_);
    dragLayout->setContentsMargins(8, 0, 8, 0);
    QLabel *dragLabel = new QLabel(windowTitle(), dragHandle_);
    dragLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    dragLayout->addWidget(dragLabel);
    dragHandle_->installEventFilter(this);
    dragLabel->installEventFilter(this);
    setMenuWidget(dragHandle_);

    // printf("x: %d y: %d\n", QApplication::primaryScreen()->geometry().x(), QApplication::primaryScreen()->geometry().y());
    // printf("%d x %d\n", QApplication::primaryScreen()->geometry().width(), QApplication::primaryScreen()->geometry().height());
    // printf("x: %d y: %d\n", QMainWindow::x(), QMainWindow::y());
    // printf("x: %d y: %d\n", QMainWindow::mapToParent(QMainWindow::pos()).x(), QMainWindow::mapToParent(QMainWindow::pos()).y());
    // printf("%d x %d\n", QMainWindow::width(), QMainWindow::height());

    //// TODO: fix these, maybe??
    ui->input_y_fixed->setEnabled(false);
    ui->input_y_fract->setEnabled(false);
    // issues with float input... set RO
    // ui->input_x_float->setReadOnly(true);
    // ui->input_x_float->setEnabled(false);
    ////

    BuildBitsWindows();
    InitializeButtons();
    InitializeInputs();
    // simulate a click here to 'enable' little endian as default
    // this is due to the hook being set on clicked() as well as
    // the default layout of the bits is big endian. If you want
    // to default big endian just changed to
    // ui->radio_endian_big->setChecked(true)
    ui->radio_endian_little->click();
    ui->radio_bit_64->setChecked(true);
    ui->button_clear->click();
    setMinimumSize(sizeHint());
}

BinCalc::~BinCalc() {
    delete ui;
}

bool BinCalc::eventFilter(QObject *object, QEvent *event) {
    if (IsXBitCheckbox(object)) {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                draggingBits_ = true;
                lastDraggedBitIndex_ = -1;
                ApplyDraggedBit(object);
                return true;
            }
        }

        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                QCheckBox *checkbox = static_cast<QCheckBox *>(object);
                checkbox->setDown(false);
                checkbox->update();
                draggingBits_ = false;
                lastDraggedBitIndex_ = -1;
                return true;
            }
        }
    }

    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QWidget *widget = QApplication::widgetAt(mouseEvent->globalPos());
            if (!IsXBitCheckbox(widget) &&
                widget &&
                (IsXBitCheckbox(widget) ||
                 (ui->x_binary_window && ui->x_binary_window->isAncestorOf(widget)))) {
                draggingBits_ = true;
                lastDraggedBitIndex_ = -1;
                ApplyDraggedBit(widget);
            }
        }
    }

    if (event->type() == QEvent::MouseMove && draggingBits_) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->buttons() & Qt::LeftButton) {
            QWidget *widget = QApplication::widgetAt(mouseEvent->globalPos());
            ApplyDraggedBit(widget);
            if (!widget || !IsXBitCheckbox(widget)) {
                lastDraggedBitIndex_ = -1;
            }
        } else {
            draggingBits_ = false;
            lastDraggedBitIndex_ = -1;
        }
    }

    if (event->type() == QEvent::MouseButtonRelease && draggingBits_) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            draggingBits_ = false;
            lastDraggedBitIndex_ = -1;
        }
    }

    if ((object == dragHandle_ || (dragHandle_ && dragHandle_->isAncestorOf(qobject_cast<QWidget *>(object)))) &&
        (event->type() == QEvent::MouseButtonPress ||
         event->type() == QEvent::MouseMove ||
         event->type() == QEvent::MouseButtonRelease)) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (event->type() == QEvent::MouseButtonPress && mouseEvent->button() == Qt::LeftButton) {
            draggingWindow_ = true;
            dragOffset_ = mouseEvent->globalPos() - frameGeometry().topLeft();
            dragHandle_->setCursor(Qt::ClosedHandCursor);
            return true;
        }

        if (event->type() == QEvent::MouseMove && draggingWindow_ && (mouseEvent->buttons() & Qt::LeftButton)) {
            move(mouseEvent->globalPos() - dragOffset_);
            return true;
        }

        if (event->type() == QEvent::MouseButtonRelease && mouseEvent->button() == Qt::LeftButton) {
            draggingWindow_ = false;
            dragHandle_->setCursor(Qt::OpenHandCursor);
            return true;
        }
    }

    if (event->type() == QEvent::KeyPress) {
        if (qobject_cast<QLineEdit *>(object) &&
            (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Minus ||
             static_cast<QKeyEvent *>(event)->key() == Qt::Key_Plus)) {
            return false;
        }

        QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
        //printf("key: %x object: %s\n", key_event->key(), object->objectName().toUtf8().data());
        switch(key_event->key()) {
            case Qt::Key_Left:
                UpdateXDisplay(core_.state().x << 1);
                // not animating the button because then i can't hold the key
                // maybe there's a better way...
                //ui->button_shift_left->animateClick();
                return true;

            case Qt::Key_Right:
                UpdateXDisplay(static_cast<uint64_t>(SignedValue(core_.state().x) >> 1));
                //ui->button_shift_right->animateClick();
                return true;

            case Qt::Key_Up:
                UpdateXDisplay(core_.state().x + 1);
                return true;

            case Qt::Key_Down:
                UpdateXDisplay(core_.state().x - 1);
                return true;

            case Qt::Key_Asterisk:
                ui->button_multiply->animateClick();
                return true;

            case Qt::Key_Ampersand:
                ui->button_and->animateClick();
                return true;

            case Qt::Key_Bar:
                ui->button_or->animateClick();
                return true;

            case Qt::Key_Slash:
                ui->button_divide->animateClick();
                return true;

            case Qt::Key_Percent:
                ui->button_mod->animateClick();
                return true;

            case Qt::Key_AsciiCircum:
                ui->button_xor->animateClick();
                return true;

            case Qt::Key_Minus:
                // removed so you can type `-` for negative numbers
                //ui->button_subtract->animateClick();
                return false;

            case Qt::Key_Plus:
                // removed so you can type `+` for float input
                //ui->button_add->animateClick();
                return false;

            case Qt::Key_PageDown:
                ui->button_stack_down->animateClick();
                return true;

            case Qt::Key_Enter:
                ui->button_enter->animateClick();
                return true;

            case Qt::Key_Return:
                ui->button_enter->animateClick();
                return true;

            case Qt::Key_PageUp:
                ui->button_stack_up->animateClick();
                return true;

            case Qt::Key_End:
                ui->button_stack_swap->animateClick();
                return true;

            case Qt::Key_AsciiTilde:
                ui->button_not->animateClick();
                return true;
        }
    }

    UNUSED(object);
    return false;
}

void BinCalc::BuildBitsWindows() {
    ui->gridLayout->setColumnStretch(1, 1);
    ui->gridLayout->setColumnStretch(3, 0);
    ui->y_display_window->setColumnStretch(1, 1);
    ui->y_display_window->setColumnStretch(2, 1);
    ui->x_display_window->setColumnStretch(3, 1);
    ui->x_display_window->setColumnStretch(4, 1);
    ui->horizontalLayout_5->setColumnStretch(1, 1);
    ui->y_constraints_window->setColumnStretch(1, 1);

    ui->horizontalLayout_5->setColumnMinimumWidth(0, 32);
    ui->horizontalLayout_5->setColumnMinimumWidth(2, 32);
    ui->horizontalLayout_5->setColumnMinimumWidth(3, 32);
    ui->y_constraints_window->setColumnMinimumWidth(0, 32);
    ui->y_constraints_window->setColumnMinimumWidth(2, 32);
    ui->y_constraints_window->setColumnMinimumWidth(3, 32);

    for (int i=0; i < 64; i++) {
        yBits_[i] = new QCheckBox();
        yBits_[i]->setFixedSize(15, 15);
        yBits_[i]->setStyleSheet(""
                    "QCheckBox::indicator { width: 15px; height: 15px; }"
                    "QCheckBox::indicator:checked{ image: url(:resources/checked_box.png); }"
                    );
        yBits_[i]->setEnabled(false);
    }

    for (int i=0; i < 8; i++) {
        yBytes_[i] = new QHBoxLayout();
        yTopLabelBytes_[i] = new QHBoxLayout();
        yBottomLabelBytes_[i] = new QHBoxLayout();
        yTopLabelBytes_[i]->setContentsMargins(0, 0, 0, 0);
        yBottomLabelBytes_[i]->setContentsMargins(0, 0, 0, 0);
    }

    for (int i=0; i < 64; i++) {
        xBits_[i] = new QCheckBox();
        xBits_[i]->setFixedSize(15, 15);
        xBits_[i]->setStyleSheet(""
                    "QCheckBox::indicator { width: 15px; height: 15px; }"
                    "QCheckBox::indicator:checked{ image: url(:resources/checked_box.png); }"
                    );
    }

    for (int i=0; i < 8; i++) {
        xBytes_[i] = new QHBoxLayout();
        xTopLabelBytes_[i] = new QHBoxLayout();
        xBottomLabelBytes_[i] = new QHBoxLayout();
        xTopLabelBytes_[i]->setContentsMargins(0, 0, 0, 0);
        xBottomLabelBytes_[i]->setContentsMargins(0, 0, 0, 0);
    }

    for (uint32_t i=0; i < ARRAYSIZE(yBytes_); i++) {
        yBytes_[i]->setSpacing(0);
        yTopLabelBytes_[i]->setSpacing(0);
        yBottomLabelBytes_[i]->setSpacing(0);
        yBytes_[i]->setObjectName((QString) "y_byte_" + QString::number(i));
        yTopLabelBytes_[i]->setObjectName((QString) "y_top_label_byte_" + QString::number(i));
        yBottomLabelBytes_[i]->setObjectName((QString) "y_bottom_label_byte_" + QString::number(i));

        for (int z=0; z<8; z++) {
            yBytes_[i]->addWidget(yBits_[z+(i*8)]);
            const int bitNumber = 63 - (z + (i * 8));

            QLabel *top = new QLabel(bitNumber >= 10 ? QString::number(bitNumber / 10) : QString(" "));
            top->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            top->setMinimumWidth(11);
            top->setMaximumHeight(10);
            QLabel *bottom = new QLabel(QString::number(bitNumber % 10));
            bottom->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            bottom->setMinimumWidth(11);
            bottom->setMaximumHeight(10);
            yTopLabelBytes_[i]->addWidget(top);
            yBottomLabelBytes_[i]->addWidget(bottom);
        }

        ui->y_bit_labels_top->addLayout(yTopLabelBytes_[i]);
        ui->y_bits_window->addLayout(yBytes_[i]);
        ui->y_bit_labels_bottom->addLayout(yBottomLabelBytes_[i]);
    }

    for (uint32_t i=0; i < ARRAYSIZE(xBytes_); i++) {
        xBytes_[i]->setSpacing(0);
        xTopLabelBytes_[i]->setSpacing(0);
        xBottomLabelBytes_[i]->setSpacing(0);
        xBytes_[i]->setObjectName((QString) "x_byte_" + QString::number(i));
        xTopLabelBytes_[i]->setObjectName((QString) "x_top_label_byte_" + QString::number(i));
        xBottomLabelBytes_[i]->setObjectName((QString) "x_bottom_label_byte_" + QString::number(i));

        for (int z=0; z < 8; z++) {
            xBytes_[i]->addWidget(xBits_[z+(i*8)]);
            const int bitNumber = 63 - (z + (i * 8));

            QLabel *top = new QLabel(bitNumber >= 10 ? QString::number(bitNumber / 10) : QString(" "));
            top->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            top->setMinimumWidth(11);
            top->setMaximumHeight(10);
            QLabel *bottom = new QLabel(QString::number(bitNumber % 10));
            bottom->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            bottom->setMinimumWidth(11);
            bottom->setMaximumHeight(10);
            xTopLabelBytes_[i]->addWidget(top);
            xBottomLabelBytes_[i]->addWidget(bottom);
        }

        ui->x_bit_labels_top->addLayout(xTopLabelBytes_[i]);
        ui->x_bits_window->addLayout(xBytes_[i]);
        ui->x_bit_labels_bottom->addLayout(xBottomLabelBytes_[i]);
    }

    ui->x_binary_window->setMinimumWidth(ui->x_binary_window->sizeHint().width());
    ui->y_binary_window->setMinimumWidth(ui->y_binary_window->sizeHint().width());

    InitializeBits();
}

void BinCalc::InitializeBits() {
    for (uint32_t i=0; i < 64; i++) {
        xBits_[i]->installEventFilter(this);
        connect(xBits_[i], SIGNAL(clicked(bool)), this, SLOT(BitChanged(bool)));
    }
}

void BinCalc::InitializeButtons() {

    const char *labels[16] = {
        "add", "subtract", "multiply",
        "divide", "and", "or", "xor", "mod",
        "not", "shift_left", "shift_right",
        "stack_up", "stack_down", "stack_swap",
        "enter", "clear"
    };

    // Push Buttons
    QPushButton *buttons[16];
    for (uint32_t i=0; i < ARRAYSIZE(labels); i++) {
        QString buttonName = "button_" + QString::fromUtf8(labels[i]);
        buttons[i] = BinCalc::findChild<QPushButton *>(buttonName);
        connect(buttons[i], SIGNAL(released()), this, SLOT(ButtonPressed()));
    }

    // Radio Buttons
    connect(ui->radio_bit_32, SIGNAL(toggled(bool)), this, SLOT(BitsSettingToggled(bool)));
    connect(ui->radio_bit_64, SIGNAL(toggled(bool)), this, SLOT(BitsSettingToggled(bool)));

    // gross hack...
    connect(ui->radio_endian_little, SIGNAL(toggled(bool)), this, SLOT(EndianSettingToggled(bool)));
    connect(ui->radio_endian_big, SIGNAL(toggled(bool)), this, SLOT(EndianSettingToggled(bool)));
}

void BinCalc::SetInputValidators(bool bits64) {
    QRegularExpression rg_hex64("[0-9a-fA-F]{0,16}");
    QRegularExpression rg_hex32("[0-9a-fA-F]{0,8}");
    QRegularExpression rg_int64("-?[0-9]{0,19}");
    QRegularExpression rg_int32("-?[0-9]{0,10}");
    QRegularExpression rg_uint64("[0-9]{0,20}");
    QRegularExpression rg_uint32("[0-9]{0,10}");
    QRegularExpression rg_octal64("[0-7]{0,22}");
    QRegularExpression rg_octal32("[0-7]{0,11}");
    QRegularExpression rg_chars64("[ -~]{0,8}"); // all ascii characters
    QRegularExpression rg_chars32("[ -~]{0,4}"); // all ascii characters

    QRegularExpression rg_float64("(?i)(?:[-+]?(?:[0-9]+\\.?[0-9]*|\\.[0-9]+)(?:[eE][-+]?[0-9]{0,3})?|[-+]?(?:inf|infinity|nan))");
    QRegularExpression rg_float32("(?i)(?:[-+]?(?:[0-9]+\\.?[0-9]*|\\.[0-9]+)(?:[eE][-+]?[0-9]{0,2})?|[-+]?(?:inf|infinity|nan))");
    QRegularExpression rg_fixed64("[-+]?(?:[0-9]+\\.?[0-9]*|\\.[0-9]+)");
    QRegularExpression rg_fixed32("[-+]?(?:[0-9]+\\.?[0-9]*|\\.[0-9]+)");

    
    QValidator *hexValidator64 = new QRegularExpressionValidator(rg_hex64, this);
    QValidator *intValidator64 = new QRegularExpressionValidator(rg_int64, this);
    QValidator *uintValidator64 = new QRegularExpressionValidator(rg_uint64, this);
    QValidator *octalValidator64 = new QRegularExpressionValidator(rg_octal64, this);
    QValidator *charValidator64 = new QRegularExpressionValidator(rg_chars64, this);
    QValidator *floatValidator64 = new QRegularExpressionValidator(rg_float64, this);
    QValidator *fixedValidator64 = new QRegularExpressionValidator(rg_fixed64, this);
    
    QValidator *hexValidator32 = new QRegularExpressionValidator(rg_hex32, this);
    QValidator *intValidator32 = new QRegularExpressionValidator(rg_int32, this);
    QValidator *uintValidator32 = new QRegularExpressionValidator(rg_uint32, this);
    QValidator *octalValidator32 = new QRegularExpressionValidator(rg_octal32, this);
    QValidator *charValidator32 = new QRegularExpressionValidator(rg_chars32, this);
    QValidator *floatValidator32 = new QRegularExpressionValidator(rg_float32, this);
    QValidator *fixedValidator32 = new QRegularExpressionValidator(rg_fixed32, this);

    if (bits64) {
        ui->input_x_hex->setValidator(hexValidator64);
        ui->input_x_int->setValidator(intValidator64);
        ui->input_x_uint->setValidator(uintValidator64);
        ui->input_x_octal->setValidator(octalValidator64);
        ui->input_x_float->setValidator(floatValidator64);
        ui->input_x_fixed->setValidator(fixedValidator64);
        ui->input_x_fract->setValidator(fixedValidator64);
        ui->input_x_chars->setValidator(charValidator64);
    } else {
        ui->input_x_hex->setValidator(hexValidator32);
        ui->input_x_int->setValidator(intValidator32);
        ui->input_x_uint->setValidator(uintValidator32);
        ui->input_x_octal->setValidator(octalValidator32);
        ui->input_x_float->setValidator(floatValidator32);
        ui->input_x_fixed->setValidator(fixedValidator32);
        ui->input_x_fract->setValidator(fixedValidator32);
        ui->input_x_chars->setValidator(charValidator32);
    }
}

void BinCalc::InitializeInputs() {
    const char *labels[8] = {
        "int", "hex", "fixed", "float",
        "uint", "octal", "fract", "chars"
    };

    for (uint32_t i=0; i < ARRAYSIZE(labels); i++) {
        QString inputName = "input_x_" + QString::fromUtf8(labels[i]);
        xInputs_[i] = BinCalc::findChild<QLineEdit *>(inputName);
        connect(xInputs_[i], SIGNAL(textEdited(QString)), this, SLOT(InputChanged(QString)));
    }

    SetInputValidators(ui->radio_bit_64->isChecked());
}

void BinCalc::ButtonPressed() {
    QPushButton *button = (QPushButton *)sender();
    if (!core_.apply(static_cast<CalculatorCore::Operation>(buttonFromObjectName(button->objectName())))) {
        return;
    }

    RefreshUi();

    if (lastModifiedInput_) {
        lastModifiedInput_->selectAll();
    } else {
        ui->input_x_int->selectAll();
    }
}

void BinCalc::SwapEndianDisplay() {
    // remove each QHBoxLayout (byte) from the bits window
    // reverse their order and sequentially re-add...
    // this is basically reversing the child indexes

    for (uint32_t i=0; i < 8; i++) {
        ui->x_bit_labels_top->removeItem(xTopLabelBytes_[i]);
        ui->x_bits_window->removeItem(xBytes_[i]);
        ui->x_bit_labels_bottom->removeItem(xBottomLabelBytes_[i]);
        ui->y_bit_labels_top->removeItem(yTopLabelBytes_[i]);
        ui->y_bits_window->removeItem(yBytes_[i]);
        ui->y_bit_labels_bottom->removeItem(yBottomLabelBytes_[i]);
    }

    std::reverse(xBytes_.begin(), xBytes_.end());
    std::reverse(yBytes_.begin(), yBytes_.end());
    std::reverse(xTopLabelBytes_.begin(), xTopLabelBytes_.end());
    std::reverse(xBottomLabelBytes_.begin(), xBottomLabelBytes_.end());
    std::reverse(yTopLabelBytes_.begin(), yTopLabelBytes_.end());
    std::reverse(yBottomLabelBytes_.begin(), yBottomLabelBytes_.end());

    for (uint32_t i=0; i < 8; i++) {
        ui->x_bit_labels_top->addLayout(xTopLabelBytes_[i]);
        ui->x_bits_window->addLayout(xBytes_[i]);
        ui->x_bit_labels_bottom->addLayout(xBottomLabelBytes_[i]);
        ui->y_bit_labels_top->addLayout(yTopLabelBytes_[i]);
        ui->y_bits_window->addLayout(yBytes_[i]);
        ui->y_bit_labels_bottom->addLayout(yBottomLabelBytes_[i]);
    }

    // force bool allows us to force update all fields on the toggle of radio
    // button, instead of restricting to update the last edited field. The 
    // we avoid updating the last modified field is to avoid making changes
    // to the current input field while user is typing in it.
    forceRefresh_ = true;
    core_.swapEndian();
    RefreshUi();
    forceRefresh_ = false;

}

void BinCalc::BitsSettingToggled(bool on) {
    if (!on) return;

    core_.setBit32(ui->radio_bit_32->isChecked());
    ui->label_x_float->setText(core_.state().bit32 ? "float32" : "float64");
    ui->label_y_float->setText(core_.state().bit32 ? "float32" : "float64");
    RefreshUi();

    // update input validators for 64bit or 32bit lengths
    SetInputValidators(ui->radio_bit_64->isChecked());
}

void BinCalc::EndianSettingToggled(bool on) {
    if(!on) return;
    SwapEndianDisplay();
}

void BinCalc::BitChanged(bool checked) {
    // we don't actually care about bool, will just always refresh display
    UNUSED(checked);

    // walk bit array and extract complete 64-bit number
    uint64_t result = 0;
    for (uint32_t i=0; i < 64; i++) {
        if (xBits_[63-i]->isChecked()) {
            result = result ^ ((uint64_t)1 << i);
        }
    }

    UpdateXDisplay(result);
}

void BinCalc::InputChanged(const QString &input) {
    bool ok;
    uint64_t tmp;

    if (input.isEmpty() || input == "-" || input == "+" || input == "." || input == "-." || input == "+.") {
        return;
    }

    // loop through all inputs and check which was modified
    for (uint32_t i=0; i < ARRAYSIZE(xInputs_); i++) {
        if (xInputs_[i]->isModified() || forceRefresh_) {
            lastModifiedInput_ = xInputs_[i];

            switch(inputFromObjectName(xInputs_[i]->objectName())) {
            case input_int:
                UpdateXDisplay(input.toLongLong());
                break;

            case input_uint:
                UpdateXDisplay(input.toULongLong());
                break;

            case input_hex:
                tmp = input.toULongLong(&ok, 16);
                if (ok)
                    UpdateXDisplay(tmp);
                break;

            case input_octal:
                tmp = input.toULongLong(&ok, 8);
                if (ok)
                    UpdateXDisplay(tmp);
                break;

            case input_float:
                tmp = parseFloatBits(input, ui->radio_bit_32->isChecked(), &ok);
                if (ok) {
                    UpdateXDisplay(tmp);
                }
                break;

            case input_fixed:
                tmp = parseScaledFixed(input, core_.state().bit32, fractionalBitsForFixed(core_.state().bit32), &ok);
                if (ok) {
                    UpdateXDisplay(tmp);
                }
                break;

            case input_fract:
                tmp = parseScaledFixed(input, core_.state().bit32, fractionalBitsForFract(core_.state().bit32), &ok);
                if (ok) {
                    UpdateXDisplay(tmp);
                }
                break;

            case input_chars:
                UpdateXDisplay(packChars(input, ui->radio_bit_32->isChecked()));
                break;
            }
        }
    }
}

void BinCalc::UpdateXDisplay(uint64_t value) {
    core_.setX(value);
    RefreshUi();
}

void BinCalc::UpdateYDisplay(uint64_t value) {
    core_.setY(value);
    RefreshUi();
}

void BinCalc::RefreshUi() {
    const bool bit_32 = core_.state().bit32;
    const uint64_t value = core_.state().x;
    // update input fields
    for (uint32_t i=0; i < ARRAYSIZE(xInputs_); i++) {
        // don't update current input being typed
        if (!xInputs_[i]->isModified() || forceRefresh_) { 
            switch(inputFromObjectName(xInputs_[i]->objectName())) {
            case input_int:
                xInputs_[i]->setText(QString::number(SignedValue(value)));
                break;

            case input_uint:
                xInputs_[i]->setText(QString::number(value));
                break;

            case input_hex:
                if (bit_32) {
                    xInputs_[i]->setText(QString::number(value, 16).rightJustified(8, '0'));
                } else {
                    xInputs_[i]->setText(QString::number(value, 16).rightJustified(16, '0'));
                }
                break;

            case input_octal:
                xInputs_[i]->setText(QString::number(value, 8));
                break;

            case input_float:
                xInputs_[i]->setText(formatFloatText(value, bit_32));
                break;

            case input_fixed:
                xInputs_[i]->setText(formatScaledFixed(value, bit_32, fractionalBitsForFixed(bit_32)));
                break;

            case input_fract:
                xInputs_[i]->setText(formatScaledFixed(value, bit_32, fractionalBitsForFract(bit_32)));
                break;

            case input_chars:
                if(xInputs_[i]->isModified())
                    break;

                ui->input_x_chars->setText(unpackChars(value, bit_32));
                break;
            }
        }
    }

    if (bit_32) {
        ui->input_y_int->setText(QString::number(static_cast<int32_t>(core_.state().y)));
    } else {
        ui->input_y_int->setText(QString::number(static_cast<int64_t>(core_.state().y)));
    }

    ui->input_y_uint->setText(QString::number(core_.state().y));

    if (bit_32) {
        ui->input_y_hex->setText(QString::number(core_.state().y, 16).rightJustified(8, '0'));
    } else {
        ui->input_y_hex->setText(QString::number(core_.state().y, 16).rightJustified(16, '0'));
    }

    ui->input_y_octal->setText(QString::number(core_.state().y, 8));
    ui->input_y_float->setText(formatFloatText(core_.state().y, bit_32));
    ui->input_y_fixed->setText(formatScaledFixed(core_.state().y, bit_32, fractionalBitsForFixed(bit_32)));
    ui->input_y_fract->setText(formatScaledFixed(core_.state().y, bit_32, fractionalBitsForFract(bit_32)));
    ui->input_y_chars->setText(unpackChars(core_.state().y, bit_32));

    UpdateStackDisplays();
    UpdateBitsDisplay();

    // reset modified flags to detect next change
    for (uint32_t i=0; i < ARRAYSIZE(xInputs_); i++) {
        xInputs_[i]->setModified(false);
    }
}

void BinCalc::UpdateStackDisplays() {
    ui->input_stack_x->setText(QString::number(SignedValue(core_.state().x)));
    ui->input_stack_y->setText(QString::number(SignedValue(core_.state().y)));
    ui->input_stack_z->setText(QString::number(SignedValue(core_.state().z)));
    ui->input_stack_t->setText(QString::number(SignedValue(core_.state().t)));
}

int64_t BinCalc::SignedValue(uint64_t value) const {
    return core_.signedValue(value);
}

uint64_t BinCalc::NormalizeValue(uint64_t value) const {
    return core_.normalize(value);
}

bool BinCalc::IsXBitCheckbox(QObject *object) const {
    return XBitIndex(object) >= 0;
}

int BinCalc::XBitIndex(QObject *object) const {
    for (uint32_t i = 0; i < xBits_.size(); ++i) {
        if (xBits_[i] == object) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

void BinCalc::ApplyDraggedBit(QObject *object) {
    const int bitIndex = XBitIndex(object);
    if (bitIndex < 0 || bitIndex == lastDraggedBitIndex_) {
        return;
    }

    QCheckBox *checkbox = xBits_[static_cast<size_t>(bitIndex)];
    if (!checkbox->isEnabled()) {
        return;
    }

    lastDraggedBitIndex_ = bitIndex;
    checkbox->setChecked(!checkbox->isChecked());
    checkbox->setDown(false);
    checkbox->update();
    BitChanged(checkbox->isChecked());
}

bool BinCalc::IsBitSet(uint64_t n, uint32_t k) {
    if (!k)
        return (n & 1);

    uint64_t new_num = n >> (k);
    return (new_num & 1);
}

void BinCalc::UpdateBitsDisplay() {
    bool bit_32 = core_.state().bit32;
    const uint64_t xValue = core_.state().x;
    const uint64_t yValue = core_.state().y;

    for (uint32_t i=0; i < 64; i++) {
        if(bit_32 && i > 31) {
            xBits_[63-i]->setChecked(false);
            yBits_[63-i]->setChecked(false);
            xBits_[63-i]->setEnabled(false);
            continue;
        }

        // re-enable checkboxes if 64bit
        if(!bit_32 && i > 31) {
            xBits_[63-i]->setEnabled(true);
        }

        if (IsBitSet(xValue, i)) {
            xBits_[63-i]->setChecked(true);
        } else {
            xBits_[63-i]->setChecked(false);
        }

        if (IsBitSet(yValue, i)) {
            yBits_[63-i]->setChecked(true);
        } else {
            yBits_[63-i]->setChecked(false);
        }
    }
}
