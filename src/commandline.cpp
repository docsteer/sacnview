#include "commandline.h"
#include "consts.h"
#include "preferences.h"
#include <QApplication>

CommandLine::CommandLine(QObject * parent)
    : QObject(parent), m_terminated(false), m_clearKeyTimer(new QTimer(this))
{
    m_clearKeyTimer->setSingleShot(true);
}

void CommandLine::processKey(Key value)
{
    if (value == ALL_OFF)
    {
        m_keyStack.clear();
        m_errorText.clear();
        m_text.clear();
        m_addresses.clear();
        m_keyStack.push(K1);
        m_keyStack.push(THRU);
        m_keyStack.push(K5);
        m_keyStack.push(K1);
        m_keyStack.push(K2);
        m_keyStack.push(AT);
        m_keyStack.push(K0);
        m_keyStack.push(ENTER);
        processStack();
        return;
    }
    if (value == CLEAR && !m_keyStack.isEmpty())
    {
        if (m_terminated)
        {
            // Clearing a terminated command line
            m_keyStack.clear();
            m_errorText.clear();
            m_text.clear();
            m_addresses.clear();
            m_terminated = false;
        }
        else if (m_clearKeyTimer->isActive())
        {
            // Double tap, clear line
            m_keyStack.clear();
            m_errorText.clear();
        }
        else
        {
            // Backspace
            m_keyStack.pop();
            m_errorText.clear();
        }

        m_clearKeyTimer->start(QApplication::doubleClickInterval());
    }
    else
    {
        if (m_terminated)
        {
            m_keyStack.clear();
            m_errorText.clear();
            m_text.clear();
            m_addresses.clear();
            m_terminated = false;
        }
        if (m_errorText.isEmpty()) m_keyStack.push(value);
    }

    processStack();
}

void CommandLine::getSelection(QSet<int> * selection, int * numberEntry, int * startRange, int offset, stackFlags flags)
{
    QSet<int> tempSelection;
    if (*startRange != 0 && *numberEntry != 0)
    {
        // Thru
        if (*numberEntry > *startRange)
            for (int i = *startRange; i <= *numberEntry; i += offset) tempSelection.insert(i);
        else
            for (int i = *numberEntry; i <= *startRange; i += offset) tempSelection.insert(i);
    }
    else
    {
        // Single
        if (*numberEntry != 0) tempSelection.insert(*numberEntry);
    }

    // Update selection
    for (auto i : tempSelection)
    {
        if (flags.addMode)
            selection->insert(i);
        else
            selection->remove(i);
    }

    *numberEntry = 0;
    *startRange = 0;
}

