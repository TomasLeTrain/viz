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
#include <qgraphicsitem.h>
#include <qpolygon.h>

struct RobotElementProperties {
  Length trackWidth = 10.5_in;

  Length robotWidth = 12_in;
  Length robotHeight = 17_in;

  Length point_radius = 1_in;
  QColor color = Qt::red;
  QColor outline_color = Qt::red;
  QColor heading_color = Qt::white;
  bool movable = true;
};

class RobotModel : public QObject, public ElementModel {
  Q_OBJECT
public:
  RobotModel(Pose pose);
  Pose pose() const;

public slots:
  void setPose(const Pose &point);

signals:
  void poseChanged(const Pose &newPose);

private:
  Pose m_pose;
};

class RobotView : public QObject, public ElementView {
  Q_OBJECT
public:
  RobotView(RobotModel *model, FieldView *fieldView,
            RobotElementProperties properties);

  ~RobotView() override;

  QGraphicsRectItem *graphicsItem() const;

  QRectF makeRect();

public slots:
  void onModelMoved(const Pose &pose);

private:
  RobotModel *m_model;
  FieldView *m_fieldView;
  QGraphicsRectItem *item{nullptr};
  RobotElementProperties m_properties;
};

class RobotInfoWidget : public QWidget {
  Q_OBJECT
public:
  RobotInfoWidget(RobotModel *model, bool editable = true,
                  QWidget *parent = nullptr);

public slots:
  void updateFromModel(const Pose &pose);

  void applyEdits();

private:
  RobotModel *m_model;
  QLineEdit *xEdit;
  QLineEdit *yEdit;
  QLineEdit *headingEdit;
};
