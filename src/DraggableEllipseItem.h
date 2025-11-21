#pragma once

#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QObject>
#include <QPointF>

class DraggableEllipseItem : public QObject, public QGraphicsEllipseItem {
  Q_OBJECT
public:
  DraggableEllipseItem(QPointF center, qreal r = 6, bool movable = true,
                       QGraphicsItem *parent = nullptr);

signals:
  void pointMoved(const QPointF &newPos);

protected:
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant &value) override;
};
