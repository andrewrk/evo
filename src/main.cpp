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
float assignOutputScore(QByteArray goal, QByteArray actual);

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    CmdLineOptions args;
    args.addPositionalArgument("program");
    args.addNamedArgument("g", "goal", QString());
    args.addNamedArgument("s", "seed", QString());
    args.addNamedArgument("t", "testscore", QString());
    int result = args.parse();
    if (result) return result;

    // initialize randomness
    QString seed_str = args.argumentValue("seed");
    unsigned int seed = seed_str.isNull() ? std::time(NULL) : seed_str.toUInt();
    std::srand(seed);

    QString testscore = args.argumentValue("testscore");
    if (!testscore.isNull()) {
        float score = assignOutputScore(testscore.toUtf8(), args.argumentValue(0).toUtf8());
        qDebug() << "score:" << score;
        return 0;
    }

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
    bool file_opened = file.open(QIODevice::ReadOnly);
    Q_ASSERT(file_opened);
    QByteArray goal_out_bytes = file.readAll();
    file.close();

    // configuration
    const int generation_size = 10;
    const int program_size = 200;
    const int parent_count = 2; // how many parents to mate when making babies
    const qint64 timeout_cycle_count = 5000; // how many instructions to run in the program before timing out
    const float mutation_chance = .10f; // when creating a baby the chance of a mutation happening

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
        interp->setInput(QByteArray());
        interp->setMaxCycles(timeout_cycle_count);
        interp->start();

        qDebug() << "cycle count: " << interp->cycleCount();

        QByteArray output = interp->stdout_->readAll().toUtf8();

        delete interp;

        qDebug() << "output:\n" << output;

        float score = assignOutputScore(goal_out_bytes, output);

        qDebug() << "output score: " << score;
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

float assignOutputScore(QByteArray goal, QByteArray actual) {
    // if actual is blank, score is 0
    // if goal == actual score is perfect 1

    float score = 0.0f;
    float char_count = goal.size();
    for (int i = 0; i < goal.size() && i < actual.size(); i++) {
        float ascii_diff = std::abs(goal[i] - actual[i]);
        float delta = (1.0f - ascii_diff / 255.0f) * (1.0f / char_count);
        //qDebug() << "character " << i << " off by " << ascii_diff << ", adding " << delta << "to score";
        score += delta;
    }

    // penalty for actual being too long
    float how_much_longer = actual.size() - goal.size();
    if (how_much_longer > 0) {
        float max_penalty = .10f;
        float penalty = (how_much_longer / 1000.0f) * max_penalty;
        score -= penalty;
    }

    if (score < 0.0f)
        score = 0.0f;
    else if (score > 1.0f)
        score = 1.0f;
    return score;
}
