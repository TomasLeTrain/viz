#pragma once

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QToolButton>
#include <QVBoxLayout>

class ComponentCard : public QFrame {
  Q_OBJECT
public:
  ComponentCard(const QString &title, QWidget *infoWidget, int priority,
                QWidget *parent = nullptr);

  int getPriority() { return m_priority; }
  void setPriority(int priority) { m_priority = priority; }

private:
  int m_priority;
};
