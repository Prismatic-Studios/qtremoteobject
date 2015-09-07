/****************************************************************************
**
** Copyright (C) 2014 Ford Motor Company
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qcommandlineoption.h>
#include <qcommandlineparser.h>
#include <qcoreapplication.h>
#include <qfileinfo.h>

#include "cppcodegenerator.h"
#include "moc.h"
#include "preprocessor.h"
#include "repcodegenerator.h"
#include "repparser.h"
#include "utils.h"

#include <cstdio>

#define PROGRAM_NAME  "repc"
#define REPC_VERSION  "0.1"

enum Mode {
    InRep = 1,
    InSrc = 2,
    OutRep = 4,
    OutSource = 8,
    OutReplica = 16
};

static const QLatin1String REP("rep");
static const QLatin1String SRC("src");
static const QLatin1String REPLICA("replica");
static const QLatin1String SOURCE("source");

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QString::fromLatin1(REPC_VERSION));

    QString outputFile;
    QString inputFile;
    int mode = 0;
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("repc tool v%1 (Qt %2).\n")
                                     .arg(QStringLiteral(REPC_VERSION)).arg(QString::fromLatin1(QT_VERSION_STR)));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption inputTypeOption(QStringLiteral("i"));
    inputTypeOption.setDescription(QLatin1String("Input file type:\n"
                                                  "rep: replicant template files.\n"
                                                  "src: C++ QObject derived classes."));
    inputTypeOption.setValueName(QStringLiteral("rep|src"));
    parser.addOption(inputTypeOption);

    QCommandLineOption outputTypeOption(QStringLiteral("o"));
    outputTypeOption.setDescription(QLatin1String("Output file type:\n"
                                                   "source: generates source header. Is incompatible with \"-i src\" option.\n"
                                                   "replica: generates replica header.\n"
                                                   "rep: generates replicant template file from C++ QOject classes. Is not compatible with \"-i rep\" option."));
    outputTypeOption.setValueName(QStringLiteral("source|replica|rep"));
    parser.addOption(outputTypeOption);

    QCommandLineOption includePathOption(QStringLiteral("I"));
    includePathOption.setDescription(QStringLiteral("Add dir to the include path for header files. This parameter is needed only if the input file type is src (.h file)."));
    includePathOption.setValueName(QStringLiteral("dir"));
    parser.addOption(includePathOption);

    QCommandLineOption debugOption(QStringLiteral("d"));
    debugOption.setDescription(QStringLiteral("Print out parsing debug information (for troubleshooting)."));
    parser.addOption(debugOption);

    parser.addPositionalArgument(QStringLiteral("[header-file/rep-file]"),
            QStringLiteral("Input header/rep file to read from, otherwise stdin."));

    parser.addPositionalArgument(QStringLiteral("[rep-file/header-file]"),
            QStringLiteral("Output header/rep file to write to, otherwise stdout."));

    parser.process(app.arguments());

    const QStringList files = parser.positionalArguments();

    if (files.count() > 2) {
        fprintf(stderr, "%s", qPrintable(QLatin1String(PROGRAM_NAME ": Too many input, output files specified: '") + files.join(QStringLiteral("' '")) + QStringLiteral("\'.\n")));
        parser.showHelp(1);
    }

    if (parser.isSet(inputTypeOption)) {
        const QString &inputType = parser.value(inputTypeOption);
        if (inputType == REP)
            mode = InRep;
        else if (inputType == SRC)
            mode = InSrc;
        else {
            fprintf(stderr, PROGRAM_NAME ": Unknown input type\"%s\".\n", qPrintable(inputType));
            parser.showHelp(1);
        }
    }

    if (parser.isSet(outputTypeOption)) {
        const QString &outputType = parser.value(outputTypeOption);
        if (outputType == REP)
            mode |= OutRep;
        else if (outputType == REPLICA)
            mode |= OutReplica;
        else if (outputType == SOURCE)
            mode |= OutSource;
        else {
            fprintf(stderr, PROGRAM_NAME ": Unknown output type\"%s\".\n", qPrintable(outputType));
            parser.showHelp(1);
        }
    }

    switch (files.count()) {
    case 2:
        outputFile = files.last();
        if (!(mode & (OutRep | OutSource | OutReplica))) {
            // try to figure out the Out mode from file extension
            if (outputFile.endsWith(QLatin1Literal(".rep")))
                mode |= OutRep;
        }
        // fall through
    case 1:
        inputFile = files.first();
        if (!(mode & (InRep | InSrc))) {
            // try to figure out the In mode from file extension
            if (inputFile.endsWith(QLatin1Literal(".rep")))
                mode |= InRep;
            else
                mode |= InSrc;
        }
        break;
    }
    // check mode sanity
    if (!(mode & (InRep | InSrc))) {
        fprintf(stderr, PROGRAM_NAME ": Unknown input type, please use -i option to specify one.\n");
        parser.showHelp(1);
    }
    if (!(mode & (OutRep | OutSource | OutReplica))) {
        fprintf(stderr, PROGRAM_NAME ": Unknown output type, please use -o option to specify one.\n");
        parser.showHelp(1);
    }
    if (mode & InRep && mode & OutRep) {
        fprintf(stderr, PROGRAM_NAME ": Invalid input/output type combination, both are rep files.\n");
        parser.showHelp(1);
    }
    if (mode & InSrc && mode & OutSource) {
        fprintf(stderr, PROGRAM_NAME ": Invalid input/output type combination, both are source header files.\n");
        parser.showHelp(1);
    }

    QFile input;
    if (inputFile.isEmpty()) {
        inputFile = QStringLiteral("standard input");
        input.open(stdin, QIODevice::ReadOnly);
    } else {
        input.setFileName(inputFile);
        if (!input.open(QIODevice::ReadOnly)) {
            fprintf(stderr, PROGRAM_NAME ": %s: No such file.\n", qPrintable(inputFile));
            return 1;
        }
    }

    QFile output;
    if (outputFile.isEmpty()) {
        output.open(stdout, QIODevice::WriteOnly);
    } else {
        output.setFileName(outputFile);
        if (!output.open(QIODevice::WriteOnly)) {
            fprintf(stderr, PROGRAM_NAME ": could not open output file '%s': %s.\n",
                    qPrintable(outputFile), qPrintable(output.errorString()));
            return 1;
        }
    }

    if (mode & InSrc) {
        Preprocessor pp;
        const QFileInfo includePath(QLatin1String(RO_INSTALL_HEADERS));
        pp.includes += Preprocessor::IncludePath(QFile::encodeName(includePath.canonicalFilePath()));
        pp.includes += Preprocessor::IncludePath(QFile::encodeName(includePath.canonicalPath()));
        foreach (const QString &path, parser.values(includePathOption))
            pp.includes += Preprocessor::IncludePath(QFile::encodeName(path));

        pp.macros["Q_MOC_RUN"];
        pp.macros["__cplusplus"];

        Moc moc;
        if (!inputFile.isEmpty())
            moc.filename = inputFile.toLocal8Bit();
        moc.currentFilenames.push(inputFile.toLocal8Bit());
        moc.includes = pp.includes;
        moc.symbols = pp.preprocessed(moc.filename, &input);
        moc.parse();

        if (moc.classList.isEmpty()) {
            fprintf(stderr, PROGRAM_NAME ": No QObject classes found.\n");
            return 0;
        }
        input.close();
        if (mode & OutRep) {
            CppCodeGenerator generator(&output);
            generator.generate(moc.classList);
        } else {
            Q_ASSERT(mode & OutReplica);
            RepCodeGenerator generator(&output);
            generator.generate(classList2AST(moc.classList), RepCodeGenerator::REPLICA, outputFile);
        }
        output.close();
    } else {
        Q_ASSERT(!(mode & OutRep));
        RepParser repparser(input);
        if (parser.isSet(debugOption))
            repparser.setDebug();
        if (!repparser.parse()) {
            fprintf(stderr, PROGRAM_NAME ": Can't parse input file.\n");
            fprintf(stderr, "%s", PROGRAM_NAME ": ");
            fprintf(stderr, "%s\n", qPrintable(repparser.errorString()));
            return 1;
        }
        input.close();
        RepCodeGenerator generator(&output);
        generator.generate(repparser.ast(), (mode & OutSource) ? RepCodeGenerator::SOURCE
                                                            : RepCodeGenerator::REPLICA, outputFile);
        output.close();
    }
    return 0;
}
