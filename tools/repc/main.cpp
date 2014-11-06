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

#include <QCoreApplication>

#include "repcodegenerator.h"
#include "repparser.h"

#include <cstdio>

void usage()
{
    printf("repc [-s/-r] -i input-file -o output-file\n");
    printf("  -s outputs the source version from the template\n");
    printf("  -r outputs the replica version from the template\n");
    exit(-1);
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (app.arguments().size() < 3 || app.arguments().contains(QLatin1String("--help")))
        usage();

    const int idxOfInput = app.arguments().indexOf(QLatin1String("-i"))+1;
    const int idxOfOutput = app.arguments().indexOf(QLatin1String("-o"))+1;
    if (idxOfInput <= 1 && idxOfOutput <= 1)
        usage();

    RepCodeGenerator::Mode mode;
    if (app.arguments().contains(QLatin1String("-s")))
        mode = RepCodeGenerator::SOURCE;
    else if (app.arguments().contains(QLatin1String("-r")))
        mode = RepCodeGenerator::REPLICA;
    else
        usage();

    const QString input = app.arguments().at(idxOfInput);
    const QString output = app.arguments().at(idxOfOutput);

    RepParser parser(input);
    if (!parser.parse())
        return 1;

    RepCodeGenerator generator(output);
    if (!generator.generate(parser.ast(), mode))
        return 2;

    return 0;
}