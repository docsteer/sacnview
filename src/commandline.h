#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <QObject>
#include <QWidget>
#include <QPlainTextEdit>
#include <QStack>
#include <QLCDNumber>
#include <QTimer>
#include <QString>

class CommandLine : public QObject
{
    Q_OBJECT
public:
    // Strings
    const QString K_THRU() { return QObject::tr("THRU"); }
    const QString K_OFFSET() { return QObject::tr("OFFSET"); }
    const QString K_AT() { return QObject::tr("AT"); }
    const QString K_FULL() { return QObject::tr("FULL"); }
    const QString K_CLEAR() { return QObject::tr("CLEAR"); }
    const QString K_AND() { return QObject::tr("+"); }
    const QString K_MINUS() { return QObject::tr("-"); }

    const QString E_SYNTAX() { return QObject::tr("Error - syntax error"); }
    const QString E_RANGE() { return QObject::tr("Error - number out of range"); }
    const QString E_NO_SELECTION() {return QObject::tr("Error - no selection"); }

    explicit CommandLine(QObject *parent = nullptr);

    enum Key {
        K0,
        K1,
        K2,
        K3,
        K4,
        K5,
        K6,
        K7,
        K8,
        K9,
        THRU,
        OFFSET,
        AT,
        FULL,
        CLEAR,
        AND,
        MINUS,
        ENTER,
        ALL_OFF
    };


    QString text();
    QString errorText() { return m_errorText; }
    void processKey(Key value);
    QSet<int> addresses() { return m_addresses; }
    int level() { return m_level; }
private:
    enum stackState
    {
            stChannel,
            stLevels,
            stReady,
            stError,
    };
    struct stackFlags
    {
        stackState state = stChannel;
        bool addMode = true;
        bool thruMode = false;
    };

    QString m_text;
    QString m_errorText;
    void processStack();
    void getSelection(QSet<int> *selection, int *numberEntry, int *startRange, int offset, stackFlags flags);
    QSet<int> m_addresses;
    int m_level;
    bool m_terminated;
    QStack<Key> m_previousKeyStack;
    QStack<Key> m_keyStack;
    QTimer *m_clearKeyTimer;
};

class CommandLineWidget : public QTextEdit
{
    Q_OBJECT
public:
    CommandLineWidget(QWidget *parent = 0);
public slots:
    void key1() { processKey(CommandLine::K1); }
    void key2() { processKey(CommandLine::K2); }
    void key3() { processKey(CommandLine::K3); }
    void key4() { processKey(CommandLine::K4); }
    void key5() { processKey(CommandLine::K5); }
    void key6() { processKey(CommandLine::K6); }
    void key7() { processKey(CommandLine::K7); }
    void key8() { processKey(CommandLine::K8); }
    void key9() { processKey(CommandLine::K9); }
    void key0() { processKey(CommandLine::K0); }
    void keyThru() { processKey(CommandLine::THRU); }
    void keyOffset() { processKey(CommandLine::OFFSET); }
    void keyAt() { processKey(CommandLine::AT); }
    void keyFull() { processKey(CommandLine::FULL); }
    void keyClear() { processKey(CommandLine::CLEAR); }
    void keyAnd() { processKey(CommandLine::AND); }
    void keyMinus() { processKey(CommandLine::MINUS); }
    void keyEnter() { processKey(CommandLine::ENTER); }
    void keyAllOff() { processKey(CommandLine::ALL_OFF); }
signals:
    void setLevels(QSet<int> addreses, int level);
protected:
    virtual void keyPressEvent(QKeyEvent *e);
private slots:
    void flashCursor();
private:
    CommandLine m_commandLine;
    void updateText();
    void processKey(CommandLine::Key);
    QTimer *m_cursorTimer;
    bool m_cursorState;
};

class EditableLCDNumber : public QLCDNumber
{
    Q_OBJECT
public:
    EditableLCDNumber(QWidget *parent);
signals:
    void valueChanged(int);
    void toggleOff();
protected:
    virtual void keyPressEvent(QKeyEvent *event);
};

#endif // COMMANDLINE_H
