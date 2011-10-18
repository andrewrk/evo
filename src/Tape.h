#ifndef TAPE_H
#define TAPE_H

#include <QVector>

class Tape
{
public:
    Tape();

    void incrementHead();
    void decrementHead();

    void incrementAtHead();
    void decrementAtHead();

    char readFromHead();
    void writeToHead(char byte);

private:
    int m_head;
    QVector<char> m_memory;
};

#endif // TAPE_H