void CommandLine::processStack()
{
    m_text.clear();
    stackFlags flags;
    int numberEntry = 0;
    int startRange = 0;
    int endRange = 0;
    QSet<int> selection;
    const int maxLevel = Preferences::Instance().GetMaxLevel();

    for (int pos = 0; pos < m_keyStack.count(); pos++)
    {
        if (flags.state == stError)
        {
            return;
        }

        Key key = m_keyStack.at(pos);
        int numeric = (int)key;
        switch (key)
        {
            case K0:
            case K1:
            case K2:
            case K3:
            case K4:
            case K5:
            case K6:
            case K7:
            case K8:
            case K9:
                numberEntry *= 10;
                numberEntry += numeric;

                if (flags.state == stChannel)
                {
                    if (numberEntry > MAX_DMX_ADDRESS)
                    {
                        // Not a valid entry, would be >512
                        flags.state = stError;
                        m_errorText = E_RANGE();
                        m_keyStack.pop_back();
                        return;
                    }
                }

                if (flags.state == stLevels)
                {
                    if (numberEntry > maxLevel)
                    {
                        // Not a valid entry, would be >max
                        flags.state = stError;
                        m_errorText = E_RANGE();
                        m_keyStack.pop_back();
                        return;
                    }
                }

                m_text.append(QString::number(numeric));
                break;

            case THRU:
                m_text.append(QString(" %1 ").arg(K_THRU()));
                // [THRU] [THRU] = [THRU] [5][1][2]
                if (m_keyStack.count() > 1
                    && m_keyStack.last() == Key::THRU
                    && m_keyStack.last() == m_keyStack[m_keyStack.count() - 2])
                {
                    m_keyStack.pop();
                    processKey(K5);
                    processKey(K1);
                    processKey(K2);
                    return;
                }
                if (numberEntry == 0 || startRange != 0 || flags.state != stChannel)
                {
                    flags.state = stError;
                    m_errorText = E_SYNTAX();
                    return;
                }
                startRange = numberEntry;
                numberEntry = 0;
                flags.thruMode = true;
                break;

            case OFFSET:
                m_text.append(QString(" %1 ").arg(K_OFFSET()));
                if (numberEntry == 0 || flags.state != stChannel || !flags.thruMode)
                {
                    flags.state = stError;
                    m_errorText = E_SYNTAX();
                    return;
                }
                endRange = numberEntry;
                numberEntry = 0;
                break;

            case AT:
            {
                // [@] [@] = [@] [Full]
                if (m_keyStack.count() > 1
                    && m_keyStack.last() == Key::AT
                    && m_keyStack.last() == m_keyStack[m_keyStack.count() - 2])
                {
                    m_keyStack.pop();
                    processKey(FULL);
                    return;
                }

                m_text.append(QString(" %1 ").arg(K_AT()));
                ;
                if (startRange != 0 && numberEntry == 0)
                {
                    flags.state = stError;
                    m_errorText = E_SYNTAX();
                    return;
                }

                if (flags.thruMode && endRange != 0)
                {
                    getSelection(&selection, &endRange, &startRange, numberEntry, flags);
                    flags.thruMode = false;
                    numberEntry = 0;
                }
                else
                    getSelection(&selection, &numberEntry, &startRange, 1, flags);

                if (selection.isEmpty() && !m_previousKeyStack.isEmpty())
                {
                    m_keyStack = m_previousKeyStack;
                    m_keyStack.push(AT);
                    processStack();
                    return;
                }

                flags.state = stLevels;

                // Copy the entries up to the at to the last addresses
                int i = 0;
                m_previousKeyStack.clear();
                while (m_keyStack[i] != AT)
                {
                    m_previousKeyStack << m_keyStack[i];
                    i++;
                }
                break;
            }

            case AND:
                m_text.append(QString(" %1 ").arg(K_AND()));
                if (numberEntry == 0 || flags.state != stChannel)
                {
                    if (m_keyStack.count() == 1 && !m_previousKeyStack.isEmpty())
                    {
                        // Empty command line and selection...use previous
                        m_keyStack = m_previousKeyStack;
                        processKey(AND);
                        return;
                    }
                    else
                    {
                        flags.state = stError;
                        m_errorText = E_SYNTAX();
                        return;
                    }
                }

                if (flags.thruMode && endRange != 0)
                {
                    getSelection(&selection, &endRange, &startRange, numberEntry, flags);
                    flags.thruMode = false;
                    numberEntry = 0;
                }
                else
                    getSelection(&selection, &numberEntry, &startRange, 1, flags);

                flags.addMode = true;
                break;

            case MINUS:
                m_text.append(QString(" %1 ").arg(K_MINUS()));
                if (numberEntry == 0 || flags.state != stChannel)
                {
                    if (m_keyStack.count() == 1 && !m_previousKeyStack.isEmpty())
                    {
                        // Empty command line and selection...use previous
                        m_keyStack = m_previousKeyStack;
                        processKey(MINUS);
                        return;
                    }
                    else
                    {
                        flags.state = stError;
                        m_errorText = E_SYNTAX();
                        return;
                    }
                }
                if (flags.thruMode && endRange != 0)
                {
                    getSelection(&selection, &endRange, &startRange, numberEntry, flags);
                    flags.thruMode = false;
                    numberEntry = 0;
                }
                else
                    getSelection(&selection, &numberEntry, &startRange, 1, flags);

                flags.addMode = false;
                break;

            case ENTER:

                m_terminated = true;
                m_text.append("*");

                if (flags.state != stLevels) return;

                // m_level is always in absolute (0-255)
                if (Preferences::Instance().GetDisplayFormat() == DisplayFormat::PERCENT)
                    m_level = PTOHT[numberEntry];
                else
                    m_level = numberEntry;

                m_addresses = selection;
                return;

            case FULL:
                // Anything selected?
                if (m_keyStack.size() && !m_keyStack.contains(AT))
                {
                    m_keyStack.pop();
                    processKey(AT);
                    processKey(FULL);
                    return;
                }
                else if (selection.isEmpty())
                {
                    // ...Nope nothing
                    // Lets try the previous selection
                    if (!m_previousKeyStack.isEmpty())
                    {
                        m_keyStack = m_previousKeyStack;
                        processKey(AT);
                        processKey(FULL);
                    }
                    else
                    {
                        flags.state = stError;
                        m_errorText = E_NO_SELECTION();
                    }
                    return;
                }

                m_text.append(QString("%1*").arg(K_FULL()));
                if (startRange != 0 && numberEntry == 0)
                {
                    flags.state = stError;
                    m_errorText = E_SYNTAX();
                    return;
                }
                if (flags.state == stLevels && numberEntry != 0)
                {
                    flags.state = stError;
                    m_errorText = E_SYNTAX();
                    return;
                }
                m_level = MAX_SACN_LEVEL;
                m_addresses = selection;
                m_terminated = true;
                return;

            case CLEAR: Q_FALLTHROUGH();
            default: break;
        }
    }
}

