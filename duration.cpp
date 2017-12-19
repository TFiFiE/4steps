#include <QCheckBox>
#include "duration.hpp"

Duration::Duration(const unsigned int minimum_,const QString& extraOption,QWidget* const parent) :
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

unsigned int Duration::totalSeconds() const
{
  unsigned int result=days.value()*24+hours.value();
  result=result*60+minutes.value();
  return result*60+seconds.value();
}

void Duration::normalize()
{
  if (days.hasFocus() || hours.hasFocus() || minutes.hasFocus() || seconds.hasFocus())
    return;
  unsigned int total=std::max(minimum,totalSeconds());
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
