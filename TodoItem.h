#pragma once

#include <QString>
#include <QDateTime>
#include <QColor>

struct TodoItem {
    QString title;
    QDateTime created;
    QDateTime completed;
    QString notes;
    QColor color;
    bool done = false;
};

Q_DECLARE_METATYPE(TodoItem)