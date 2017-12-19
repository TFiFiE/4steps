#ifndef PLAYERBAR_HPP
#define PLAYERBAR_HPP

#include <QHBoxLayout>
#include <QLabel>
#include <QLCDNumber>

class PlayerBar : public QWidget {
public:
  explicit PlayerBar();
  void setTimes(const std::array<qint64,3>& times);
  void setActive(const bool active);
  int nextChange() const {return nextChange_;}
private:
  static QString timeDisplay(const qint64 milliseconds);
  QSize sizeHint() const override;

  qint64 nextChange_;

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
