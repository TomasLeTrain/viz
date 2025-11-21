#pragma once

#include "DraggableEllipseItem.h"
#include "Element.h"
#include "FieldView.h"
#include "utils.h"

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QObject>

struct PointElementProperties {
  Length radius = 2_in;
  QColor color = Qt::red;
  bool movable = true;
};

class PointModel : public QObject, public ElementModel {
  Q_OBJECT
public:
  PointModel(Point pos);
  Point position() const;

public slots:
  void setPosition(const Point &point);

signals:
  void positionChanged(const Point &newPos);

private:
  Point m_pos;
};

class PointView : public QObject, public ElementView {
  Q_OBJECT
public:
  PointView(PointModel *model, FieldView *fieldView,
            PointElementProperties properties);

  ~PointView() override;

  DraggableEllipseItem *graphicsItem() const;

public slots:
  void onModelMoved(const Point &p);

  void onItemMoved(const QPointF &point);

private:
  PointModel *m_model;
  FieldView *m_fieldView;
  DraggableEllipseItem *item{nullptr};
  Length m_radius;
};

class PointInfoWidget : public QWidget {
  Q_OBJECT
public:
  PointInfoWidget(PointModel *model, bool editable = true,
                  QWidget *parent = nullptr);

public slots:
  void updateFromModel(const Point &p);

  void applyEdits();

private:
  PointModel *m_model;
  QLineEdit *xEdit;
  QLineEdit *yEdit;
};
