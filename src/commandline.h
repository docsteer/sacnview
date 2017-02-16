#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <QObject>
#include <QWidget>
#include <QPlainTextEdit>
#include <QStack>

class CommandLine
{
public:
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
        AT,
        FULL,
        CLEAR,
        AND,
        ENTER,
    };

    CommandLine();

    QString text();
    QString errorText() { return m_errorText;};
    void processKey(Key value);
    QSet<int> addresses() { return m_addresses;};
    int level() { return m_level;};
private:
    QString m_text;
    QString m_errorText;
    void processStack();
    QSet<int> m_addresses;
    int m_level;
    bool m_terminated;
    QStack<Key> m_previousKeyStack;
    QStack<Key> m_keyStack;
};

class CommandLineWidget : public QTextEdit
{
    Q_OBJECT
public:
    CommandLineWidget(QWidget *parent = 0);
public slots:
    void key1();
    void key2();
    void key3();
    void key4();
    void key5();
    void key6();
    void key7();
    void key8();
    void key9();
    void key0();
    void keyThru();
    void keyAt();
    void keyFull();
    void keyClear();
    void keyAnd();
    void keyEnter();
signals:
    void setLevels(QSet<int> addreses, int level);
protected:
    virtual void keyPressEvent(QKeyEvent *e);
private:
    CommandLine m_commandLine;
    void displayText();
};

#endif // COMMANDLINE_H
