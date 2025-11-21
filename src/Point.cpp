#include "Point.h"
#include "moc_Point.cpp"

PointModel::PointModel(Point pos) : m_pos(pos) {}
Point PointModel::position() const { return m_pos; }

void PointModel::setPosition(const Point &point) {
  // only emit if changed, prevents infinite loop
  if (point.x == m_pos.x && point.y == m_pos.y)
    return;
  m_pos = point;
  emit positionChanged(m_pos);
}

PointView::PointView(PointModel *model, FieldView *fieldView,
                     PointElementProperties properties)
    : QObject(nullptr), m_model(model), m_fieldView(fieldView),
      m_radius(properties.radius) {
  QPointF scenePos = m_fieldView->fieldToScene(m_model->position());
  qreal radiusPixels = m_fieldView->LengthToQreal(m_radius);

  item = new DraggableEllipseItem(scenePos, radiusPixels, properties.movable);

  item->setBrush(QBrush(properties.color));
  item->setPen(Qt::NoPen);
  item->setZValue(10);
  m_fieldView->getScene()->addItem(item);

  connect(m_model, &PointModel::positionChanged, this,
          &PointView::onModelMoved);

  connect(item, &DraggableEllipseItem::pointMoved, this,
          &PointView::onItemMoved);
}

PointView::~PointView() {
  if (item) {
    m_fieldView->getScene()->removeItem(item);
    delete item;
    item = nullptr;
  }
}

DraggableEllipseItem *PointView::graphicsItem() const { return item; }

void PointView::onModelMoved(const Point &p) {
  QPointF scenePos = m_fieldView->fieldToScene(p);
  item->setPos(scenePos);
}

void PointView::onItemMoved(const QPointF &point) {
  Point p = m_fieldView->sceneToField(point);
  m_model->setPosition(p);
}

PointInfoWidget::PointInfoWidget(PointModel *model, bool editable,
                                 QWidget *parent)
    : QWidget(parent), m_model(model) {
  auto *lay = new QHBoxLayout(this);
  xEdit = new QLineEdit;
  yEdit = new QLineEdit;

  xEdit->setReadOnly(!editable);
  yEdit->setReadOnly(!editable);

  lay->addWidget(new QLabel("x:"));
  lay->addWidget(xEdit);
  lay->addWidget(new QLabel("y:"));
  lay->addWidget(yEdit);

  // update UI when model updates
  connect(m_model, &PointModel::positionChanged, this,
          &PointInfoWidget::updateFromModel);
  // update model when user edits
  connect(xEdit, &QLineEdit::editingFinished, this,
          &PointInfoWidget::applyEdits);
  connect(yEdit, &QLineEdit::editingFinished, this,
          &PointInfoWidget::applyEdits);

  // init
  updateFromModel(m_model->position());
}

void PointInfoWidget::updateFromModel(const Point &p) {
  xEdit->setText(QString::number(p.x.convert(in)));
  yEdit->setText(QString::number(p.y.convert(in)));
}

void PointInfoWidget::applyEdits() {
  bool ok1, ok2;
  double x = xEdit->text().toDouble(&ok1);
  double y = yEdit->text().toDouble(&ok2);
  if (ok1 && ok2) {
    m_model->setPosition(Point(x * in, y * in));
  }
}
