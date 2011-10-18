#ifndef INSTRUCTION_H
#define INSTRUCTION_H

class Interpreter;

class Instruction
{
public:
    Instruction(Interpreter * interpreter);

    virtual void execute() = 0;

private:

    Interpreter * m_interpreter;
};

#endif // INSTRUCTION_H
