#include "playerbar.hpp"

PlayerBar::PlayerBar() :
  hBoxLayout(this),
  timeLeft(1)
{
  timeLeft.display("-");
  hBoxLayout.addWidget(&player,Qt::AlignLeft);
  separator.setFrameShape(QFrame::VLine);
  hBoxLayout.addWidget(&separator);
  hBoxLayout.addWidget(&used,Qt::AlignRight);
  hBoxLayout.addWidget(&timeLeft,Qt::AlignCenter);
  hBoxLayout.addWidget(&potentialReserve,Qt::AlignLeft);
  timeLeft.setSegmentStyle(QLCDNumber::Outline);
}

void PlayerBar::setTimes(const std::array<qint64,3>& times)
{
  const auto timeLeft_=timeDisplay(std::get<0>(times));
  timeLeft.setDigitCount(timeLeft_.size());
  timeLeft.display(timeLeft_);
  potentialReserve.setText(timeDisplay(std::get<1>(times)));
  used.setText(timeDisplay(std::get<2>(times)));

  nextChange_=1000-std::get<2>(times)%1000;
  if (std::get<0>(times)>=1000)
    nextChange_=std::min(nextChange_,1+std::get<0>(times)%1000);
  if (std::get<1>(times)>=1000)
    nextChange_=std::min(nextChange_,1+std::get<1>(times)%1000);
}

void PlayerBar::setActive(const bool active)
{
  timeLeft.setSegmentStyle(active ? QLCDNumber::Flat : QLCDNumber::Outline);
}

QString PlayerBar::timeDisplay(const qint64 milliseconds)
{
  const auto seconds=std::max(qint64(0),milliseconds)/1000;
  if (seconds<60)
    return QString::number(seconds);
  QString result=':'+QString::number(seconds%60).rightJustified(2,'0');

  const auto minutes=seconds/60;
  if (minutes<60)
    return QString::number(minutes)+result;
  result.prepend(':'+QString::number(minutes%60).rightJustified(2,'0'));

  const auto hours=minutes/60;
  result.prepend(QString::number(hours%24));
  if (hours<24)
    return result;

  return QString::number(hours/24)+"d "+result;
}

QSize PlayerBar::sizeHint() const
{
  return QWidget::sizeHint()+QSize(0,20);
}
