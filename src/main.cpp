#include "TimelineWidget.h"

#include <qabstractscrollarea.h>
#include <qbrush.h>
#include <qcolor.h>
#include <qdebug.h>
#include <qevent.h>
#include <qlogging.h>
#include <qobject.h>
#include <qtmetamacros.h>
#define M_TWOPI (2 * M_PI)

#include "units/Vector2D.hpp"
#include "units/units.hpp"
#include <QApplication>
#include <QFileDialog>
#include <QFrame>
#include <QGesture>
#include <QGestureRecognizer>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyleOption>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>
#include <iostream>
#include <qgesture.h>
#include <qgraphicsitem.h>
#include <qmainwindow.h>
#include <qnamespace.h>
#include <qpoint.h>
#include <qtypes.h>
#include <qwidget.h>

using Point = units::V2Position;

class DraggableEllipseItem : public QObject, public QGraphicsEllipseItem {
  Q_OBJECT
public:
  DraggableEllipseItem(qreal x = 0, qreal y = 0, qreal r = 6,
                       QGraphicsItem *parent = nullptr)
      : QObject(nullptr), QGraphicsEllipseItem(-r / 2, -r / 2, r, r, parent) {

    setPos(x, y);
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
  }

signals:
  void pointMoved(const QPointF &newPos);

protected:
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant &value) override {
    if (change == ItemPositionChange) {
      emit pointMoved(value.toPointF());
    }
    return QGraphicsEllipseItem::itemChange(change, value);
  }

private:
};

