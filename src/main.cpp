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
#include <QMap>

QByteArray generateRandomProgram(int program_size);
float assignOutputScore(QByteArray goal, QByteArray actual);

char byteToChar[] = {' ', '>', '<', '+', '-', '.', ',', '[', ']'};

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
    const int generation_size = 20; // how many programs to use in each generation
    const int surviver_count = 10; // how many programs to use to generate the next generation
    const int program_size = 200; // how many bytes of source code
    const qint64 timeout_cycle_count = 4000; // how many instructions to run in the program before timing out
    const float mutation_chance = .005f; // when copying a gene the chance of a mutation happening

    // generate a set of random starting programs
    QList<QByteArray> program_set;
    for (int i = 0; i < generation_size; i++) {
        program_set.append(generateRandomProgram(program_size));
        qDebug() << "generated random bf program " << i << ":\n" << program_set.last();
    }

    int generation_count = 0;
    while (true) {
        generation_count++;
        qDebug() << "Generation" << generation_count;
        float generation_score = 0;
        float generation_max_score = 0;

        // evaluate the set of programs and give a score to each
        QMap<float, QByteArray> program_scores;
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
            generation_score += score;
            if (score > generation_max_score)
                generation_max_score = score;

            qDebug() << "output score: " << score;
            program_scores.insertMulti(score, program_set.at(i));

        }

        // take a subset of the top scoring programs and breed them to get a new
        // set of programs to evaluate
        QMapIterator<float, QByteArray> it(program_scores);
        it.toBack();
        int survivor_index = 0;
        int babies_per_program = generation_size / surviver_count;
        int next_generation_index = 0;
        while (it.hasPrevious() && survivor_index < surviver_count) {
            it.previous();

            for (int baby = 0; baby < babies_per_program; baby++) {
                // how is babby formed?
                program_set[next_generation_index] = QByteArray(program_size, ' ');
                for (int byte_index = 0; byte_index < it.value().size(); byte_index++) {
                    char byte = it.value().at(byte_index);
                    // mutate?
                    float rand_float = rand() / (float) RAND_MAX;
                    if (rand_float < mutation_chance) {
                        // mutate!
                        byte = byteToChar[std::rand() % 9];
                    }
                    // copy gene (byte) to program
                    program_set[next_generation_index][byte_index] = byte;
                }

                qDebug() << "Generated new code for program" << next_generation_index << ":\n" << program_set[next_generation_index];

                next_generation_index++;
            }

            survivor_index++;
        }

        qDebug() << "Generation" << generation_count << "Average fitness:" << generation_score / generation_size << " Max fitness:" << generation_max_score;
    }
}

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
