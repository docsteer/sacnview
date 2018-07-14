#include <QApplication>
#include "commandline.h"
#include "consts.h"
#include "preferences.h"

CommandLine::CommandLine(QObject *parent) : QObject(parent),
    m_terminated(false),
    m_clearKeyTimer(new QTimer(this))
{
    m_clearKeyTimer->setSingleShot(true);
}

void CommandLine::processKey(Key value)
{
    if(value==ALL_OFF)
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
    if(value==CLEAR && !m_keyStack.isEmpty())
    {
        if(m_terminated)
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
        if(m_terminated)
        {
            m_keyStack.clear();
            m_errorText.clear();
            m_text.clear();
            m_addresses.clear();
            m_terminated = false;
        }
        m_keyStack.push(value);
    }
    processStack();
}

void getSelection(QSet<int> *selection, int *numberEntry, int *startRange)
{
    if(*startRange != 0 && *numberEntry != 0)
    {
        // Thru
        if (*numberEntry > *startRange)
            for(int i = *startRange; i <= *numberEntry; i++)
                selection->insert(i);
        else
            for(int i = *numberEntry; i <= *startRange; i++)
                selection->insert(i);
    }
    else
    {
        // Single
        if(*numberEntry!=0)
            selection->insert(*numberEntry);
    }

   *numberEntry = 0;
   *startRange = 0;
}

void CommandLine::processStack()
{
    enum {
        stChannel,
        stThruStart,
        stThruEnd,
        stLevels,
        stReady,
        stError,
    };

    m_text.clear();
    int state = stChannel;
    int numberEntry = 0;
    int startRange = 0;
    int i=0;
    QSet<int> selection;
    const int maxLevel = Preferences::getInstance()->GetMaxLevel();

    for(int pos=0; pos<m_keyStack.count(); pos++)
    {
        Key key = m_keyStack.at(pos);
        int numeric = (int) key;
        switch(key)
        {
        case K0:
            Q_FALLTHROUGH();
        case K1:
            Q_FALLTHROUGH();
        case K2:
            Q_FALLTHROUGH();
        case K3:
            Q_FALLTHROUGH();
        case K4:
            Q_FALLTHROUGH();
        case K5:
            Q_FALLTHROUGH();
        case K6:
            Q_FALLTHROUGH();
        case K7:
            Q_FALLTHROUGH();
        case K8:
            Q_FALLTHROUGH();
        case K9:
            numberEntry *= 10;
            numberEntry += numeric;

            if(state==stChannel || state==stThruStart || state==stThruEnd)
            {
                if(numberEntry>MAX_DMX_ADDRESS)
                {
                    // Not a valid entry, would be >512
                    state = stError;
                    m_errorText = E_RANGE();
                    m_keyStack.pop_back();
                    return;
                }

            }

            if(state==stLevels)
            {
                if(numberEntry>maxLevel)
                {
                    // Not a valid entry, would be >max
                    state = stError;
                    m_errorText = E_RANGE();
                    m_keyStack.pop_back();
                    return;
                }
            }

            m_text.append(QString::number(numeric));
            break;

        case THRU:
                m_text.append(QString(" %1 ").arg(K_THRU()));
                if(numberEntry==0 || state!=stChannel || startRange!=0)
                {
                    m_errorText = E_SYNTAX();
                    return;
                }
                startRange=numberEntry;
                numberEntry = 0;
            break;

        case AT:
            // [@] [@] = [@] [Full]
            if (
                m_keyStack.count() > 1 &&
                m_keyStack.last() == Key::AT &&
                m_keyStack.last() == m_keyStack[m_keyStack.count() - 2]
                )
            {
                m_keyStack.pop();
                processKey(FULL);
                return;
            }

            m_text.append(QString(" %1 ").arg(K_AT()));;
            if(startRange!=0 && numberEntry==0)
            {
                m_errorText = E_SYNTAX();
                return;
            }

            getSelection(&selection, &numberEntry, &startRange);

            if(selection.isEmpty() && !m_previousKeyStack.isEmpty())
            {
                m_keyStack = m_previousKeyStack;
                m_keyStack.push(AT);
                processStack();
                return;
            }

            state = stLevels;

            // Copy the entries up to the at to the last addresses
            i=0;
            m_previousKeyStack.clear();
            while(m_keyStack[i]!=AT)
            {
                m_previousKeyStack << m_keyStack[i];
                i++;
            }
            break;

        case AND:
            m_text.append(QString(" %1 ").arg(K_AND()));
            if(numberEntry==0 || state != stChannel)
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
                    m_errorText = E_SYNTAX();
                    return;
                }
            }

            getSelection(&selection, &numberEntry, &startRange);
            numberEntry = 0;
            startRange = 0;

            break;

        case ENTER:

            m_terminated = true;
            m_text.append("*");

            if(state!=stLevels)
                return;

            // m_level is always in absolute (0-255)
            if(Preferences::getInstance()->GetDisplayFormat()==Preferences::PERCENT)
                m_level = PTOHT[numberEntry];
            else
                m_level = numberEntry;

            m_addresses = selection;
            return;

        case FULL:
            // Anything selected?
            if(
                selection.isEmpty()
                && m_keyStack.size()
                && !m_keyStack.contains(AT)
                )
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
                    m_errorText = E_NO_SELECTION();
                }
                return;
            }

            m_text.append(QString("%1*").arg(K_FULL()));
            if(startRange!=0 && numberEntry==0)
            {
                m_errorText = E_SYNTAX();
                return;
            }
            if(state==stLevels && numberEntry!=0)
            {
                m_errorText = E_SYNTAX();
                return;
            }
            m_level = MAX_SACN_LEVEL;
            m_addresses = selection;
            m_terminated = true;
            return;

        case CLEAR:
            Q_FALLTHROUGH();
        default:
            break;
        }
    }
}

