#ifndef WCOVERART_H
#define WCOVERART_H

#include <QWidget>
#include <QMouseEvent>
#include <QColor>
#include <QDomNode>
#include "configobject.h"

class WCoverArt : public QWidget {
    Q_OBJECT
public:
    WCoverArt(ConfigObject<ConfigValue>* pConfig,
              QWidget *parent);
    virtual ~WCoverArt();
    void setup(QDomNode node);

protected:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void leaveEvent(QEvent *);

private:
    ConfigObject<ConfigValue>* m_pConfig;
    QImage m_coverArt;
    bool m_coverIsVisible;
    bool m_coverIsHovered;
    bool m_coverIsEmpty;
    QString m_currentCover;

public slots:
    void loadCover(QString img);

};

#endif // WCOVERART_H
