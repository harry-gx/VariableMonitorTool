/*
 * 文件说明：UI 界面模块头文件，声明相关类型和函数接口。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#pragma once

#include <QString>

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
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

    /* 变量说明：name，变量名、页面名或节点名。 */
    QString name;
    /* 变量说明：address，MCU 目标内存地址或协议地址。 */
    QString address;
    /* 变量说明：size，数据长度，单位为字节。 */
    QString size;
    /* 变量说明：false，保存当前对象运行所需的状态、参数或缓存数据。 */
    bool writable = false;
    /* 变量说明：typeName，保存当前对象运行所需的状态、参数或缓存数据。 */
    QString typeName;
    /* 变量说明：false，保存当前对象运行所需的状态、参数或缓存数据。 */
    bool bitField = false;
    quint8 bitOffset = 0;
    quint8 bitSize = 0;
};
