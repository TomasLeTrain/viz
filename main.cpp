#include <qabstractscrollarea.h>
#include <qcolor.h>
#include <qdebug.h>
#include <qevent.h>
#include <qlogging.h>
#include <qobject.h>
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
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
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

using Point = units::V2FPosition;
class FieldView : public QGraphicsView {
  Q_OBJECT
public:
  FieldView(QWidget *parent = nullptr) : QGraphicsView(parent) {
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    // setDragMode(QGraphicsView::ScrollHandDrag);
    setDragMode(QGraphicsView::RubberBandDrag);

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    scene = new QGraphicsScene(this);
    setScene(scene);
  }

  void setBackgroundImage(const QPixmap &pix) {
    if (bgItem) {
      scene->removeItem(bgItem);
      delete bgItem;
      bgItem = nullptr;
    }
    bgItem = scene->addPixmap(pix);

    // behind everything
    bgItem->setZValue(-100);
    auto original_bounds = bgItem->boundingRect();
    image_width = original_bounds.width();
    image_height = original_bounds.width();

    // leaves about 500 pixels of padding
    scene->setSceneRect(-500, -500, image_width + 1000, image_height + 1000);

    // makes it actually smooth
    bgItem->setTransformationMode(Qt::SmoothTransformation);

    resetTransform();

    // resize to fit whole image (?)
    fitInView(200, 200, 1000, 1000, Qt::AspectRatioMode::KeepAspectRatio);
  }

  qreal LengthToQreal(Length value) {
    return value * (image_width / total_field_length);
  }
  Length QrealToLength(qreal value) {
    return value * (total_field_length / image_width);
  }

  QPointF PointToQPoint(Point point) {
    return {(point.x / total_field_length + 0.5) * image_width,
            (point.y / total_field_length + 0.5) * image_height};
  }

  Point QPointToPoint(QPointF point) {
    return {(point.x() / image_width - 0.5) * total_field_length,
            (point.y() / image_height - 0.5) * total_field_length};
  }

  // Add a point (x,y in field coordinates)
  QGraphicsEllipseItem *drawPoint(Point point, Length radius = 2_in,
                                  const QColor &c = Qt::red) {
    QPointF q_point = PointToQPoint(point);
    qreal qradius = LengthToQreal(radius);
    QGraphicsEllipseItem *it =
        scene->addEllipse(q_point.x() - qradius / 2, q_point.y() - qradius / 2,
                          qradius, qradius, QPen(Qt::NoPen), QBrush(c));
    // scale with view
    it->setFlag(QGraphicsItem::ItemIgnoresTransformations, false);
    it->setZValue(10);
    return it;
  }

  QGraphicsPathItem *addPath(const QVector<QPointF> &pts,
                             const QColor &c = Qt::green, qreal width = 2.0) {
    QPainterPath p;
    if (!pts.isEmpty()) {
      p.moveTo(pts.first());
      for (int i = 1; i < pts.size(); ++i)
        p.lineTo(pts[i]);
    }
    QGraphicsPathItem *it = scene->addPath(p, QPen(c, width));
    it->setZValue(5);
    return it;
  }
  void removeItem(QGraphicsItem *item) { scene->removeItem(item); }

protected:
  bool event(QEvent *event) override {
    if (event->type() == QEvent::NativeGesture) {
      return nativeGestureEvent(static_cast<QNativeGestureEvent *>(event));
    }

    return QWidget::event(event);
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

  float image_width = 2000;
  float image_height = 2000;

  const Length total_field_length = 144_in;
};

// card in sidebar
class ComponentCard : public QFrame {
  Q_OBJECT
public:
  ComponentCard(QString title, int priority, QWidget *infoWidget,
                QWidget *parent = nullptr)
      : QFrame(parent), priority(priority), infoWidget(infoWidget) {

    setFrameStyle(QFrame::NoFrame);

    // contains title then info
    auto *lay = new QVBoxLayout(this);
    // contains title and remove box
    auto *h = new QHBoxLayout;

    QLabel *title_label = new QLabel(QString("<b>%1</b>").arg(title));
    expand_collapse_btn = new QToolButton;

    expand_collapse_btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    expand_collapse_btn->setArrowType(Qt::ArrowType::UpArrow);

    // TODO: expand across the entire title so its a lot easier to access
    expand_collapse_btn->setFixedWidth(20);

    connect(expand_collapse_btn, &QPushButton::clicked, this,
            &ComponentCard::expand_collapse);

    h->addWidget(title_label);
    h->addStretch();
    h->addWidget(expand_collapse_btn);

    lay->addLayout(h);

    lay->addWidget(infoWidget);
  }

  int getPriority() const { return priority; }
  void setPriority(int p) { priority = p; }

