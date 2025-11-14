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
#include <qmainwindow.h>
#include <qnamespace.h>

class FieldView : public QGraphicsView {
  Q_OBJECT
public:
  FieldView(QWidget *parent = nullptr) : QGraphicsView(parent) {
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    setDragMode(QGraphicsView::ScrollHandDrag);
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

    // leaves about 500 pixels of padding
    scene->setSceneRect(-500, -500, original_bounds.width() + 1000,
                        original_bounds.height() + 1000);

    // makes it actually smooth
    bgItem->setTransformationMode(Qt::SmoothTransformation);

    resetTransform();

    // resize to fit whole image
    fitInView(200, 200, 1000, 1000, Qt::AspectRatioMode::KeepAspectRatio);
  }

  // Add a point (x,y in field coordinates)
  QGraphicsEllipseItem *addPoint(qreal x, qreal y, qreal radius = 6,
                                 const QColor &c = Qt::red) {
    QGraphicsEllipseItem *it =
        scene->addEllipse(x - radius / 2, y - radius / 2, radius, radius,
                          QPen(Qt::NoPen), QBrush(c));
    it->setFlag(QGraphicsItem::ItemIgnoresTransformations,
                false); // scale with view
    it->setZValue(10);
    points.push_back(it);
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
    paths.push_back(it);
    return it;
  }

  void clearField() {
    for (auto *it : points) {
      scene->removeItem(it);
      delete it;
    }
    points.clear();
    for (auto *it : paths) {
      scene->removeItem(it);
      delete it;
    }
    paths.clear();
  }

protected:
  void wheelEvent(QWheelEvent *event) override {
    // quadratic scaling makes big changes zoom quicker
    const qreal power = pow(event->angleDelta().y() / 1000.0, 2);

    const double scalingFactor = 1 + power;

    // used to cap zoom in/out
    qreal curr_scale = transform().m11();

    qreal max_zoom_in = 2.0;
    qreal max_zoom_out = 3.0;

    if (event->angleDelta().y() > 0) {
      if (curr_scale <= max_zoom_in) {
        scale(scalingFactor, scalingFactor);
      }
    } else {
      if (curr_scale >= 1.0 / max_zoom_out) {
        scale(1.0 / scalingFactor, 1.0 / scalingFactor);
      }
    }
  }

private:
  QGraphicsScene *scene{nullptr};
  QGraphicsPixmapItem *bgItem{nullptr};
  QVector<QGraphicsEllipseItem *> points;
  QVector<QGraphicsPathItem *> paths;
};

// card in sidebar
class ComponentCard : public QFrame {
  Q_OBJECT
public:
  ComponentCard(QString id, QString title, int priority,
                QWidget *parent = nullptr)
      : QFrame(parent), id(id), priority(priority) {

    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);

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

    connect(expand_collapse_btn, &QPushButton::clicked, this, &ComponentCard::expand_collapse);

    h->addWidget(title_label);
    h->addStretch();
    h->addWidget(expand_collapse_btn);

    lay->addLayout(h);

    info = new QLabel(QString("info... %1").arg(priority));
    info->setWordWrap(true);

    lay->addWidget(info);
  }

  QString getId() const { return id; }
  int getPriority() const { return priority; }
  void setPriority(int p) { priority = p; }
  void setInfoText(const QString &s) { info->setText(s); }

signals:
  void removeRequested(const QString &id);

private slots:
  void requestRemove() { emit removeRequested(id); }
  void expand_collapse() {
    if (info->isVisible()) {
      info->setVisible(false);
      expand_collapse_btn->setArrowType(Qt::ArrowType::DownArrow);
      // remove_btn->setText(QString("Expand"));
    } else {
      info->setVisible(true);
      expand_collapse_btn->setArrowType(Qt::ArrowType::UpArrow);
      // remove_btn->setText(QString("Collapse"));
    }
  }

private:
  QToolButton *expand_collapse_btn;
  QString id;
  int priority;
  QLabel *info;
};

// ----------------------
// Sidebar manager
// ----------------------
class Sidebar : public QWidget {
  Q_OBJECT
public:
  Sidebar(QWidget *parent = nullptr) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    setMinimumWidth(260);
    scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    container = new QWidget;
    vlay = new QVBoxLayout(container);
    vlay->addStretch();
    scroll->setWidget(container);
    lay->addWidget(scroll);
    QPushButton *add = new QPushButton("Add random component");
    connect(add, &QPushButton::clicked, this, &Sidebar::addRandomComponent);
    lay->addWidget(add);
  }

  void addComponent(const QString &id, const QString &title, int priority) {
    ComponentCard *card = new ComponentCard(id, title, priority);
    connect(card, &ComponentCard::removeRequested, this,
            &Sidebar::removeComponentById);
    cards.push_back(card);
    refreshOrder();
  }

public slots:
  void addRandomComponent() {
    static int n = 1;
    QString id = QString("comp_%1").arg(n++);
    int p = rand() % 100;
    addComponent(id, QString("Component %1").arg(id), p);
  }

  void removeComponentById(const QString &id) {
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
};

// ----------------------
// Main window tying everything together
// ----------------------
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
    fieldView->addPoint(0, 0, 10);
    fieldView->addPoint(2000, 2000, 10);
  }

private slots:
  void onSliderChanged(int value) {
    double pct = value / 1000.0;

    int totalSeconds = 100;

    int secs = int(pct * totalSeconds);
    int m = secs / 60;
    int s = secs % 60;

    timeLabel->setText(QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0')));
  }

private:
  FieldView *fieldView;

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

void add_path(){
    // TODO: impl
    // add to field
    // add info to sidebar
    // add to some list that keeps tracks of all elements
}

void add_point_cloud(){
    // TODO: impl
    // add to field
    // add info to sidebar
    // add to some list that keeps tracks of all elements
}

void load_file(){
    // TODO: impl
    // load field image?
    // load paths / point clouds
}

int main(int argc, char **argv) {
  QApplication app(argc, argv);

  MainWindow w;

  w.show();
  return app.exec();
}

#include "main.moc"
