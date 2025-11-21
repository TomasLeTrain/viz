#pragma once

#include "DraggableEllipseItem.h"
#include "Element.h"
#include "Point.h"
#include "utils.h"

#include "geometry/Bezier.h"
#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <qgraphicsitem.h>
#include <qpainterpath.h>

struct BezierElementProperties {
  Length strokeWidth = 1_in;
  Length controlPointRadius = 4_in;
  QColor pathColor = Qt::green;
  QColor controlPointColor = QColor(255, 0, 0, 127);
  bool movable = true;
};

class BezierModel : public QObject, public ElementModel {
  Q_OBJECT
public:
  BezierModel(geometry::CubicBezier *bezier);
  std::array<Point, 4> endpoints() const;

public slots:
  void setEndpoints(const std::array<Point, 4> &endpoints);

signals:
  void endpointsChanged(const std::array<Point, 4> &endpoints);

private:
  geometry::CubicBezier *m_bezier;
};

class BezierView : public QObject, public ElementView {
  Q_OBJECT
public:
  BezierView(BezierModel *model, FieldView *fieldView,
             BezierElementProperties properties);

  ~BezierView() override;

  std::array<QPointF, 4> endpointsToScene(std::array<Point, 4> endpoints);
  QPainterPath createPath(std::array<Point, 4> endpoints);

  QGraphicsPathItem *graphicsItem() const;

public slots:
  void onModelChanged(const std::array<Point, 4> &endpoints);
  void onItemChanged(const QPointF &endpoint, int index);

private:
  BezierModel *m_model;
  FieldView *m_fieldView;
  QGraphicsPathItem *item{nullptr};

  std::array<DraggableEllipseItem *, 4> control_items = {nullptr, nullptr,
                                                         nullptr, nullptr};

  BezierElementProperties m_properties;
};

class BezierInfoWidget : public QWidget {
  Q_OBJECT
public:
  BezierInfoWidget(BezierModel *model, bool editable = true,
                   QWidget *parent = nullptr);

public slots:
  void updateFromModel(const std::array<Point, 4> &new_endpoints);

  void applyEdits();

private:
  BezierModel *m_model;
  std::array<QLineEdit *, 4> xEdit;
  std::array<QLineEdit *, 4> yEdit;
};
