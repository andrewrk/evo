#include "CmdLineOptions.h"
#include "Interpreter.h"

#include <cstdlib>
#include <ctime>
#include <iostream>

#include <QCoreApplication>
#include <QFile>
#include <QList>
#include <QByteArray>
#include <QMap>
#include <QTextStream>

QByteArray generateRandomProgram(int program_size);
float assignOutputScore(QByteArray goal, QByteArray actual, float cycle_usage);

char byteToChar[] = {' ', '>', '<', '+', '-', '.', ',', '[', ']'};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QTextStream stdout_(stdout);

    CmdLineOptions args;
    args.addPositionalArgument("program");
    args.addNamedArgument("g", "goal", QString());
    args.addNamedArgument("s", "seed", QString());
    args.addNamedArgument("t", "testscore", QString());
    args.addNamedArgument("l", "generationlimit", QString("-1"));
    // how many programs to use in each generation
    args.addNamedArgument("_", "gen_size", QString("60"));
    // how many programs to use to generate the next generation
    args.addNamedArgument("_", "surv_count", QString("30"));
    // how many bytes of source code
    args.addNamedArgument("_", "init_prg_size", QString("200"));
    // how many instructions to run in the program before timing out
    args.addNamedArgument("_", "cycle_count", QString("4000"));
    // when copying a gene the chance of a mutation happening
    args.addNamedArgument("_", "mut_chance", QString(".014"));


    int result = args.parse();
    if (result) return result;

    // initialize randomness
    QString seed_str = args.argumentValue("seed");
    unsigned int seed = seed_str.isNull() ? std::time(NULL) : seed_str.toUInt();
    std::srand(seed);

    QString testscore = args.argumentValue("testscore");
    if (!testscore.isNull()) {
        float score = assignOutputScore(testscore.toUtf8(), args.argumentValue(0).toUtf8(), 0);
        stdout_ << "score: " << score << "\n"; stdout_.flush();
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

    int generation_limit = args.argumentValue("generationlimit").toInt();

    // time to play with genetic algorithms!
    // goal is the goal stdout that we want to achieve.
    QFile file(goal);
    bool file_opened = file.open(QIODevice::ReadOnly);
    Q_ASSERT(file_opened);
    QByteArray goal_out_bytes = file.readAll();
    file.close();

    // configuration
    const int generation_size = args.argumentValue("gen_size").toInt();
    const int surviver_count = args.argumentValue("surv_count").toInt();
    const int init_program_size = args.argumentValue("init_prg_size").toInt();
    const qint64 timeout_cycle_count = args.argumentValue("cycle_count").toInt();
    const float mutation_chance = args.argumentValue("mut_chance").toFloat();

    stdout_ << "(C) seed=" << seed
            << " gen_size=" << generation_size
            << " surviver_count=" << surviver_count
            << " init_prg_size=" << init_program_size
            << " timeout_cycle_count=" << timeout_cycle_count
            << " mutation_chance=" << mutation_chance
            << "\n";
    stdout_.flush();

    // generate a set of random starting programs
    QList<QByteArray> program_set;
    for (int i = 0; i < generation_size; i++) {
        program_set.append(generateRandomProgram(init_program_size));
        stdout_ << "generated random bf program " << i << ":\n" << program_set.last() << "\n";
        stdout_.flush();
    }

    int generation_count = 0;
    while (generation_count != generation_limit) {
        generation_count++;
        stdout_ << "Generation " << generation_count << "\n";
        float generation_score = 0;
        float generation_max_score = 0;
        float generation_program_size = 0;

        // evaluate the set of programs and give a score to each
        QMap<float, QByteArray> program_scores;
        for (int i = 0; i < program_set.size(); i++) {
            generation_program_size += program_set.at(i).size();
            stdout_ << "evaluating program " << i << "\n";
            Interpreter * interp = new Interpreter(program_set.at(i));
            interp->setCaptureOutput(true);
            interp->setInput(QByteArray());
            interp->setMaxCycles(timeout_cycle_count);
            interp->start();

            stdout_ << "cycle count: " << interp->cycleCount() << "\n";

            QByteArray output = interp->stdout_->readAll().toUtf8();
            float cycle_usage = interp->cycleCount() / (float)timeout_cycle_count;

            delete interp;

            stdout_ << "output:\n" << output << "\n";

            float score = assignOutputScore(goal_out_bytes, output, cycle_usage);
            generation_score += score;
            if (score > generation_max_score)
                generation_max_score = score;

            stdout_ << "output score: " << score << "\n";
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
                program_set[next_generation_index] = QByteArray();
                for (int byte_index = 0; byte_index < it.value().size(); byte_index++) {
                    char byte = it.value().at(byte_index);
                    // mutate?
                    float rand_float = rand() / (float) RAND_MAX;
                    if (rand_float < mutation_chance) {
                        // mutate!
                        int mutate_action = rand() % 3;
                        if (mutate_action == 0) {
                            // change a byte randomly
                            program_set[next_generation_index].append(byteToChar[std::rand() % 9]);
                        } else if (mutate_action == 1) {
                            // insert a random byte
                            program_set[next_generation_index].append(byteToChar[std::rand() % 9]);
                            program_set[next_generation_index].append(byte);

                        } else {
                            // don't copy this byte
                        }
                    } else {
                        // copy gene (byte) to program. no errors
                        program_set[next_generation_index].append(byte);
                    }
                }

                stdout_ << "Generated new code for program " << next_generation_index
                        << ":\n" << program_set[next_generation_index] << "\n";

                next_generation_index++;
            }

            survivor_index++;
        }

        stdout_ << "(S) Generation=" << generation_count << " avg_score=" << generation_score / generation_size
                << " max_score=" << generation_max_score << " avg_prg_size=" << generation_program_size / generation_size << "\n";
        stdout_.flush();
    }
}

QByteArray generateRandomProgram(int program_size) {
    QByteArray program(program_size, ' ');
    for (int i = 0; i < program_size; i++) {
        program[i] = byteToChar[std::rand() % 9];
    }
    return program;
}

float assignOutputScore(QByteArray goal, QByteArray actual, float cycle_usage) {
    // if actual is blank, score is 0
    // if goal == actual score is perfect 1

    float score = 0.0f;
    float char_count = goal.size();
    for (int i = 0; i < goal.size() && i < actual.size(); i++) {
        float ascii_diff = std::abs(goal[i] - actual[i]);
        float delta = (1.0f - ascii_diff / 255.0f) * (1.0f / char_count);
        //stdout_ << "character " << i << " off by " << ascii_diff << ", adding " << delta << "to score";
        score += delta;
    }

    // penalty for actual being too long
    float how_much_longer = actual.size() - goal.size();
    if (how_much_longer > 0) {
        float max_penalty = .10f;
        float penalty = (how_much_longer / 100.0f) * max_penalty;
        score -= penalty;
    }

    // penalty for using more cycles
    float max_cycle_penalty = 0.10f;
    float cycle_penalty = cycle_usage * cycle_usage * max_cycle_penalty;
    score -= cycle_penalty;

    if (score < 0.0f)
        score = 0.0f;
    else if (score > 1.0f)
        score = 1.0f;
    return score;
}
