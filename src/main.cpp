#include "CmdLineOptions.h"
#include "Interpreter.h"

#include <cstdlib>
#include <ctime>
#include <iostream>

#include <QCoreApplication>
#include <QFile>
#include <QList>
#include <QByteArray>
#include <QDebug>

QByteArray generateRandomProgram(int program_size);

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    CmdLineOptions args;
    args.addPositionalArgument("program");
    args.addNamedArgument("g", "goal", QString());
    args.addNamedArgument("s", "seed", QString());
    int result = args.parse();
    if (result) return result;

    // initialize randomness
    QString seed_str = args.argumentValue("seed");
    unsigned int seed = seed_str.isNull() ? std::time(NULL) : seed_str.toUInt();
    std::srand(seed);

    QString goal = args.argumentValue("goal");
    if (goal.isNull()) {
        // just run the program that was passed in
        QString program_filename = args.argumentValue(0);

        // get program bytes
        QFile file(program_filename);
        file.open(QIODevice::ReadOnly);
        QByteArray program_bytes = file.readAll();
        file.close();

        // run the interpreter on it
        Interpreter interp(program_bytes);
        interp.start();

        return 0;
    }

    // time to play with genetic algorithms!
    // goal is the goal stdout that we want to achieve.
    QFile file(goal);
    file.open(QIODevice::ReadOnly);
    QByteArray goal_out_bytes = file.readAll();
    file.close();

    // configuration
    const int generation_size = 10;
    const int program_size = 200;
    const int parent_count = 2; // how many parents to mate when making babies

    // generate a set of random starting programs
    QList<QByteArray> program_set;
    for (int i = 0; i < generation_size; i++) {
        program_set.append(generateRandomProgram(program_size));
        qDebug() << "generated random bf program " << i << ":\n" << program_set.last();
    }

    // evaluate the set of programs and give a score to each
    for (int i = 0; i < program_set.size(); i++) {
        qDebug() << "evaluating program" << i;
        Interpreter * interp = new Interpreter(program_set.at(i));
        interp->setCaptureOutput(true);

        interp->start();

        QString output = interp->stdout_->readAll();

        delete interp;

        qDebug() << "output:\n" << output;
    }

    // take a subset of the top scoring programs and breed them to get a new
    // set of programs to evaluate
}

char byteToChar[] = {' ', '>', '<', '+', '-', '.', ',', '[', ']'};
QByteArray generateRandomProgram(int program_size) {
    QByteArray program(program_size, ' ');
    for (int i = 0; i < program_size; i++) {
        program[i] = byteToChar[std::rand() % 9];
    }
    return program;
}
