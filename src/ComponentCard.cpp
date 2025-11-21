#include "ComponentCard.h"
#include "moc_ComponentCard.cpp"

ComponentCard::ComponentCard(const QString &title, QWidget *infoWidget,
                             int priority, QWidget *parent)
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
