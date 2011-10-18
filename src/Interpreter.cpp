#include "Interpreter.h"

#include "Instruction.h"
#include "instructions.h"

Interpreter::Interpreter(QByteArray program) :
    tape(NULL),
    m_program(program)
{
    m_instructions.insert('>', new IncrementHeadInstruction(this));
    m_instructions.insert('<', new DecrementHeadInstruction(this));
    m_instructions.insert('+', new IncrementAtHeadInstruction(this));
    m_instructions.insert('-', new DecrementAtHeadInstruction(this));
    m_instructions.insert('.', new OutputHeadInstruction(this));
    m_instructions.insert(',', new InputHeadInstruction(this));
    m_instructions.insert('[', new BeginLoopInstruction(this));
    m_instructions.insert(']', new EndLoopInstruction(this));

    m_noop = new NoopInstruction(this);
}

Interpreter::~Interpreter()
{
    delete tape;

    QHashIterator<char, Instruction *> it(m_instructions);
    while (it.hasNext()) {
        it.next();
        delete it.value();
    }
}

void Interpreter::start()
{
    tape = new Tape;
    m_pc = 0;
    while (m_pc < m_program.size()) {
        Instruction * instruction = m_instructions.value(m_program.at(m_pc), m_noop);
        instruction->execute();
        m_pc++;
    }
}
