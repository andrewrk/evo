#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "Instruction.h"
#include "Interpreter.h"

#include <iostream>

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

    }
};
class DecrementHeadInstruction : public Instruction
{
public:
    DecrementHeadInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {

    }
};
class IncrementAtHeadInstruction : public Instruction
{
public:
    IncrementAtHeadInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {

    }
};
class DecrementAtHeadInstruction : public Instruction
{
public:
    DecrementAtHeadInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {

    }
};
class OutputHeadInstruction : public Instruction
{
public:
    OutputHeadInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {

    }
};
class InputHeadInstruction : public Instruction
{
public:
    InputHeadInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {

    }
};
class BeginLoopInstruction : public Instruction
{
public:
    BeginLoopInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {

    }
};
class EndLoopInstruction : public Instruction
{
public:
    EndLoopInstruction(Interpreter * interpreter) : Instruction(interpreter) {}
    void execute() {

    }
};


#endif // INSTRUCTIONS_H
