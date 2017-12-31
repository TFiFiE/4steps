#include <QCheckBox>
#include "timecontrol.hpp"
#include "globals.hpp"

using namespace std;
TimeControl::TimeControl(const QString& title,QWidget* const parent) :
  QGroupBox(title,parent),
  formLayout(this),
  absoluteMoveTime(1,tr("unlimited")),
  maxReserve(1,tr("unlimited")),
  globalButtons{make_unique<QRadioButton>(tr("unlimited")),
                make_unique<QRadioButton>(tr("time")),
                make_unique<QRadioButton>(tr("moves"))},
  totalGameTime(1)
{
  formLayout.addRow(tr("Time given per move:"),&moveTime);
  formLayout.addRow(tr("Starting reserve:"),&startingReserve);
  formLayout.addRow(tr("Absolute move time:"),&absoluteMoveTime);
  formLayout.addRow(tr("Maximum reserve:"),&maxReserve);

  carryover.setRange(0,100);
  carryover.setValue(100);
  carryover.setSuffix(" %");
  formLayout.addRow(tr("Unused time added to reserve:"),&carryover);

  buttonGroup.setExclusive(true);
  unsigned int buttonIndex=0;
  for (const auto& globalButton:globalButtons) {
    buttonGroup.addButton(globalButton.get(),buttonIndex++);
    totalGameDuration.addWidget(globalButton.get());
  }
  stackedLayout.addWidget(&emptyWidget);
  totalGameTimeWidget.setLayout(&totalGameTime);
  stackedLayout.addWidget(&totalGameTimeWidget);
  totalGameMoves.setRange(1,std::numeric_limits<int>::max());
  totalGameMoves.setValue(80);
  stackedLayout.addWidget(&totalGameMoves);
  totalGameDuration.addLayout(&stackedLayout);

  globalButtons[0]->setChecked(true);
  connect(&buttonGroup,static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),&stackedLayout,[=](const int id){
    stackedLayout.setCurrentIndex(id);
    QSpinBox* const spinBox=std::array<QSpinBox*,3>{nullptr,&totalGameTime.seconds,&totalGameMoves}[id];
    if (spinBox!=nullptr) {
      spinBox->setFocus();
      spinBox->selectAll();
    }
  });
  formLayout.addRow(tr("Maximum game duration:"),&totalGameDuration);
}

QString TimeControl::toString(const bool timed) const
{
  if (!timed)
    return "0/0/0/0/0";
  bool truncatable=true;
  QString result;
  if (!absoluteMoveTime.checkBox->isChecked()) {
    result="/"+absoluteMoveTime.toString();
    truncatable=false;
  }

  if (buttonGroup.checkedId()>0) {
    if (buttonGroup.checkedId()==1)
      result.prepend("/"+totalGameTime.toString());
    else
      result.prepend("/"+totalGameMoves.text()+"t");
    truncatable=false;
  }
  else if (!truncatable)
    result.prepend("/0");

  if (!maxReserve.checkBox->isChecked()) {
    result.prepend("/"+maxReserve.toString());
    truncatable=false;
  }
  else if (!truncatable)
    result.prepend("/0");

  if (true) //(carryover.value()!=100) // SERVER BUG: Default carryover percentage is 0 instead of 100.
    result.prepend("/"+carryover.cleanText());
  else if (!truncatable)
    result.prepend("/100");

  return moveTime.toString()+"/"+startingReserve.toString()+result;
}

void TimeControl::readSettings(QSettings& settings)
{
  settings.beginGroup("TimeControl");
  moveTime.readSetting(settings,"move_time");
  startingReserve.readSetting(settings,"starting_reserve");
  absoluteMoveTime.readSetting(settings,"absolute_move_time");
  maxReserve.readSetting(settings,"max_reserve");
  carryover.setValue(settings.value("carry_over",carryover.value()).toUInt());

  const auto globalIndex=settings.value("total_game_duration_type").toInt();
  if (globalIndex>=0 && globalIndex<=3) {
    globalButtons[globalIndex]->click();
    if (globalIndex==1)
      totalGameTime.readSetting(settings,"total_game_duration");
    else if (globalIndex==2)
      totalGameMoves.setValue(settings.value("total_game_duration",totalGameMoves.value()).toUInt());
  }
  settings.endGroup();
}

void TimeControl::writeSettings(QSettings& settings) const
{
  settings.beginGroup("TimeControl");
  moveTime.writeSetting(settings,"move_time");
  startingReserve.writeSetting(settings,"starting_reserve");
  absoluteMoveTime.writeSetting(settings,"absolute_move_time");
  maxReserve.writeSetting(settings,"max_reserve");
  settings.setValue("carry_over",carryover.value());

  settings.setValue("total_game_duration_type",buttonGroup.checkedId());
  switch (buttonGroup.checkedId()) {
    case 1: totalGameTime.writeSetting(settings,"total_game_duration"); break;
    case 2: settings.setValue("total_game_duration",totalGameMoves.value()); break;
  }
  settings.endGroup();
}
