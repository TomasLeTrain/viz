#include "TimelineWidget.h"

TimelineWidget::TimelineWidget(QWidget *parent) : QWidget(parent) {
  auto *lay = new QHBoxLayout(this);

  timeStart = new QLabel("0:00");
  slider = new QSlider(Qt::Horizontal);
  timeEnd = new QLabel("0:00");

  slider->setRange(0, 1000);

  lay->addWidget(timeStart);
  lay->addWidget(slider);
  lay->addWidget(timeEnd);

  connect(slider, &QSlider::valueChanged, this,
          &TimelineWidget::onSliderChanged);
}

void TimelineWidget::onSliderChanged(int value) {
  double pct = value / 1000.0;
  emit timeChanged(pct);

  // TODO: change
  int totalSeconds = 100;

  int secs = int(totalSeconds * pct);
  int m = secs / 60;
  int s = secs % 60;

  int total_secs = int(totalSeconds);
  int total_m = total_secs / 60;
  int total_s = total_secs % 60;
  timeStart->setText(QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0')));
  timeEnd->setText(
      QString("%1:%2").arg(total_m).arg(total_s, 2, 10, QChar('0')));
}
