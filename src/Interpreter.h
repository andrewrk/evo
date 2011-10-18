#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <QByteArray>

class Interpreter
{
public:
    Interpreter(QByteArray program);

    void start();
};

#endif // INTERPRETER_H
