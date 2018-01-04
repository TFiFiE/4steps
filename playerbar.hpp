#ifndef PLAYERBAR_HPP
#define PLAYERBAR_HPP

#include <QHBoxLayout>
#include <QLabel>
#include <QLCDNumber>

class PlayerBar : public QWidget {
  Q_OBJECT
public:
  explicit PlayerBar();
  void setTimes(const std::array<qint64,3>& times);
  void setActive(const bool active);
  qint64 nextChange() const {return nextChange_;}
  static QString timeDisplay(const qint64 milliseconds);
private:
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
