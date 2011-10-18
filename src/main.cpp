#include <iostream>
#include "CmdLineOptions.h"
#include "Interpreter.h"

#include <QCoreApplication>

#include <QFile>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    CmdLineOptions args;
    args.addPositionalArgument("program");
    int result = args.parse();
    if (result) return result;

    QString program_filename = args.argumentValue(0);

    std::cout << program_filename.toStdString() << "\n";
    return 0;

    // get program bytes
    QFile file(program_filename);
    file.open(QIODevice::ReadOnly);
    QByteArray program_bytes = file.readAll();

    // run the interpreter on it
    Interpreter interp(program_bytes);
    interp.start();


    return 0;
}
