#include "Robot.h"
#include "moc_Robot.cpp"
#include <QGraphicsPolygonItem>
#include <qgraphicsitem.h>
#include <qpen.h>
#include <qwidget.h>

RobotModel::RobotModel(Pose pose) : m_pose(pose) {}
Pose RobotModel::pose() const { return m_pose; }

void RobotModel::setPose(const Pose &pose) {
  // only emit if changed, prevents infinite loop
  if (pose.x == m_pose.x && pose.y == m_pose.y &&
      pose.orientation == m_pose.orientation)
    return;
  m_pose = pose;
  emit poseChanged(m_pose);
}

RobotView::RobotView(RobotModel *model, FieldView *fieldView,
                     RobotElementProperties properties)
    : QObject(nullptr), m_model(model), m_fieldView(fieldView),
      m_properties(properties) {
  // qreal radiusPixels = m_fieldView->LengthToQreal(m_properties.point_radius);

  auto pose = m_model->pose();
  QPointF scenePos = m_fieldView->fieldToScene(pose);

  item = new QGraphicsRectItem(makeRect());

  item->setBrush(QBrush(properties.color));
  item->setPen(QPen(properties.outline_color));
  item->setZValue(30);

  m_fieldView->getScene()->addItem(item);

  // done after to use parent coordinates
  item->setRotation(to_cDeg(pose.orientation));
  item->setPos(scenePos);

  connect(m_model, &RobotModel::poseChanged, this, &RobotView::onModelMoved);
}

RobotView::~RobotView() {
  if (item) {
    m_fieldView->getScene()->removeItem(item);
    delete item;
    item = nullptr;
  }
}

QGraphicsRectItem *RobotView::graphicsItem() const { return item; }

QRectF RobotView::makeRect() {
  Length half_width = m_properties.robotWidth / 2;
  Length half_height = m_properties.robotHeight / 2;

  // leaves center at (0,0)
  Point top_left = Point{-half_width, -half_height};

  QRectF robotPolygon =
      QRectF(m_fieldView->LengthToQreal(top_left.x),
             m_fieldView->LengthToQreal(top_left.y),
             m_fieldView->LengthToQreal(m_properties.robotWidth),
             m_fieldView->LengthToQreal(m_properties.robotHeight));
  return robotPolygon;
}

void RobotView::onModelMoved(const Pose &pose) {
  QPointF scenePos = m_fieldView->fieldToScene(pose);
  item->setRect(makeRect());

  qDebug() << item->rect().center() << '\n';

  item->setRotation(to_cDeg(pose.orientation));
  item->setPos(scenePos);

  qDebug() << item->scenePos() << "\n";
}

RobotInfoWidget::RobotInfoWidget(RobotModel *model, bool editable,
                                 QWidget *parent)
    : QWidget(parent), m_model(model) {
  auto *lay = new QVBoxLayout(this);

  xEdit = new QLineEdit;
  yEdit = new QLineEdit;
  headingEdit = new QLineEdit;

  xEdit->setReadOnly(!editable);
  yEdit->setReadOnly(!editable);
  headingEdit->setReadOnly(!editable);

  auto xWidget = new QWidget;
  auto xLay = new QHBoxLayout(xWidget);
  xLay->addWidget(new QLabel("x:"));
  xLay->addWidget(xEdit);
  lay->addWidget(xWidget);

  auto yWidget = new QWidget;
  auto yLay = new QHBoxLayout(yWidget);
  yLay->addWidget(new QLabel("y:"));
  yLay->addWidget(yEdit);
  lay->addWidget(yWidget);

  auto thetaWidget = new QWidget;
  auto thetaLay = new QHBoxLayout(thetaWidget);
  thetaLay->addWidget(new QLabel("theta:"));
  thetaLay->addWidget(headingEdit);
  lay->addWidget(thetaWidget);

  // update UI when model updates
  connect(m_model, &RobotModel::poseChanged, this,
          &RobotInfoWidget::updateFromModel);
  // update model when user edits
  connect(xEdit, &QLineEdit::editingFinished, this,
          &RobotInfoWidget::applyEdits);
  connect(yEdit, &QLineEdit::editingFinished, this,
          &RobotInfoWidget::applyEdits);
  connect(headingEdit, &QLineEdit::editingFinished, this,
          &RobotInfoWidget::applyEdits);

  // init
  updateFromModel(m_model->pose());
}

void RobotInfoWidget::updateFromModel(const Pose &pose) {
  xEdit->setText(QString::number(pose.x.convert(in)));
  yEdit->setText(QString::number(pose.y.convert(in)));
  headingEdit->setText(QString::number(pose.orientation.convert(deg)));
}

void RobotInfoWidget::applyEdits() {
  bool ok1, ok2, ok3;
  double x = xEdit->text().toDouble(&ok1);
  double y = yEdit->text().toDouble(&ok2);
  double theta = headingEdit->text().toDouble(&ok3);
  if (ok1 && ok2 && ok3) {
    m_model->setPose(Pose(x * in, y * in, theta * deg));
  }
}
