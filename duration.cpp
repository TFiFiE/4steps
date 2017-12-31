#include <QCheckBox>
#include <QSettings>
#include "duration.hpp"

Duration::Duration(const int minimum_,const QString& extraOption,QWidget* const parent) :
  QHBoxLayout(parent),
  minimum(minimum_)
{
  days.setRange(0,std::numeric_limits<int>::max());
  days.setSuffix(tr(" d"));
  hours.setRange(0,std::numeric_limits<int>::max());
  hours.setSuffix(tr(" h"));
  minutes.setRange(0,std::numeric_limits<int>::max());
  minutes.setSuffix(tr(" m"));
  seconds.setRange(0,std::numeric_limits<int>::max());
  seconds.setSuffix(tr(" s"));
  if (extraOption.isEmpty())
    checkBox=nullptr;
  else {
    checkBox=new QCheckBox(extraOption);
    addWidget(checkBox);
    connect(checkBox,&QCheckBox::toggled,this,&Duration::setOption);
    checkBox->setChecked(true);
  }
  addWidget(&days);
  addWidget(&hours);
  addWidget(&minutes);
  addWidget(&seconds);
  normalize();
  connect(&   days,&QSpinBox::editingFinished,this,&Duration::normalize);
  connect(&  hours,&QSpinBox::editingFinished,this,&Duration::normalize);
  connect(&minutes,&QSpinBox::editingFinished,this,&Duration::normalize);
  connect(&seconds,&QSpinBox::editingFinished,this,&Duration::normalize);
}

int Duration::totalSeconds() const
{
  int result=days.value()*24+hours.value();
  result=result*60+minutes.value();
  return result*60+seconds.value();
}

void Duration::normalize()
{
  if (days.hasFocus() || hours.hasFocus() || minutes.hasFocus() || seconds.hasFocus())
    return;
  int total=std::max(minimum,totalSeconds());
  seconds.setValue(total%60);
  total/=60;
  minutes.setValue(total%60);
  total/=60;
  hours.setValue(total%24);
  total/=24;
  days.setValue(total);
}

void Duration::setOption(const bool checked)
{
  days.setEnabled(!checked);
  hours.setEnabled(!checked);
  minutes.setEnabled(!checked);
  seconds.setEnabled(!checked);
  if (!checked) {
    seconds.setFocus();
    seconds.selectAll();
  }
}

QString Duration::toString() const
{
  if (totalSeconds()==0)
    return "0";
  QString result;
  for (const auto unit:{&days,&hours,&minutes,&seconds})
    if (unit->value()>0)
      result.append(unit->text());
  return result.remove(' ');
}

void Duration::readSetting(QSettings& settings,const QString& key)
{
  const auto variant=settings.value(key);
  if (variant.canConvert<int>()) {
    const auto value=variant.toInt();
    const bool valid=(value>=minimum);
    if (checkBox!=nullptr)
      checkBox->setChecked(!valid);
    if (valid) {
      seconds.setValue(value);
      normalize();
    }
  }
}

void Duration::writeSetting(QSettings& settings,const QString& key) const
{
  if (checkBox!=nullptr && checkBox->isChecked())
    settings.setValue(key,minimum-1);
  else
    settings.setValue(key,totalSeconds());
}