  void setInfoWidget(QWidget *new_info_widget) {
    delete infoWidget;
    infoWidget = new_info_widget;
  }

  QWidget *getInfoWidget() const { return infoWidget; }

  void setId(int id) { this->id = id; }
  int getId() { return this->id; }

private slots:
  void expand_collapse() {
    bool is_visible = infoWidget->isVisible();
    if (is_visible) {
      infoWidget->setVisible(false);
      expand_collapse_btn->setArrowType(Qt::ArrowType::DownArrow);
      // remove_btn->setText(QString("Expand"));
    } else {
      infoWidget->setVisible(true);
      expand_collapse_btn->setArrowType(Qt::ArrowType::UpArrow);
      // remove_btn->setText(QString("Collapse"));
    }
  }

private:
  QToolButton *expand_collapse_btn;
  int priority;
  QWidget *infoWidget;
  int id;
};

class Sidebar : public QWidget {
  Q_OBJECT
public:
  Sidebar(QWidget *parent = nullptr) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    setMinimumWidth(260);

    scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameStyle(QFrame::NoFrame);

    container = new QWidget;
    vlay = new QVBoxLayout(container);
    vlay->addStretch();

    scroll->setWidget(container);

    lay->addWidget(scroll);
    QPushButton *add = new QPushButton("Add random component");
    connect(add, &QPushButton::clicked, this, &Sidebar::addRandomComponent);
    lay->addWidget(add);
  }

  // returns id associated with the component
  int addComponent(ComponentCard *card) {
    curr_id++;
    card->setId(curr_id);

    cards.push_back(card);
    refreshOrder();

    return curr_id;
  }

public slots:
  int addRandomComponent() {
    static int n = 1;
    QString id = QString("comp_%1").arg(n++);
    int p = rand() % 100;

    QLabel *infoWidget = new QLabel(QString("info... %1").arg(p));
    infoWidget->setWordWrap(true);

    return addComponent(
        new ComponentCard(QString("Component %1").arg(id), p, infoWidget));
  }

  void removeComponentById(int id) {
    auto it = std::find_if(cards.begin(), cards.end(),
                           [&](ComponentCard *c) { return c->getId() == id; });
    if (it != cards.end()) {
      ComponentCard *c = *it;
      cards.erase(it);
      vlay->removeWidget(c);
      c->deleteLater();
      refreshOrder();
    }
  }

  // update the priority and reorder
  void refreshOrder() {
    // sort cards by priority descending
    std::sort(cards.begin(), cards.end(),
              [](ComponentCard *a, ComponentCard *b) {
                return a->getPriority() > b->getPriority();
              });

    // clear existing widgets from layout
    QLayoutItem *child;

    // removes all children
    while ((child = vlay->takeAt(0)) != nullptr)
      ;

    for (ComponentCard *c : cards) {
      vlay->addWidget(c);
    }
    vlay->addStretch();
    container->setLayout(vlay);
  }

private:
  QScrollArea *scroll;
  QWidget *container;
  QVBoxLayout *vlay;
  QVector<ComponentCard *> cards;
  // number of components that have been added so far
  // used to identify individual components
  int curr_id = 0;
};

class Element {
public:
  virtual void draw(FieldView *fieldView) = 0;
  virtual void clear(FieldView *fieldView) = 0;

  virtual Point getClosestPoint(Point target) = 0;
  virtual ~Element() = default;
};

class PointElement : public Element {
public:
  PointElement(Point point, Length radius = 2_in, const QColor &color = Qt::red)
      : point(point), radius(radius), color(color) {}

  void draw(FieldView *fieldView) override {
    fieldView->drawPoint(point, radius, color);
  }
  void clear(FieldView *fieldView) override {
    fieldView->removeItem(it);
    delete it;
    it = nullptr;
  }
  Point getClosestPoint(Point target) override { return point; }

private:
  Point point;
  Length radius = 2_in;
  QColor color = Qt::red;
  QGraphicsEllipseItem *it;
};

// TODO: how to handle when deleting elements?
class ElementManager {
public:
  ElementManager() {}

  Point getclosestPoint(Point target) {
    Point result = units::origin<Length>;
    Length current_best = 10000_in;

    for (auto element : elements) {
      const Point current_point = element->getClosestPoint(target);
      if (const Length curr_dist = current_point.distanceTo(target);
          curr_dist < current_best) {
        result = current_point;
        current_best = curr_dist;
      }
    }

    return result;
  }

  void addElement(Element *element) { elements.push_back(element); }

  void drawElements(FieldView *fieldView) {
    for (auto element : elements) {
      element->draw(fieldView);
    }
  }

  void clearElements(FieldView *fieldView) {
    for (auto element : elements) {
      element->clear(fieldView);
    }
  }

private:
  std::vector<Element *> elements;
};

