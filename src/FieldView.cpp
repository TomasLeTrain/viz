#include "FieldView.h"
#include "moc_FieldView.cpp"

FieldView::FieldView(QWidget *parent) : QGraphicsView(parent) {
  setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
  setDragMode(QGraphicsView::RubberBandDrag);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

  scene = new QGraphicsScene(this);
  setScene(scene);
  image_width = 2000;
  image_height = 2000;
  total_field_length = 140.4_in + 2 * 2_in;
  scene->setSceneRect(-500, -500, image_width + 1000, image_height + 1000);
}

void FieldView::setBackgroundImage(const QPixmap &pix) {
  if (bgItem) {
    scene->removeItem(bgItem);
    delete bgItem;
    bgItem = nullptr;
  }
  bgItem = scene->addPixmap(pix);

  if (!pix.isNull()) {
    image_width = pix.width();
    image_height = pix.height();

    // adds padding around scene
    scene->setSceneRect(-500, -500, image_width + 1000, image_height + 1000);

    // makes image actually smooth
    bgItem->setTransformationMode(Qt::SmoothTransformation);

    resetTransform();

    // resize to fit whole image (?)
    fitInView(200, 200, 1000, 1000, Qt::AspectRatioMode::KeepAspectRatio);
  }
}

qreal FieldView::LengthToQreal(Length value) {
  return value * (image_width / total_field_length);
}
Length FieldView::QrealToLength(qreal value) {
  return value * (total_field_length / image_width);
}

QPointF FieldView::fieldToScene(Point point) {
  return {(point.x / total_field_length + 0.5) * image_width,
          (-point.y / total_field_length + 0.5) * image_height};
}

Point FieldView::sceneToField(QPointF point) {
  return {(point.x() / image_width - 0.5) * total_field_length,
          (-(point.y() / image_height - 0.5)) * total_field_length};
}

QGraphicsScene *FieldView::getScene() const { return scene; }

bool FieldView::event(QEvent *event) {
  if (event->type() == QEvent::NativeGesture) {
    return nativeGestureEvent(static_cast<QNativeGestureEvent *>(event));
  }

  return QGraphicsView::event(event);
}

// allows pinch on touchpads
bool FieldView::nativeGestureEvent(QNativeGestureEvent *event) {
  if (event->gestureType() == Qt::NativeGestureType::ZoomNativeGesture) {
    const qreal power = event->value();

    const double scalingFactor = 1 + power;

    // used to cap zoom in/out
    qreal curr_scale = transform().m11();

    qreal max_zoom_in = 2.0;
    qreal max_zoom_out = 3.0;

    // zoom in
    if (scalingFactor > 1 && curr_scale <= max_zoom_in) {
      scale(scalingFactor, scalingFactor);
      // zoom out
    } else if (scalingFactor < 1 && curr_scale >= 1.0 / max_zoom_out) {
      scale(scalingFactor, scalingFactor);
    }
  }

  // allows panning while pinching (zooming)
  if (event->gestureType() == Qt::NativeGestureType::PanNativeGesture) {
    auto hor = horizontalScrollBar();
    auto ver = verticalScrollBar();

    const qreal sensitivity = 0.5;

    hor->setValue(hor->value() - event->delta().x() * sensitivity);
    ver->setValue(ver->value() - event->delta().y() * sensitivity);
  }

  return true;
}

void FieldView::wheelEvent(QWheelEvent *event) {
  auto hor = horizontalScrollBar();
  auto ver = verticalScrollBar();

  const qreal sensitivity = 0.1;

  hor->setValue(hor->value() - event->angleDelta().x() * sensitivity);
  ver->setValue(ver->value() - event->angleDelta().y() * sensitivity);
}
