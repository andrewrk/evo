#ifndef CMDLINEOPTIONS_H
#define CMDLINEOPTIONS_H

#include <QString>
#include <QHash>
#include <QStringList>
#include <QSet>

class CmdLineOptions
{
public:
    CmdLineOptions();

    // ex: ("v", "version", QString())
    void addNamedArgument(QString short_name, QString long_name, QString default_value, bool mandatory = false);
    void addPositionalArgument(QString arg_name);

    // if parsing was successful, 0, otherwise 1.
    // prints usage to stderr if unsuccessful.
    int parse();

    QString argumentValue(QString arg_name) const;
    QString argumentValue(int position) const;

    int positionalArgumentCount() const;

    // prints usage to stderr
    void printUsage();

private:
    QHash<QString, QString> m_short_to_long;
    QHash<QString, QString> m_named_args;
    QStringList m_positional_args;
    QStringList m_positional_arg_names;
    QSet<QString> m_mandatory_args;
    QString m_exe_name;
};

#endif // CMDLINEOPTIONS_H
