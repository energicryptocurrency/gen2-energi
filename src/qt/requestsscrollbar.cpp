#include "requestsscrollbar.h"

void RequestsScrollBar::paintEvent(QPaintEvent * event) {
    QScrollBar::paintEvent(event);
    Q_EMIT painted();
}
