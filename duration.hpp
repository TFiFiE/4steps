#ifndef DURATION_HPP
#define DURATION_HPP

#include <QHBoxLayout>
#include <QSpinBox>
class QCheckBox;

class Duration : public QHBoxLayout {
  Q_OBJECT
public:
  explicit Duration(const unsigned int minimum_=0,const QString& extraOption="",QWidget* const parent=nullptr);
  unsigned int totalSeconds() const;
  void normalize();
  QString toString() const;

  unsigned int minimum;

  QSpinBox days,hours,minutes,seconds;
  QCheckBox* checkBox;
private:
  void setOption(const bool checked);
};

#endif // DURATION_HPP