class FieldView : public QGraphicsView {
  Q_OBJECT
public:
  FieldView(QWidget *parent = nullptr) : QGraphicsView(parent) {
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

  void setBackgroundImage(const QPixmap &pix) {
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

  qreal LengthToQreal(Length value) {
    return value * (image_width / total_field_length);
  }
  Length QrealToLength(qreal value) {
    return value * (total_field_length / image_width);
  }

  QPointF fieldToScene(Point point) {
    return {(point.x / total_field_length + 0.5) * image_width,
            (-point.y / total_field_length + 0.5) * image_height};
  }

  Point sceneToField(QPointF point) {
    return {(point.x() / image_width - 0.5) * total_field_length,
            (-(point.y() / image_height - 0.5)) * total_field_length};
  }

  QGraphicsScene *getScene() const { return scene; }

protected:
  bool event(QEvent *event) override {
    if (event->type() == QEvent::NativeGesture) {
      return nativeGestureEvent(static_cast<QNativeGestureEvent *>(event));
    }

    return QGraphicsView::event(event);
  }

  // allows pinch on touchpads
  bool nativeGestureEvent(QNativeGestureEvent *event) {
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

  void wheelEvent(QWheelEvent *event) override {
    auto hor = horizontalScrollBar();
    auto ver = verticalScrollBar();

    const qreal sensitivity = 0.1;

    hor->setValue(hor->value() - event->angleDelta().x() * sensitivity);
    ver->setValue(ver->value() - event->angleDelta().y() * sensitivity);
  }

private:
  QGraphicsScene *scene{nullptr};
  QGraphicsPixmapItem *bgItem{nullptr};
  double image_width{2000}, image_height{2000};
  Length total_field_length = 144.0_in; // inches
};

class ElementModel {};
class ElementView {};

class PointModel : public QObject, public ElementModel {
  Q_OBJECT
public:
  PointModel(const Point &pos = Point()) : m_pos(pos) {}
  Point position() const { return m_pos; }

public slots:
  void setPosition(const Point &point) {
    // only emit if changed, prevents infinite loop
    if (point.x == m_pos.x && point.y == m_pos.y)
      return;
    m_pos = point;
    emit positionChanged(m_pos);
  }

signals:
  void positionChanged(const Point &newPos);

private:
  Point m_pos;
};

class PointView : public QObject, public ElementView {
  Q_OBJECT
public:
  PointView(PointModel *model, FieldView *fieldView, Length radius = 2_in,
            const QColor &color = Qt::red)
      : QObject(nullptr), m_model(model), m_fieldView(fieldView),
        m_radius(radius) {
    // create graphics item in scene
    QPointF scenePos = m_fieldView->fieldToScene(m_model->position());
    qreal radiusPixels = m_fieldView->LengthToQreal(m_radius);

    item = new DraggableEllipseItem(scenePos.x(), scenePos.y(), radiusPixels);

    item->setBrush(QBrush(color));
    item->setPen(Qt::NoPen);
    item->setZValue(10);
    m_fieldView->getScene()->addItem(item);

    // model -> view: when model changes, move item
    connect(m_model, &PointModel::positionChanged, this,
            &PointView::onModelMoved);

    // view -> model: when item moved, update model with converted coords
    connect(item, &DraggableEllipseItem::pointMoved, this,
            &PointView::onItemMoved);
  }

  ~PointView() {
    if (item) {
      m_fieldView->getScene()->removeItem(item);
      delete item;
      item = nullptr;
    }
  }

  DraggableEllipseItem *graphicsItem() const { return item; }

public slots:
  void onModelMoved(const Point &p) {
    QPointF scenePos = m_fieldView->fieldToScene(p);
    item->setPos(scenePos);
  }

  void onItemMoved(const QPointF &point) {
    Point p = m_fieldView->sceneToField(point);
    m_model->setPosition(p);
  }

private:
  PointModel *m_model;
  FieldView *m_fieldView;
  DraggableEllipseItem *item{nullptr};
  Length m_radius;
};

class PointInfoWidget : public QWidget {
  Q_OBJECT
public:
  PointInfoWidget(PointModel *model, QWidget *parent = nullptr)
      : QWidget(parent), m_model(model) {
    auto *lay = new QHBoxLayout(this);
    xEdit = new QLineEdit;
    yEdit = new QLineEdit;
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

public slots:
  void updateFromModel(const Point &p) {
    xEdit->setText(QString::number(p.x.convert(in)));
    yEdit->setText(QString::number(p.y.convert(in)));
  }

  void applyEdits() {
    bool ok1, ok2;
    double x = xEdit->text().toDouble(&ok1);
    double y = yEdit->text().toDouble(&ok2);
    if (ok1 && ok2) {
      m_model->setPosition(Point(x * in, y * in));
    }
  }

private:
  PointModel *m_model;
  QLineEdit *xEdit;
  QLineEdit *yEdit;
};

class ComponentCard : public QFrame {
  Q_OBJECT
public:
  ComponentCard(const QString &title, QWidget *infoWidget, int priority,
                QWidget *parent = nullptr)
      : QFrame(parent), m_priority(priority) {
    setFrameShape(QFrame::StyledPanel);
    auto *lay = new QVBoxLayout(this);
    auto *topLay = new QHBoxLayout;
    topLay->addWidget(new QLabel("<b>" + title + "</b>"));
    topLay->addStretch();

    QToolButton *expandCollapseBtn = new QToolButton;

    auto toggleBtn = [infoWidget, expandCollapseBtn]() {
      infoWidget->setVisible(!infoWidget->isVisible());
      expandCollapseBtn->setArrowType(infoWidget->isVisible() ? Qt::UpArrow
                                                              : Qt::DownArrow);
    };

    expandCollapseBtn->setArrowType(Qt::UpArrow);
    connect(expandCollapseBtn, &QToolButton::clicked, this, toggleBtn);

    topLay->addWidget(expandCollapseBtn);
    lay->addLayout(topLay);
    lay->addWidget(infoWidget);
  }
  int getPriority() { return m_priority; }
  void setPriority(int priority) { m_priority = priority; }

private:
  int m_priority;
};

class ElementManager : public QObject {
  Q_OBJECT
public:
  ElementManager(FieldView *fieldView, QWidget *sidebarContainer)
      : m_fieldView(fieldView), m_sidebar(sidebarContainer) {}

  PointModel *addPoint(Point point) {
    PointModel *model = new PointModel(point);

    PointView *view = new PointView(model, m_fieldView, 2_in, Qt::red);

    PointInfoWidget *info = new PointInfoWidget(model);
    ComponentCard *card = new ComponentCard("Point", info, 10);

    m_models.append(model);
    m_views.append(view);
    m_cards.append(card);

    // assumes sidebar has a layout
    if (m_sidebar->layout()) {
      m_sidebar->layout()->addWidget(card);
    }
    refreshSidebar();

    return model;
  }

  void clear() {
    // delete views/models/cards
    for (auto v : m_views)
      delete v;
    m_views.clear();
    for (auto m : m_models)
      delete m;
    m_models.clear();
    for (auto c : m_cards) {
      c->deleteLater();
    }
    m_cards.clear();
  }

private:
  void refreshSidebar() {
    if (!m_sidebar)
      return;

    QVBoxLayout *vlay = qobject_cast<QVBoxLayout *>(m_sidebar->layout());
    if (!vlay)
      return;

    QLayoutItem *child;
    while ((child = vlay->takeAt(0)) != nullptr) {
      if (child->widget()) {
        child->widget()->setParent(nullptr);
      }
      delete child;
    }

    // sort by priority descending
    std::sort(m_cards.begin(), m_cards.end(),
              [](ComponentCard *a, ComponentCard *b) {
                return a->getPriority() > b->getPriority();
              });

    // re-add widgets in sorted order
    for (ComponentCard *element : m_cards) {
      if (element)
        vlay->addWidget(element);
    }

    // keep the stretch at the end (so items stick to top)
    vlay->addStretch();
  }

  FieldView *m_fieldView;
  QWidget *m_sidebar;
  QList<ElementModel *> m_models;
  QList<ElementView *> m_views;
  QList<ComponentCard *> m_cards;
};

class FieldWindow : public QWidget {
  Q_OBJECT
public:
  FieldWindow() {
    auto *rootLayout = new QHBoxLayout(this);

    // separates sidebar from field/timeline
    auto *mainSplit = new QSplitter(Qt::Horizontal);
    rootLayout->addWidget(mainSplit);

    // separates field from timeline
    QSplitter *fieldSplit = new QSplitter(Qt::Vertical);

    mainSplit->setHandleWidth(10);
    fieldSplit->setHandleWidth(10);

    fieldView = new FieldView;
    fieldSplit->addWidget(fieldView);
    fieldSplit->addWidget(new TimelineWidget);

    mainSplit->addWidget(fieldSplit);

    QWidget *sideHolder = new QWidget;
    auto *sideLay = new QVBoxLayout(sideHolder);
    sideHolder->setMinimumWidth(240);
    // sideLay->addStretch(); // items will be inserted before stretch

    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameStyle(QFrame::NoFrame);

    auto container = new QWidget;
    auto vLay = new QVBoxLayout(container);
    vLay->addStretch();

    scroll->setWidget(container);

    sideLay->addWidget(scroll);
    QPushButton *add = new QPushButton("Add random component");
    // connect(add, &QPushButton::clicked, this, &Sidebar::addRandomComponent);
    sideLay->addWidget(add);

    mainSplit->addWidget(sideHolder);

    elementManager = new ElementManager(fieldView, container);

    QPixmap image;
    image.load("assets/V5RC-PushBack-H2H.png");
    fieldView->setBackgroundImage(image);

    // add demo points (models, views, sidebar cards)
    elementManager->addPoint(Point(0_in, 0_in));
    elementManager->addPoint(Point(24_in, 24_in));
  }

private:
  FieldView *fieldView;
  ElementManager *elementManager;
};

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow() {
    stacked = new QStackedWidget;
    fieldWindow = new FieldWindow;

    stacked->addWidget(fieldWindow);
    setCentralWidget(stacked);
  }

private:
  QStackedWidget *stacked;
  FieldWindow *fieldWindow;
};

int main(int argc, char **argv) {
  QApplication a(argc, argv);
  MainWindow w;
  w.show();
  return a.exec();
}

#include "main.moc"
