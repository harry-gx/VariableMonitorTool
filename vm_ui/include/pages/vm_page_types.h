#pragma once

#include <QString>

struct UiVariable {
    UiVariable() {}
    UiVariable(const QString &variableName, const QString &variableAddress,
               const QString &variableSize, bool variableWritable, const QString &variableTypeName = QString(),
               bool variableBitField = false, quint8 variableBitOffset = 0, quint8 variableBitSize = 0)
        : name(variableName),
          address(variableAddress),
          size(variableSize),
          writable(variableWritable),
          typeName(variableTypeName),
          bitField(variableBitField),
          bitOffset(variableBitOffset),
          bitSize(variableBitSize) {}

    QString name;
    QString address;
    QString size;
    bool writable = false;
    QString typeName;
    bool bitField = false;
    quint8 bitOffset = 0;
    quint8 bitSize = 0;
};
