#include "Interpreter.h"

#include "Instruction.h"
#include "instructions.h"

#include <QStack>

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
    // parse for matching brackets
    QStack<int> stack;
    for (int i = 0; i < m_program.size(); i++) {
        if (m_program.at(i) == '[') {
            stack.push(i);
        } else if (m_program.at(i) == ']') {
            int start = stack.pop();
            matching_bracket.insert(i, start);
            matching_bracket.insert(start, i);
        }
    }

    tape = new Tape;
    pc = 0;
    while (pc < m_program.size()) {
        Instruction * instruction = m_instructions.value(m_program.at(pc), m_noop);
        instruction->execute();
        pc++;
    }
}
