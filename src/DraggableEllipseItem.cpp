#include "DraggableEllipseItem.h"
#include "moc_DraggableEllipseItem.cpp"

DraggableEllipseItem::DraggableEllipseItem(QPointF center, qreal r,
                                           bool movable, QGraphicsItem *parent)
    : QObject(nullptr), QGraphicsEllipseItem(-r / 2, -r / 2, r, r, parent) {
  setPos(center);
  setFlags(ItemIsSelectable | ItemSendsGeometryChanges);
  if (movable) {
    setFlag(ItemIsMovable, true);
  }
}

QVariant DraggableEllipseItem::itemChange(GraphicsItemChange change,
                                          const QVariant &value) {
  if (change == ItemPositionChange) {
    emit pointMoved(value.toPointF());
  }
  return QGraphicsEllipseItem::itemChange(change, value);
}
