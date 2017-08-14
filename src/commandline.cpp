#include "commandline.h"
#include "consts.h"
#include "preferences.h"

CommandLine::CommandLine()
{
    m_terminated = false;
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
        else
        {
            // Backspace
            m_keyStack.pop();
            m_errorText.clear();
        }
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

            m_text.append(QString::number(numeric));

            if(state==stChannel || state==stThruStart || state==stThruEnd)
            {
                if(numberEntry>MAX_DMX_ADDRESS)
                {
                    // Not a valid entry, would be >512
                    state = stError;
                    m_errorText = "Error - number out of range";
                    return;
                }

            }

            if(state==stLevels)
            {
                if(numberEntry>maxLevel)
                {
                    // Not a valid entry, would be >max
                    state = stError;
                    m_errorText = "Error - number out of range";
                    return;
                }
            }
            break;

        case THRU:
                m_text.append(" THRU ");
                if(numberEntry==0 || state!=stChannel || startRange!=0)
                {
                    m_errorText = "Error - syntax error";
                    return;
                }
                startRange=numberEntry;
                numberEntry = 0;
            break;

        case AT:
            m_text.append(" @ ");
            if(startRange!=0 && numberEntry==0)
            {
                m_errorText = "Error - syntax error";
                return;
            }
            if(startRange!=0 && numberEntry!=0)
            {
                for(int i=startRange; i<numberEntry; i++)
                    selection << i;
            }
            if(numberEntry!=0) selection << numberEntry;

            if(selection.isEmpty() && !m_previousKeyStack.isEmpty())
            {
                m_keyStack = m_previousKeyStack;
                m_keyStack.push(AT);
                processStack();
                return;
            }

            state = stLevels;
            numberEntry = 0;
            startRange = 0;
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
            m_text.append(" AND ");
            if(numberEntry==0 || state != stChannel)
            {
                m_errorText = "Error - syntax error";
                return;
            }
            selection << numberEntry;
            numberEntry = 0;
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
            if(selection.isEmpty() && !m_previousKeyStack.isEmpty())
            {
                m_keyStack = m_previousKeyStack;
                m_keyStack.push(AT);
                m_keyStack.push(FULL);
                processStack();
                return;
            }

            if(state!=stLevels)
                m_text.append(" @ ");
            m_text.append("FULL*");
            if(startRange!=0 && numberEntry==0)
            {
                m_errorText = "Error : Syntax Error";
                return;
            }
            if(state==stLevels && numberEntry!=0)
            {
                m_errorText = "Error : Syntax Error";
                return;
            }
            m_level = MAX_SACN_LEVEL;
            m_addresses = selection;
            m_terminated = true;
            return;

        case CLEAR:
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

CommandLineWidget::CommandLineWidget(QWidget *parent) : QTextEdit(parent)
{
    this->setReadOnly(true);
    setStyleSheet("color: rgb(127, 255, 23);background: black;font: 75 12pt \"Courier\";");
    clear();
}

void CommandLineWidget::displayText()
{
    this->setText(m_commandLine.text());
    this->append(QString("<span style=\"color:red;\">%1</span>").arg(m_commandLine.errorText()));
    if(!m_commandLine.addresses().isEmpty())
    {
        emit setLevels(m_commandLine.addresses(), m_commandLine.level());
    }
}

void CommandLineWidget::key1()
{
    m_commandLine.processKey(CommandLine::K1);
    displayText();
}

void CommandLineWidget::key2()
{
    m_commandLine.processKey(CommandLine::K2);
    displayText();
}

void CommandLineWidget::key3()
{
    m_commandLine.processKey(CommandLine::K3);
    displayText();
}

void CommandLineWidget::key4()
{
    m_commandLine.processKey(CommandLine::K4);
    displayText();
}

void CommandLineWidget::key5()
{
    m_commandLine.processKey(CommandLine::K5);
    displayText();
}

void CommandLineWidget::key6()
{
    m_commandLine.processKey(CommandLine::K6);
    displayText();
}

void CommandLineWidget::key7()
{
    m_commandLine.processKey(CommandLine::K7);
    displayText();
}

void CommandLineWidget::key8()
{
    m_commandLine.processKey(CommandLine::K8);
    displayText();
}

void CommandLineWidget::key9()
{
    m_commandLine.processKey(CommandLine::K9);
    displayText();
}

void CommandLineWidget::key0()
{
    m_commandLine.processKey(CommandLine::K0);
    displayText();
}

void CommandLineWidget::keyThru()
{
    m_commandLine.processKey(CommandLine::THRU);
    displayText();
}

void CommandLineWidget::keyAt()
{
    m_commandLine.processKey(CommandLine::AT);
    displayText();
}

void CommandLineWidget::keyFull()
{
    m_commandLine.processKey(CommandLine::FULL);
    displayText();
}

void CommandLineWidget::keyClear()
{
    m_commandLine.processKey(CommandLine::CLEAR);
    displayText();
}

void CommandLineWidget::keyAnd()
{
    m_commandLine.processKey(CommandLine::AND);
    displayText();
}

void CommandLineWidget::keyEnter()
{
    m_commandLine.processKey(CommandLine::ENTER);
    displayText();
}

void CommandLineWidget::keyAllOff()
{
    m_commandLine.processKey(CommandLine::ALL_OFF);
    displayText();
}

void CommandLineWidget::keyPressEvent(QKeyEvent *e)
{
    switch(e->key())
    {
    case Qt::Key_0:
        m_commandLine.processKey(CommandLine::K0);
        break;
    case Qt::Key_1:
        m_commandLine.processKey(CommandLine::K1);
        break;
    case Qt::Key_2:
        m_commandLine.processKey(CommandLine::K2);
        break;
    case Qt::Key_3:
        m_commandLine.processKey(CommandLine::K3);
        break;
    case Qt::Key_4:
        m_commandLine.processKey(CommandLine::K4);
        break;
    case Qt::Key_5:
        m_commandLine.processKey(CommandLine::K5);
        break;
    case Qt::Key_6:
        m_commandLine.processKey(CommandLine::K6);
        break;
    case Qt::Key_7:
        m_commandLine.processKey(CommandLine::K7);
        break;
    case Qt::Key_8:
        m_commandLine.processKey(CommandLine::K8);
        break;
    case Qt::Key_9:
        m_commandLine.processKey(CommandLine::K9);
        break;
    case Qt::Key_T:
        m_commandLine.processKey(CommandLine::THRU);
        break;
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
        m_commandLine.processKey(CommandLine::CLEAR);
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        m_commandLine.processKey(CommandLine::ENTER);
        break;
    case Qt::Key_Plus:
        m_commandLine.processKey(CommandLine::AND);
        break;
    case Qt::Key_A:
    case Qt::Key_At:
        m_commandLine.processKey(CommandLine::AT);
        break;
    case Qt::Key_O:
        m_commandLine.processKey(CommandLine::ALL_OFF);
        break;
    case Qt::Key_F:
        m_commandLine.processKey(CommandLine::FULL);
        break;

    }

    displayText();
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
