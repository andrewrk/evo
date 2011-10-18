#include "CmdLineOptions.h"
#include <iostream>

#include <QCoreApplication>


CmdLineOptions::CmdLineOptions()
{
}

int CmdLineOptions::parse()
{
    QStringList args = QCoreApplication::instance()->arguments();
    m_exe_name = args.at(0);
    for (int i = 1; i < args.size(); i++) {
        QString arg = args.at(i);
        if (arg.startsWith("-")) {
            // named argument
            QString stripped_arg = arg.startsWith("--") ? arg.mid(2) : arg.mid(1);
            QString arg_name = m_short_to_long.value(stripped_arg, stripped_arg);
            if (i + 1 == args.size()) {
                std::cerr << "Missing argument value for " << arg_name.toStdString() << "\n\n";
                printUsage();
                return 1;
            }
            m_mandatory_args.remove(arg_name);
            QString value = args.at(++i);
            m_named_args.insert(arg_name, value);
        } else {
            // positional argument
            m_positional_args.append(arg);
        }
    }

    // make sure all positional args are present
    if (positionalArgumentCount() < m_positional_arg_names.size()) {
        for (int i = positionalArgumentCount(); i < m_positional_arg_names.size(); i++) {
            std::cerr << "Missing mandatory positional argument: " << m_positional_arg_names.at(i).toStdString() << "\n";
        }
        std::cerr << "\n";
        printUsage();
        return 1;
    }

    // make sure all mandatory named args are present
    if (m_mandatory_args.size() > 0) {
        foreach (QString arg_name, m_mandatory_args) {
            std::cerr << "Missing mandatory named argument: " << arg_name.toStdString() << "\n";
        }
        std::cerr << "\n";
        printUsage();
        return 1;
    }
    return 0;
}

void CmdLineOptions::addNamedArgument(QString short_name, QString long_name, QString default_value, bool mandatory)
{
    m_short_to_long.insert(short_name, long_name);
    m_named_args.insert(long_name, default_value);
    if (mandatory)
        m_mandatory_args.insert(long_name);
}

QString CmdLineOptions::argumentValue(QString arg_name) const
{
    return m_named_args.value(m_short_to_long.value(arg_name, arg_name));
}

QString CmdLineOptions::argumentValue(int position) const
{
    return m_positional_args.at(position);
}

int CmdLineOptions::positionalArgumentCount() const
{
    return m_positional_args.size();
}

void CmdLineOptions::printUsage()
{
    std::cerr << "Usage:\n\n";
    std::cerr << m_exe_name.toStdString() << " " << m_positional_arg_names.join(" ").toStdString() << "\n\n";
    foreach (QString arg, m_named_args) {
        std::cerr << arg.toStdString() << "\n";
    }
}

void CmdLineOptions::addPositionalArgument(QString arg_name)
{
    m_positional_arg_names.append(arg_name);
}

