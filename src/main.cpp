#include "geometry/Bezier.h"
#include "utils.h"

#include "Bezier.h"
#include "ComponentCard.h"
#include "DraggableEllipseItem.h"
#include "Element.h"
#include "FieldView.h"
#include "Point.h"
#include "TimelineWidget.h"

#include <qabstractscrollarea.h>
#include <qbrush.h>
#include <qcolor.h>
#include <qdebug.h>
#include <qevent.h>
#include <qlogging.h>
#include <qobject.h>
#include <qtmetamacros.h>

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
#include <qgesture.h>
#include <qgraphicsitem.h>
#include <qmainwindow.h>
#include <qnamespace.h>
#include <qpoint.h>
#include <qtypes.h>
#include <qwidget.h>

class ElementManager : public QObject {
  Q_OBJECT
public:
  ElementManager(FieldView *fieldView, QWidget *sidebarContainer)
      : m_fieldView(fieldView), m_sidebar(sidebarContainer) {}

  PointModel *addPoint(Point point, PointElementProperties properties) {
    PointModel *model = new PointModel(point);

    PointView *view = new PointView(model, m_fieldView, properties);

    PointInfoWidget *info = new PointInfoWidget(model, properties.movable);
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

  BezierModel *addBezier(geometry::CubicBezier *bezier,
                         BezierElementProperties properties) {
    BezierModel *model = new BezierModel(bezier);

    BezierView *view = new BezierView(model, m_fieldView, properties);

    BezierInfoWidget *info = new BezierInfoWidget(model, properties.movable);
    ComponentCard *card = new ComponentCard("Bezier", info, 5);

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
    QPushButton *add = new QPushButton("Add Point");

    sideLay->addWidget(add);

    mainSplit->addWidget(sideHolder);

    elementManager = new ElementManager(fieldView, container);

    QPixmap image;
    image.load("assets/V5RC-PushBack-H2H.png");
    fieldView->setBackgroundImage(image);

    // add demo points (models, views, sidebar cards)
    elementManager->addPoint(Point(0_in, 0_in), {.movable = true});
    elementManager->addPoint(Point(24_in, 24_in), {.movable = true});
    elementManager->addPoint(Point(-24_in, -24_in),
                             {.color = Qt::green, .movable = false});

    auto test_bezier = new geometry::CubicBezier({0_in, 0_in}, {10_in, 0_in},
                                                 {0_in, 10_in}, {24_in, 24_in});

    elementManager->addBezier(test_bezier, {});

    connect(add, &QPushButton::clicked, this,
            [this] { elementManager->addPoint(Point(0_in, 0_in), {}); });
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
