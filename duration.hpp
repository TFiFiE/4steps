#ifndef DURATION_HPP
#define DURATION_HPP

#include <QHBoxLayout>
#include <QSpinBox>
class QCheckBox;
class QSettings;

class Duration : public QHBoxLayout {
  Q_OBJECT
public:
  explicit Duration(const int minimum_=0,const QString& extraOption="",QWidget* const parent=nullptr);
  int totalSeconds() const;
  void normalize();
  QString toString() const;
  void readSetting(QSettings& settings,const QString& key);
  void writeSetting(QSettings& settings,const QString& key) const;

  int minimum;

  QSpinBox days,hours,minutes,seconds;
  QCheckBox* checkBox;
private:
  void setOption(const bool checked);
};

#endif // DURATION_HPP
