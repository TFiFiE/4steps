#ifndef TIMECONTROL_HPP
#define TIMECONTROL_HPP

#include <memory>
class QSettings;
#include <QGroupBox>
#include <QFormLayout>
#include <QRadioButton>
#include <QStackedLayout>
#include <QButtonGroup>
#include "duration.hpp"

class TimeControl : public QGroupBox {
  Q_OBJECT
public:
  explicit TimeControl(const QString& title,QWidget* const parent=nullptr);
  QString toString(const bool timed) const;
  void readSettings(QSettings& settings);
  void writeSettings(QSettings& settings) const;
private:
  QFormLayout formLayout;
    Duration moveTime,startingReserve,absoluteMoveTime,maxReserve;
    QSpinBox carryover;
    QHBoxLayout totalGameDuration;
      std::unique_ptr<QRadioButton> globalButtons[3];
      QStackedLayout stackedLayout;
        QWidget emptyWidget;
        QWidget totalGameTimeWidget;
          Duration totalGameTime;
        QSpinBox totalGameMoves;
  QButtonGroup buttonGroup;
  friend class CreateGame;
};

#endif // TIMECONTROL_HPP