QString CommandLine::text()
{
    return m_text;
}

/************************ CommandLineWidget ******************************/

CommandLineWidget::CommandLineWidget(QWidget *parent) : QTextEdit(parent),
    m_cursorTimer(new QTimer(this)),
    m_cursorState(true)
{
    this->setReadOnly(true);
    setStyleSheet("color: rgb(127, 255, 23);background: black;font: 75 12pt \"Courier\";");
    clear();

    // Cursor blinker
    connect(m_cursorTimer, SIGNAL(timeout()), this, SLOT(flashCursor()));
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
    if(!m_commandLine.addresses().isEmpty())
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

void CommandLineWidget::keyPressEvent(QKeyEvent *e)
{
    switch(e->key())
    {
    case Qt::Key_0:
        key0();
        break;
    case Qt::Key_1:
        key1();
        break;
    case Qt::Key_2:
        key2();
        break;
    case Qt::Key_3:
        key3();
        break;
    case Qt::Key_4:
        key4();
        break;
    case Qt::Key_5:
        key5();
        break;
    case Qt::Key_6:
        key6();
        break;
    case Qt::Key_7:
        key7();
        break;
    case Qt::Key_8:
        key8();
        break;
    case Qt::Key_9:
        key9();
        break;
    case Qt::Key_T:
        keyThru();
        break;
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
        keyClear();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        keyEnter();
        break;
    case Qt::Key_Plus:
        keyAnd();
        break;
    case Qt::Key_A:
    case Qt::Key_At:
        keyAt();
        break;
    case Qt::Key_O:
        keyAllOff();
        break;
    case Qt::Key_F:
        keyFull();
        break;

    }
}


EditableLCDNumber::EditableLCDNumber(QWidget *parent) : QLCDNumber(parent)
{

}

void EditableLCDNumber::keyPressEvent(QKeyEvent *event)
{
    int buf = 0;

    switch(event->key())
    {
    case Qt::Key_Backspace:
        if(intValue()/10 > 0)
        {
            buf = intValue() / 10;
            display(buf);
            emit valueChanged(buf);
        }
        else
            display(QString(" "));
        break;
    case Qt::Key_PageDown:
        if(intValue()-1>0)
        {
            buf = intValue()-1;
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
        if(intValue()<MAX_DMX_ADDRESS)
        {
            buf = intValue()+1;
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
        buf = value()*10 + (event->key()-Qt::Key_0);
        if(buf<=MAX_DMX_ADDRESS)
        {
            display(buf);
            emit valueChanged(buf);
        }
        break;
    case Qt::Key_Space:
        emit toggleOff();
        break;
    default:
        break;
    }
}
