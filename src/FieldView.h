#pragma once

#include "utils.h"

#include <QEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QNativeGestureEvent>
#include <QObject>
#include <QScrollBar>

class FieldView : public QGraphicsView {
  Q_OBJECT
public:
  FieldView(QWidget *parent = nullptr);

  void setBackgroundImage(const QPixmap &pix);

  qreal LengthToQreal(Length value);
  Length QrealToLength(qreal value);
  QPointF fieldToScene(Point point);
  Point sceneToField(QPointF point);
  QGraphicsScene *getScene() const;

protected:
  bool event(QEvent *event) override;
  // allows pinch on touchpads
  bool nativeGestureEvent(QNativeGestureEvent *event);

  void wheelEvent(QWheelEvent *event) override;

private:
  QGraphicsScene *scene{nullptr};
  QGraphicsPixmapItem *bgItem{nullptr};
  double image_width{2000}, image_height{2000};
  Length total_field_length = 144.0_in; // inches
};