QString CommandLine::text()
{
    return m_text;
}

/************************ CommandLineWidget ******************************/

CommandLineWidget::CommandLineWidget(QWidget * parent)
    : QTextEdit(parent), m_cursorTimer(new QTimer(this)), m_cursorState(true)
{
    this->setReadOnly(true);
    setStyleSheet("color: rgb(127, 255, 23);background: black;font: 75 12pt \"Courier\";");
    clear();

    // Cursor blinker
    connect(m_cursorTimer, &QTimer::timeout, this, &CommandLineWidget::flashCursor);
    m_cursorTimer->setInterval(300);
    m_cursorTimer->setSingleShot(false);
    m_cursorTimer->start();
}

void CommandLineWidget::flashCursor()
{
    m_cursorState = !m_cursorState;
    updateText();
}

void CommandLineWidget::processKey(CommandLine::Key value)
{
    m_commandLine.processKey(value);
    updateText();
    if (!m_commandLine.addresses().isEmpty())
    {
        emit setLevels(m_commandLine.addresses(), m_commandLine.level());
    }
}

void CommandLineWidget::updateText()
{
    QString text = m_commandLine.text();

    if (this->hasFocus())
    {
        auto cursor = (m_cursorState == true) ? "_" : "";
        text.append(cursor);
    }

    this->setText(text);
    this->append(QString("<span style=\"color:red;\">%1</span>").arg(m_commandLine.errorText()));
}

void CommandLineWidget::keyPressEvent(QKeyEvent * e)
{
    switch (e->key())
    {
        case Qt::Key_0: key0(); break;
        case Qt::Key_1: key1(); break;
        case Qt::Key_2: key2(); break;
        case Qt::Key_3: key3(); break;
        case Qt::Key_4: key4(); break;
        case Qt::Key_5: key5(); break;
        case Qt::Key_6: key6(); break;
        case Qt::Key_7: key7(); break;
        case Qt::Key_8: key8(); break;
        case Qt::Key_9: key9(); break;
        case Qt::Key_T: keyThru(); break;
        case Qt::Key_Delete:
        case Qt::Key_Backspace: keyClear(); break;
        case Qt::Key_Enter:
        case Qt::Key_Return: keyEnter(); break;
        case Qt::Key_Plus: keyAnd(); break;
        case Qt::Key_Minus: keyMinus(); break;
        case Qt::Key_A:
        case Qt::Key_At: keyAt(); break;
        case Qt::Key_O: keyAllOff(); break;
        case Qt::Key_F: keyFull(); break;
    }
}

EditableLCDNumber::EditableLCDNumber(QWidget * parent)
    : QLCDNumber(parent)
{}

void EditableLCDNumber::keyPressEvent(QKeyEvent * event)
{
    int buf = 0;

    switch (event->key())
    {
        case Qt::Key_Backspace:
            if (intValue() / 10 > 0)
            {
                buf = intValue() / 10;
                display(buf);
                emit valueChanged(buf);
            }
            else
                display(QString(" "));
            break;
        case Qt::Key_PageDown:
            if (intValue() - 1 > 0)
            {
                buf = intValue() - 1;
                display(buf);
                emit valueChanged(buf);
            }
            else
            {
                display(MAX_DMX_ADDRESS);
                emit valueChanged(MAX_DMX_ADDRESS);
            }
            break;
        case Qt::Key_PageUp:
            if (intValue() < MAX_DMX_ADDRESS)
            {
                buf = intValue() + 1;
                display(buf);
                emit valueChanged(buf);
            }
            else
            {
                display(1);
                emit valueChanged(1);
            }
            break;
        case Qt::Key_0:
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
        case Qt::Key_5:
        case Qt::Key_6:
        case Qt::Key_7:
        case Qt::Key_8:
        case Qt::Key_9:
            buf = value() * 10 + (event->key() - Qt::Key_0);
            if (buf <= MAX_DMX_ADDRESS)
            {
                display(buf);
                emit valueChanged(buf);
            }
            break;
        case Qt::Key_Space: emit toggleOff(); break;
        default: break;
    }
}
