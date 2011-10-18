#include "Tape.h"

Tape::Tape() :
    m_head(0)
{
    m_memory.resize(1);
}

void Tape::incrementHead()
{
    m_head++;
    while (m_head >= m_memory.size())
        m_memory.append(0);
}

void Tape::decrementHead()
{
    m_head--;
    m_head = (m_head < 0) ? 0 : m_head;
}

void Tape::incrementAtHead()
{
    m_memory.replace(m_head, m_memory.at(m_head)+1);
}

void Tape::decrementAtHead()
{
    m_memory.replace(m_head, m_memory.at(m_head)-1);
}

char Tape::readFromHead()
{
    return m_memory.at(m_head);
}

void Tape::writeToHead(char byte)
{
    m_memory.replace(m_head, byte);
}
