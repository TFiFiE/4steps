#ifndef PLAYERBAR_HPP
#define PLAYERBAR_HPP

#include <QHBoxLayout>
#include <QLabel>
#include <QLCDNumber>
#include "readonly.hpp"

class PlayerBar : public QWidget {
  Q_OBJECT
public:
  explicit PlayerBar();
  void setTimes(const std::array<qint64,3>& times);
  void setActive(const bool active);
  static QString timeDisplay(const qint64 milliseconds);

  readonly<PlayerBar,qint64> nextChange;
private:
  QHBoxLayout hBoxLayout;
public:
  QLabel player;
private:
  QFrame separator;
  QLabel used;
  QLCDNumber timeLeft;
  QLabel potentialReserve;
};

#endif // PLAYERBAR_HPP
