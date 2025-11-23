#include "Bezier.h"
#include "DraggableEllipseItem.h"
#include "moc_Bezier.cpp"
#include <algorithm>
#include <qgraphicsitem.h>
#include <qline.h>
#include <qnamespace.h>

BezierModel::BezierModel(geometry::CubicBezier *bezier) : m_bezier(bezier) {}

std::array<Point, 4> BezierModel::endpoints() const {
  return m_bezier->getControlPoints();
}

void BezierModel::setEndpoints(const std::array<Point, 4> &endpoints) {
  // only emit if changed, prevents infinite loop
  if (m_bezier->getControlPoints() == endpoints) {
    return;
  }
  m_bezier->updateBezierEndpoints(endpoints);

  emit endpointsChanged(endpoints);
}

BezierView::BezierView(BezierModel *model, FieldView *fieldView,
                       BezierElementProperties properties)
    : QObject(nullptr), m_model(model), m_fieldView(fieldView) {

  auto endpoints = model->endpoints();
  auto newEndpoints = endpointsToScene(endpoints);

  auto path = createPath(endpoints);
  item = new QGraphicsPathItem(path);

  item->setBrush(Qt::NoBrush);
  item->setPen(QPen(m_properties.pathColor,
                    m_fieldView->LengthToQreal(m_properties.strokeWidth)));
  item->setZValue(5);
  m_fieldView->getScene()->addItem(item);

  auto first_line = QLineF(newEndpoints.at(0), newEndpoints.at(1));
  auto second_line = QLineF(newEndpoints.at(2), newEndpoints.at(3));

  control_line_items[0] = m_fieldView->getScene()->addLine(
      first_line, QPen(Qt::red, 2, Qt::DashLine));
  control_line_items[1] = m_fieldView->getScene()->addLine(
      second_line, QPen(Qt::red, 2, Qt::DashLine));

  connect(m_model, &BezierModel::endpointsChanged, this,
          &BezierView::onModelChanged);

  // TODO: add lines that make it easier to differentiate control points

  // add every endpoint as
  for (int i = 0; i < 4; i++) {
    qreal radiusPixels =
        m_fieldView->LengthToQreal(m_properties.controlPointRadius);

    control_items[i] = new DraggableEllipseItem(newEndpoints[i], radiusPixels,
                                                properties.movable);

    control_items[i]->setBrush(QBrush(properties.controlPointColor));
    control_items[i]->setPen(Qt::NoPen);
    control_items[i]->setZValue(10);
    m_fieldView->getScene()->addItem(control_items[i]);

    // binds connect with specific index
    connect(control_items[i], &DraggableEllipseItem::pointMoved, this,
            [this, i](const QPointF &endpoint) { onItemChanged(endpoint, i); });
  }
}

std::array<QPointF, 4>
BezierView::endpointsToScene(std::array<Point, 4> endpoints) {
  std::array<QPointF, 4> newEndpoints;

  std::ranges::transform(endpoints.begin(), endpoints.end(),
                         newEndpoints.begin(),
                         [this](auto original_endpoint) -> QPointF {
                           return m_fieldView->fieldToScene(original_endpoint);
                         });
  return newEndpoints;
}

QPainterPath BezierView::createPath(std::array<Point, 4> endpoints) {
  auto newEndpoints = endpointsToScene(endpoints);
  QPainterPath path(newEndpoints.at(0));

  path.cubicTo(newEndpoints.at(1), newEndpoints.at(2), newEndpoints.at(3));

  return path;
}

void BezierView::drawControlLines(std::array<Point, 4> endpoints) {
  auto newEndpoints = endpointsToScene(endpoints);
  auto first_line = QLineF(newEndpoints.at(0), newEndpoints.at(1));
  auto second_line = QLineF(newEndpoints.at(2), newEndpoints.at(3));

  control_line_items[0]->setLine(first_line);
  control_line_items[1]->setLine(second_line);
}

BezierView::~BezierView() {
  if (item) {
    m_fieldView->getScene()->removeItem(item);
    delete item;
    item = nullptr;
  }
  for (DraggableEllipseItem *control_item : control_items) {
    if (control_item) {
      m_fieldView->getScene()->removeItem(control_item);
      delete control_item;
      control_item = nullptr;
    }
  }
}

QGraphicsPathItem *BezierView::graphicsItem() const { return item; }

void BezierView::onModelChanged(const std::array<Point, 4> &endpoints) {
  drawControlLines(endpoints);
  item->setPath(createPath(endpoints));
}

void BezierView::onItemChanged(const QPointF &scene_endpoint, int index) {
  Point endpoint = m_fieldView->sceneToField(scene_endpoint);

  auto new_endpoints = m_model->endpoints();
  new_endpoints[index] = endpoint;

  m_model->setEndpoints(new_endpoints);
}

BezierInfoWidget::BezierInfoWidget(BezierModel *model, bool editable,
                                   QWidget *parent)
    : QWidget(parent), m_model(model) {
  auto *lay = new QVBoxLayout(this);

  auto endpoints = m_model->endpoints();

  // update UI when model updates
  connect(m_model, &BezierModel::endpointsChanged, this,
          &BezierInfoWidget::updateFromModel);

  for (int i = 0; i < endpoints.size(); i++) {
    auto *control_widget = new QWidget;
    auto *control_lay = new QHBoxLayout(control_widget);

    xEdit[i] = new QLineEdit;
    yEdit[i] = new QLineEdit;

    xEdit[i]->setReadOnly(!editable);
    yEdit[i]->setReadOnly(!editable);

    control_lay->addWidget(new QLabel("x:"));
    control_lay->addWidget(xEdit[i]);
    control_lay->addWidget(new QLabel("y:"));
    control_lay->addWidget(yEdit[i]);

    QString title;
    if (i == 0)
      title = QString("<b>Start</b>");
    else if (i == 3)
      title = QString("<b>End<b>");
    else
      title = QString("<b>Control %2</b>").arg(i);

    lay->addWidget(new QLabel(title));
    lay->addWidget(control_widget);

    // update model when user edits
    connect(xEdit[i], &QLineEdit::editingFinished, this,
            &BezierInfoWidget::applyEdits);
    connect(yEdit[i], &QLineEdit::editingFinished, this,
            &BezierInfoWidget::applyEdits);
  }

  // init
  updateFromModel(m_model->endpoints());
}

void BezierInfoWidget::updateFromModel(
    const std::array<Point, 4> &new_endpoints) {

  for (int i = 0; i < xEdit.size(); i++) {
    xEdit[i]->setText(QString::number(new_endpoints[i].x.convert(in)));
    yEdit[i]->setText(QString::number(new_endpoints[i].y.convert(in)));
  }
}

void BezierInfoWidget::applyEdits() {
  bool conversions_ok = true, tmp_ok1, tmp_ok2;

  std::array<Point, 4> newEndpoints;

  for (int i = 0; i < xEdit.size(); i++) {
    double x = xEdit[i]->text().toDouble(&tmp_ok1);
    double y = yEdit[i]->text().toDouble(&tmp_ok2);
    if (!tmp_ok1 || !tmp_ok2) {
      conversions_ok = false;
      break;
    }

    newEndpoints[i] = {x * in, y * in};
  }

  if (conversions_ok)
    m_model->setEndpoints(newEndpoints);
}
