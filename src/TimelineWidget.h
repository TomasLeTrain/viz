#pragma once

#include <QLabel>
#include <QSlider>
#include <QWidget>
#include <QHBoxLayout>

class TimelineWidget : public QWidget {
  Q_OBJECT
public:
  TimelineWidget(QWidget *parent = nullptr);

signals:
  void timeChanged(double t);

private slots:
  void onSliderChanged(int value);

private:
  QLabel *timeStart;
  QLabel *timeEnd;
  QSlider *slider;
};
