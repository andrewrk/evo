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
    void setInput(QByteArray input);
    void setMaxCycles(qint64 cycles);

    void start();

    qint64 cycleCount() const;

    Tape * tape;

    // current instruction pointer
    int pc;

    // maps beginning bracket pc to ending bracket pc and vice versa
    QHash<int, int> matching_bracket;

    QTextStream * stdout_;
    QTextStream * stderr_;
    QTextStream * stdin_;

    // set to true if cycles reached max cycles
    bool timed_out_flag;

private:
    QByteArray m_program;
    QString m_stdout_str;
    QString m_stderr_str;

    QByteArray m_input;

    Instruction * m_noop;

    // maps instruction character to instruction object instance
    QHash<char, Instruction *> m_instructions;

    // how many instructions have we processed
    qint64 m_cycle_count;
    qint64 m_max_cycles;
};

#endif // INTERPRETER_H