class PathComponentInfo : public QWidget {
  Q_OBJECT
public:
  PathComponentInfo(QWidget *parent = nullptr) : QWidget(parent) {
    auto *lay = new QHBoxLayout(this);

    QLabel *velocityLabel = new QLabel("Velocity: ");
    slider = new QSlider(Qt::Horizontal);
    velocityNumberLabel = new QLabel("0");

    connect(slider, &QSlider::valueChanged, this,
            &PathComponentInfo::onSliderChanged);

    lay->addWidget(velocityLabel);
    lay->addWidget(slider);
    lay->addWidget(velocityNumberLabel);
  }
private slots:
  void onSliderChanged(int value) {
    double pct = value / 1000.0;

    float maxVel = 80;
    float curr_vel = maxVel * pct;

    velocityNumberLabel->setText(QString("%1").arg(curr_vel));
    emit velocityChanged(curr_vel);
  }
signals:
  void velocityChanged(float pct);

private:
  QSlider *slider;
  QLabel *velocityNumberLabel;
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

    mainSplit->setHandleWidth(20);
    fieldSplit->setHandleWidth(20);

    fieldView = new FieldView;
    fieldSplit->addWidget(fieldView);

    // Timeline at bottom
    auto *bottom = new QWidget;
    auto *bl = new QHBoxLayout(bottom);

    timeLabel = new QLabel("0:00");
    slider = new QSlider(Qt::Horizontal);

    // TODO: make each tick be a second (or less?)
    slider->setRange(0, 1000);
    connect(slider, &QSlider::valueChanged, this,
            &FieldWindow::onSliderChanged);
    timeEndLabel = new QLabel("0:00");

    bl->addWidget(timeLabel);
    bl->addWidget(slider);
    bl->addWidget(timeEndLabel);

    fieldSplit->addWidget(bottom);

    mainSplit->addWidget(fieldSplit);

    sidebar = new Sidebar;
    mainSplit->addWidget(sidebar);

    QPixmap image;
    image.load("assets/V5RC-PushBack-H2H.png");

    fieldView->setBackgroundImage(image);

    element_manager.addElement(new PointElement({0_in, 0_in}));
    element_manager.addElement(new PointElement({24_in, 24_in}));
    element_manager.drawElements(fieldView);

    auto info = new PathComponentInfo;
    QObject::connect(info, &PathComponentInfo::velocityChanged,
                     [&](float new_vel) {
                       // handle the change in velocity
                       std::cout << "changed to" << new_vel << std::endl;
                     });
    ComponentCard *card = new ComponentCard("howdy", 2, info);
    sidebar->addComponent(card);
  }

private slots:
  void onSliderChanged(int value) {
    double pct = value / 1000.0;

    int totalSeconds = 100;

    int secs = int(pct * totalSeconds);
    int m = secs / 60;
    int s = secs % 60;

    int total_secs = totalSeconds;
    int total_m = total_secs / 60;
    int total_s = total_secs % 60;

    timeLabel->setText(QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0')));
    timeEndLabel->setText(
        QString("%1:%2").arg(total_m).arg(total_s, 2, 10, QChar('0')));
  }

private:
  FieldView *fieldView;
  ElementManager element_manager;

  QLabel *timeLabel;
  QSlider *slider;
  QLabel *timeEndLabel;

  Sidebar *sidebar;
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

// class PathManager {
//   PathManager(int path) : path(path) {}
//   ComponentCard *makeComponentCard() {}
//   // void setComponentId(int id) { component_id = id; }
//
//   void show(Sidebar *sidebar_instance, FieldView *field_instance) {
//     // should add the component to sidebar
//     // should show on field
//     component_id = sidebar_instance->addComponent(makeComponentCard());
//
//     QVector<QPointF> pts = {QPointF{2, 2}, QPointF{4, 4}};
//     QGraphicsPathItem *path_field_item = field_instance->addPath(pts);
//   }
//
//   void wantToRemove() {
//     // signal?
//   }
//
//   void wantToAdd() {
//     // signal?
//   }
//
// private:
//   int path;
//   int component_id;
// };

void add_path(int path) {
  // TODO: impl
  // add to field
  // add info to sidebar
  // add to some list that keeps tracks of all elements
}

void add_point_cloud() {
  // TODO: impl
  // add to field
  // add info to sidebar
  // add to some list that keeps tracks of all elements
}

void load_file() {
  // TODO: impl
  // load field image?
  // load paths / point clouds
}

// TODO:
// add keybinds (left/right arrows, space, more?)

int main(int argc, char **argv) {
  QApplication app(argc, argv);

  MainWindow w;

  w.show();
  return app.exec();
}

#include "main.moc"
