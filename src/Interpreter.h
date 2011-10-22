#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <QByteArray>
#include <QHash>
#include <QTextStream>

#include "Tape.h"


class Instruction;

class Interpreter
{
public:
    Interpreter(QByteArray program);
    ~Interpreter();

    void setCaptureOutput(bool on);

    void start();

    Tape * tape;

    // current instruction pointer
    int pc;

    // maps beginning bracket pc to ending bracket pc and vice versa
    QHash<int, int> matching_bracket;

    QTextStream * stdout_;
    QTextStream * stderr_;

private:
    QByteArray m_program;
    QString m_stdout_str;
    QString m_stderr_str;


    Instruction * m_noop;

    // maps instruction character to instruction object instance
    QHash<char, Instruction *> m_instructions;
};

#endif // INTERPRETER_H
