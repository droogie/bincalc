#include "bincalc.h"
#include "ui_bincalc.h"
#include <QCheckBox>
#include <QRegularExpressionValidator>
#include <QPainter>
#include <QKeyEvent>
#include <map>
#include <unistd.h>

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

std::map<QString, g_button_enum> g_buttonMap;
std::map<QString, g_input_enum> g_inputMap;

QLineEdit *g_x_inputs[8];
QCheckBox *g_x_bits[64];
QCheckBox *g_y_bits[64];
QHBoxLayout *g_x_bytes[8];
QHBoxLayout *g_y_bytes[8];
QRadioButton *g_last_button; // gross workaround because I'm stupid....
QLineEdit *g_last_modified = NULL;
bool g_force = false;

uint64_t swap_uint64( uint64_t val )
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
}

BinCalc::BinCalc(QWidget *parent): QMainWindow(parent), ui(new Ui::BinCalc) {
    ui->setupUi(this);

    QApplication::instance()->installEventFilter(this);

    setWindowTitle("Binary Calculator");
    setFixedSize(width()*2, height());

    //// TODO: fix these, maybe??
    // disabled until decide on if wanted or not...
    ui->input_x_fixed->setEnabled(false);
    ui->input_x_fract->setEnabled(false);
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
}

BinCalc::~BinCalc() {
    delete ui;
}

