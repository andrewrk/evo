#ifndef INSTRUCTION_H
#define INSTRUCTION_H

class Interpreter;

class Instruction
{
public:
    Instruction(Interpreter * interpreter);

    virtual void execute() = 0;

protected:

    Interpreter * m_interpreter;
};

#endif // INSTRUCTION_H
