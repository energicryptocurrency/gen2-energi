#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include <QScrollBar>
#include <QWidget>
#include <QObject>
#include <QPaintEvent>

class RequestsScrollBar : public QScrollBar {
    Q_OBJECT
public:
    RequestsScrollBar(QWidget *parent = Q_NULLPTR) : QScrollBar(parent) {}
    RequestsScrollBar(Qt::Orientation orientation, QWidget *parent = Q_NULLPTR) : QScrollBar(orientation, parent) {}
Q_SIGNALS:
    void painted();
protected:
    virtual void paintEvent(QPaintEvent * event);
};

#endif // SCROLLBAR_H