bool BinCalc::eventFilter(QObject *object, QEvent *event) {
    UNUSED(object);

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
        //printf("key: %x object: %s\n", key_event->key(), object->objectName().toUtf8().data());
        switch(key_event->key()) {
            case Qt::Key_Left:
                UpdateXDisplay(ui->input_x_int->text().toLongLong() << 1);
                // not animating the button because then i can't hold the key
                // maybe there's a better way...
                //ui->button_shift_left->animateClick();
                return true;

            case Qt::Key_Right:
                UpdateXDisplay(ui->input_x_int->text().toLongLong() >> 1);
                //ui->button_shift_right->animateClick();
                return true;

            case Qt::Key_Up:
                UpdateXDisplay(ui->input_x_int->text().toLongLong() + 1);
                return true;

            case Qt::Key_Down:
                UpdateXDisplay(ui->input_x_int->text().toLongLong() - 1);
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
                return true;

            case Qt::Key_Plus:
                // removed so you can type `+` for float input
                //ui->button_add->animateClick();
                return true;

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

    return false;
}

void BinCalc::paintEvent(QPaintEvent *event)
{
    // only used to draw bits text so we don't care about specific events
    UNUSED(event);

    QPainter painter(this);

    uint64_t z = 0;
    for (uint32_t i=0; i < 64; i++) {
        QPoint x_pos = g_x_bits[63-i]->pos();
        QPoint y_pos = g_x_bits[63-i]->pos();
        x_pos.setX(x_pos.x() + 48);
        x_pos.setY(x_pos.y() + 465);
        
        y_pos.setX(y_pos.x() + 48);
        y_pos.setY(y_pos.y() + 205);

        // draw bottom row
        if (z > 9) {
            z = 0;
        }

        painter.drawText(x_pos, QString::number(z));
        painter.drawText(y_pos, QString::number(z));
        z++;

        // draw top row
        x_pos.setY(x_pos.y() - 15);
        y_pos.setY(y_pos.y() - 15);
        if (i > 9 && i < 20) {
            painter.drawText(x_pos, QString::number(1));
            painter.drawText(y_pos, QString::number(1));
        } else if (i >= 20 && i < 30) {
            painter.drawText(x_pos, QString::number(2));
            painter.drawText(y_pos, QString::number(2));
        } else if (i >= 30 && i < 40) {
            painter.drawText(x_pos, QString::number(3));
            painter.drawText(y_pos, QString::number(3));
        } else if (i >= 40 && i < 50) {
            painter.drawText(x_pos, QString::number(4));
            painter.drawText(y_pos, QString::number(4));
        } else if (i >= 50 && i < 60) {
            painter.drawText(x_pos, QString::number(5));
            painter.drawText(y_pos, QString::number(5));
        } else if (i >= 60 && i < 64) {
            painter.drawText(x_pos, QString::number(6));
            painter.drawText(y_pos, QString::number(6));
        }
    }
}

void BinCalc::BuildBitsWindows() {
    // this is a hack to make the y-bits-window match the same width
    // as the x-bits-window. I just add push buttons same properties of 
    // the <<, >>, ~ buttons and hide them
    QSizePolicy sp_retain = ui->padding_0->sizePolicy();
    sp_retain.setRetainSizeWhenHidden(true);
    ui->padding_0->setSizePolicy(sp_retain);
    ui->padding_0->setVisible(false);

    sp_retain = ui->padding_1->sizePolicy();
    sp_retain.setRetainSizeWhenHidden(true);
    ui->padding_1->setSizePolicy(sp_retain);
    ui->padding_1->setVisible(false);

    sp_retain = ui->padding_2->sizePolicy();
    sp_retain.setRetainSizeWhenHidden(true);
    ui->padding_2->setSizePolicy(sp_retain);
    ui->padding_2->setVisible(false);

    for (int i=0; i < 64; i++) {
        g_y_bits[i] = new QCheckBox();
        g_y_bits[i]->setStyleSheet(""
                    "QCheckBox::indicator { width: 15px; height: 15px; }"
                    "QCheckBox::indicator:checked{ image: url(:resources/checked_box.png); }"
                    //"QCheckBox::indicator:unchecked{ image: url(:resources/unchecked_box.png); }"
                    );
        g_y_bits[i]->setEnabled(false);
    }

    for (int i=0; i < 8; i++) {
        g_y_bytes[i] = new QHBoxLayout();
    }

    for (int i=0; i < 64; i++) {
        g_x_bits[i] = new QCheckBox();
        g_x_bits[i]->setStyleSheet(""
                    "QCheckBox::indicator { width: 15px; height: 15px; }"
                    "QCheckBox::indicator:checked{ image: url(:resources/checked_box.png); }"
                    //"QCheckBox::indicator:unchecked{ image: url(:resources/unchecked_box.png); }"
                    );
    }

    for (int i=0; i < 8; i++) {
        g_x_bytes[i] = new QHBoxLayout();
    }

    for (uint32_t i=0; i < ARRAYSIZE(g_y_bytes); i++) {
        g_y_bytes[i]->setSpacing(0);
        g_y_bytes[i]->setObjectName((QString) "y_byte_" + QString::number(i));

        for (int z=0; z<8; z++) {
            g_y_bytes[i]->addWidget(g_y_bits[z+(i*8)]);
        }

        ui->y_bits_window->addLayout(g_y_bytes[i]);
    }

    for (uint32_t i=0; i < ARRAYSIZE(g_x_bytes); i++) {
        g_x_bytes[i]->setSpacing(0);
        g_x_bytes[i]->setObjectName((QString) "x_byte_" + QString::number(i));

        for (int z=0; z < 8; z++) {
            g_x_bytes[i]->addWidget(g_x_bits[z+(i*8)]);
        }

        ui->x_bits_window->addLayout(g_x_bytes[i]);
    }

    InitializeBits();
}

void BinCalc::InitializeBits() {
    for (uint32_t i=0; i < 64; i++) {
        connect(g_x_bits[i], SIGNAL(clicked(bool)), this, SLOT(BitChanged(bool)));
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
        g_buttonMap[buttonName] = g_button_enum(i);
        buttons[i] = BinCalc::findChild<QPushButton *>(buttonName);
        connect(buttons[i], SIGNAL(released()), this, SLOT(ButtonPressed()));
    }

    // Radio Buttons
    connect(ui->radio_bit_32, SIGNAL(toggled(bool)), this, SLOT(BitsSettingToggled(bool)));
    connect(ui->radio_bit_64, SIGNAL(toggled(bool)), this, SLOT(BitsSettingToggled(bool)));

    // gross hack...
    connect(ui->radio_endian_little, SIGNAL(clicked(bool)), this, SLOT(EndianSettingToggled(bool)));
    connect(ui->radio_endian_big, SIGNAL(clicked(bool)), this, SLOT(EndianSettingToggled(bool)));
}

void BinCalc::InitializeInputs() {
    const char *labels[8] = {
        "int", "hex", "fixed", "float",
        "uint", "octal", "fract", "chars"
    };

    for (uint32_t i=0; i < ARRAYSIZE(labels); i++) {
        QString inputName = "input_x_" + QString::fromUtf8(labels[i]);
        g_x_inputs[i] = BinCalc::findChild<QLineEdit *>(inputName);
        g_inputMap[inputName] = g_input_enum(i);
        connect(g_x_inputs[i], SIGNAL(textEdited(QString)), this, SLOT(InputChanged(QString)));
    }

    QRegularExpression rg_hex64("[0-9a-f]{0,16}");
    QRegularExpression rg_hex32("[0-9a-f]{0,8}");
    QRegularExpression rg_int64("-?[0-9]{0,19}");
    QRegularExpression rg_int32("-?[0-9]{0,10}");
    QRegularExpression rg_float64("[-+]?[0-9]*\\.?[0-9]+");
    QRegularExpression rg_chars("[ -~]{0,8}"); // all ascii characters
    QValidator *hexValidator = new QRegularExpressionValidator(rg_hex64, this);
    QValidator *intValidator = new QRegularExpressionValidator(rg_int64, this);
    QValidator *charValidator = new QRegularExpressionValidator(rg_chars, this);
    QValidator *floatValidator = new QRegularExpressionValidator(rg_float64, this);
    ui->input_x_hex->setValidator(hexValidator);
    ui->input_x_int->setValidator(intValidator);
    ui->input_x_uint->setValidator(intValidator);
    ui->input_x_octal->setValidator(intValidator);
    ui->input_x_float->setValidator(floatValidator);
    ui->input_x_chars->setValidator(charValidator);
}

void BinCalc::ButtonPressed() {
    QPushButton *button = (QPushButton *)sender();
    int64_t x = ui->input_x_int->text().toLongLong();
    int64_t y = ui->input_y_int->text().toLongLong();
    int64_t z = ui->input_stack_z->text().toLongLong();
    int64_t t = ui->input_stack_t->text().toLongLong();

    switch(g_buttonMap[button->objectName()]) {

    case button_add:
        UpdateXDisplay(y+x);
        UpdateYDisplay(0);
        break;

    case button_subtract:
        UpdateXDisplay(y-x);
        UpdateYDisplay(0);
        break;

    case button_multiply:
        UpdateXDisplay(y*x);
        UpdateYDisplay(0);
        break;

    case button_divide:
        if(!x)
            break;

        UpdateXDisplay(y/x);
        UpdateYDisplay(0);
        break;

    case button_and:
        UpdateXDisplay(y&x);
        UpdateYDisplay(0);
        break;

    case button_or:
        UpdateXDisplay(y|x);
        UpdateYDisplay(0);
        break;

    case button_xor:
        UpdateXDisplay(y^x);
        UpdateYDisplay(0);
        break;

    case button_mod:
        if(!x)
            break;

        UpdateXDisplay(y%x);
        UpdateYDisplay(0);
        break;

    case button_not:
        UpdateXDisplay(~x);
        break;

    case button_shift_left:
        UpdateXDisplay(x << 1);
        break;

    case button_shift_right:
        UpdateXDisplay(x >> 1);
        break;

    case button_stack_up:
        ui->input_stack_t->setText(QString::number(z));
        ui->input_stack_z->setText(QString::number(y));
        UpdateYDisplay(x);
        UpdateXDisplay(t);
        break;

    case button_stack_down:
        ui->input_stack_t->setText(QString::number(x));
        ui->input_stack_z->setText(QString::number(t));
        UpdateYDisplay(z);
        UpdateXDisplay(y);
        break;

    case button_stack_swap:
        // swap X and Y
        UpdateXDisplay(y);
        UpdateYDisplay(x);
        break;

    case button_enter:
        UpdateYDisplay(x);
        break;

    case button_clear:
        // clear rest of stack
        UpdateXDisplay(0);
        UpdateYDisplay(0);
        ui->input_stack_t->setText(QString::number(0));
        ui->input_stack_z->setText(QString::number(0));
        break;

    default:
        printf("case: %d, %s\n", g_buttonMap[button->objectName()], button->objectName().toUtf8().data());
        break;
    }

    if (g_last_modified) {
        g_last_modified->selectAll();
    } else {
        ui->input_x_int->selectAll();
    }
}

void BinCalc::SwapEndianDisplay() {
    // remove each QHBoxLayout (byte) from the bits window
    // reverse their order and sequentially re-add...
    // this is basically reversing the child indexes

    for (uint32_t i=0; i < 8; i++) {
        ui->x_bits_window->removeItem(g_x_bytes[i]);
        ui->y_bits_window->removeItem(g_y_bytes[i]);
    }

    std::reverse(std::begin(g_x_bytes), std::end(g_x_bytes));
    std::reverse(std::begin(g_y_bytes), std::end(g_y_bytes));

    for (uint32_t i=0; i < 8; i++) {
        ui->x_bits_window->addLayout(g_x_bytes[i]);
        ui->y_bits_window->addLayout(g_y_bytes[i]);
    }

    // force bool allows us to force update all fields on the toggle of radio
    // button, instead of restricting to update the last edited field. The 
    // we avoid updating the last modified field is to avoid making changes
    // to the current input field while user is typing in it.
    g_force = true;
    uint64_t value = ui->input_x_uint->text().toULongLong();
    UpdateXDisplay(swap_uint64(value));
    g_force = false;

    ui->mainwidget->update();
}

void BinCalc::BitsSettingToggled(bool on) {
    if (!on) return;

    UpdateXDisplay(ui->input_x_int->text().toLongLong());
    UpdateYDisplay(ui->input_y_int->text().toLongLong());
}

void BinCalc::EndianSettingToggled(bool on) {
    QRadioButton *button = (QRadioButton *)sender();

    // I could not for the life of me get a simple toggle check working
    // instead i'm toggling on "clicks" but keeping track with a global
    // so that the user can't repeatedly click the same button causing
    // a toggle....

    if(!on) return;

    if(button == g_last_button) return;

    if (button->isChecked()) {
        SwapEndianDisplay();
        g_last_button = button;
    }
}

void BinCalc::BitChanged(bool checked) {
    // we don't actually care about bool, will just always refresh display
    UNUSED(checked);

    // walk bit array and extract complete 64-bit number
    uint64_t result = 0;
    for (uint32_t i=0; i < 64; i++) {
        if (g_x_bits[63-i]->isChecked()) {
            result = result ^ ((uint64_t)1 << i);
        }
    }

    UpdateXDisplay(result);
}

void BinCalc::InputChanged(const QString &input) {
    bool ok;
    uint64_t tmp;

    if(input.toUtf8() == "-") {
        return;
    }

    // loop through all inputs and check which was modified
    for (uint32_t i=0; i < ARRAYSIZE(g_x_inputs); i++) {
        if (g_x_inputs[i]->isModified() || g_force) {
            g_last_modified = g_x_inputs[i];

            switch(g_inputMap[g_x_inputs[i]->objectName()]) {
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
                tmp = input.toLongLong(&ok, 8);
                if (ok)
                    UpdateXDisplay(tmp);
                break;

            case input_float:
                UpdateXDisplay(input.toDouble());
                break;

            case input_fixed:
                //UpdateXDisplay(0);
                break;

            case input_fract:
                //UpdateXDisplay(0);
                break;

            case input_chars:
                uint64_t mask = 0x00;
                for (int i=0; i < input.length(); i++) {
                    mask = mask | ((uint64_t)0xff << i*8);
                }
                uint64_t value = *(uint64_t *)input.toUtf8().data() & mask;

                UpdateXDisplay(value);
                break;
            }
        }
    }
}

void BinCalc::convertToCharArray(unsigned char *arr, int64_t a)
{
    for (uint32_t i = 0; i < 8; ++i)
    {
        arr[i] = (unsigned char)((((uint64_t) a) >> (56 - (8*i))) & 0xFFu);
    }
}

void BinCalc::UpdateXDisplay(uint64_t value) {

    bool bit_32 = ui->radio_bit_32->isChecked();

    if (bit_32) {
        value = value & 0xffffffff;
    }

    // update input fields
    for (uint32_t i=0; i < ARRAYSIZE(g_x_inputs); i++) {
        // don't update current input being typed
        if (!g_x_inputs[i]->isModified() || g_force) { 
            switch(g_inputMap[g_x_inputs[i]->objectName()]) {
            case input_int:
                if (bit_32) {
                    g_x_inputs[i]->setText(QString::number((int32_t)value));
                    ui->input_stack_x->setText(QString::number((int32_t)value));
                } else {
                    g_x_inputs[i]->setText(QString::number((int64_t)value));
                    ui->input_stack_x->setText(QString::number((int64_t)value));
                }
                break;

            case input_uint:
                g_x_inputs[i]->setText(QString::number(value));
                break;

            case input_hex:
                if (bit_32) {
                    g_x_inputs[i]->setText(QString::number(value, 16).rightJustified(8, '0'));
                } else {
                    g_x_inputs[i]->setText(QString::number(value, 16).rightJustified(16, '0'));
                }
                break;

            case input_octal:
                g_x_inputs[i]->setText(QString::number(value, 8));
                break;

            case input_float:
                g_x_inputs[i]->setText(QString::number(value, 'g', 6));
                break;

            case input_fixed:
                //typedef numeric::fixed<64, 64> fixed;
                //fixed f = (fixed)value;
                //uint64_t integer = f.data_ & f.integer_mask;
                //uint64_t fractional = f.data_ & f.fractional_mask;
                //g_x_inputs[i]->setText(QString::number(integer) + "." + QString::number(fractional));
                //g_x_inputs[i]->setText(QString::number((qlonglong)f.data_));
                g_x_inputs[i]->setText("N/A");
                break;

            case input_fract:
                g_x_inputs[i]->setText("N/A");
                break;

            case input_chars:
                if(g_x_inputs[i]->isModified())
                    break;

                char ascii[9] = {0};
                unsigned char buf[8] = {};
                unsigned int c = 0;
                convertToCharArray(buf, value);
                for (int i = 0; i < 8; i++) {
                        if(isprint(buf[i])) {
                            ascii[c++] =  buf[i];
                        } else {
                            ascii[c++] = '.';
                        }
                }

                ui->input_x_chars->setText(ascii);
                break;
            }
        }
    }

    UpdateBitsDisplay();

    // reset modified flags to detect next change
    for (uint32_t i=0; i < ARRAYSIZE(g_x_inputs); i++) {
        g_x_inputs[i]->setModified(false);
    }
}

void BinCalc::UpdateYDisplay(uint64_t value) {
    if (ui->radio_bit_32->isChecked()) {
        value = value & 0xffffffff;
    }

    if (ui->radio_bit_32->isChecked()) {
        ui->input_y_int->setText(QString::number((int32_t)value));
        ui->input_stack_y->setText(QString::number((int32_t)value));
    } else {
        ui->input_y_int->setText(QString::number((int64_t)value));
        ui->input_stack_y->setText(QString::number((int64_t)value));
    }

    ui->input_y_uint->setText(QString::number(value));
    ui->input_y_hex->setText(QString::number(value, 16));
    ui->input_y_octal->setText(QString::number(value, 8));
    ui->input_y_float->setText(QString::number(value, 'g', 6));
    ui->input_y_fixed->setText("N/A");
    ui->input_y_fract->setText("N/A");

    char ascii[9] = {0};
    unsigned char buf[8] = {};
    unsigned int c = 0;
    convertToCharArray(buf, value);
    for (int i = 0; i < 8; i++) {
            if(isprint(buf[i])) {
                ascii[c++] =  buf[i];
            } else {
                ascii[c++] = '.';
            }
    }

    // std::string string(reinterpret_cast<const char *>(&value), sizeof(value));
    // ui->input_y_chars->setText(QString().fromStdString(string));
    ui->input_y_chars->setText(ascii);

    UpdateBitsDisplay();
}

bool BinCalc::IsBitSet(uint64_t n, uint32_t k) {
    if (!k)
        return (n & 1);

    uint64_t new_num = n >> (k);
    return (new_num & 1);
}

void BinCalc::UpdateBitsDisplay() {
    bool bit_32 = ui->radio_bit_32->isChecked();

    for (uint32_t i=0; i < 64; i++) {
        if(bit_32 && i > 31) {
            g_x_bits[63-i]->setChecked(false);
            g_y_bits[63-i]->setChecked(false);
            g_x_bits[63-i]->setEnabled(false);
            continue;
        }

        // re-enable checkboxes if 64bit
        if(!bit_32 && i > 31) {
            g_x_bits[63-i]->setEnabled(true);
        }

        if (IsBitSet(ui->input_x_int->text().toLongLong(), i)) {
            g_x_bits[63-i]->setChecked(true);
        } else {
            g_x_bits[63-i]->setChecked(false);
        }

        if (IsBitSet(ui->input_y_int->text().toLongLong(), i)) {
            g_y_bits[63-i]->setChecked(true);
        } else {
            g_y_bits[63-i]->setChecked(false);
        }
    }
}
