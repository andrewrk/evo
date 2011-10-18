#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <QByteArray>
#include <QHash>

#include "Tape.h"


class Instruction;

class Interpreter
{
public:
    Interpreter(QByteArray program);
    ~Interpreter();

    void start();

    Tape * tape;

private:
    QByteArray m_program;
    int m_pc; // current instruction pointer

    Instruction * m_noop;

    // maps instruction character to instruction object instance
    QHash<char, Instruction *> m_instructions;
};

#endif // INTERPRETER_H
