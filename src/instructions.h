#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "Instruction.h"
#include "Interpreter.h"

class NoopInstruction : public Instruction
{
public:
    NoopInstruction(Interpreter * interpreter) : Instruction(interpreter) {}

    void execute() {
        // do nothing
    }
};

class IncrementHeadInstruction : public Instruction
{
public:
    IncrementHeadInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {
        m_interpreter->tape->incrementHead();
    }
};
class DecrementHeadInstruction : public Instruction
{
public:
    DecrementHeadInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {
        m_interpreter->tape->decrementHead();
    }
};
class IncrementAtHeadInstruction : public Instruction
{
public:
    IncrementAtHeadInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {
        m_interpreter->tape->incrementAtHead();
    }
};
class DecrementAtHeadInstruction : public Instruction
{
public:
    DecrementAtHeadInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {
        m_interpreter->tape->decrementAtHead();
    }
};
class OutputHeadInstruction : public Instruction
{
public:
    OutputHeadInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {
        *(m_interpreter->stdout_) << m_interpreter->tape->readFromHead();
        m_interpreter->stdout_->flush();
    }
};
class InputHeadInstruction : public Instruction
{
public:
    InputHeadInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {
        if (m_interpreter->stdin_ == NULL)
            return;
        char byte;
        *(m_interpreter->stdin_) >> byte;
        m_interpreter->tape->writeToHead(byte);
    }
};
class BeginLoopInstruction : public Instruction
{
public:
    BeginLoopInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {
        if (m_interpreter->tape->readFromHead() == 0) {
            m_interpreter->pc = m_interpreter->matching_bracket.value(m_interpreter->pc, m_interpreter->pc);
        }
    }
};
class EndLoopInstruction : public Instruction
{
public:
    EndLoopInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {
        // - 1 to compensate for the PC incrementing
        m_interpreter->pc = m_interpreter->matching_bracket.value(m_interpreter->pc) - 1;
    }
};


#endif // INSTRUCTIONS_H
