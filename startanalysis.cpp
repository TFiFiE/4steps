#include <QFileDialog>
#include "startanalysis.hpp"
#include "globals.hpp"
#include "game.hpp"
#include "io.hpp"
#include "analysis.hpp"

StartAnalysis::StartAnalysis(Globals& globals_,const NodePtr& node_,const std::pair<Placements,ExtendedSteps>& partialMove_,Game* const game) :
  QDialog(game),
  globals(globals_),
  node(Node::reroot(node_)),
  partialMove(partialMove_),
  vBoxLayout(this),
  executableLabel(tr("Executable:")),
  executablePushButton(tr("&Locate")),
  argumentGroupBox(tr("Arguments")),
  partialMoveCheckBox(tr("Partial &move:")),
  dialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel)
{
  executable.addWidget(&executableLabel);
  executable.addWidget(&executableLineEdit);

  connect(&executablePushButton,&QPushButton::clicked,this,[this] {
    executableLineEdit.setText(QFileDialog::getOpenFileName(this,tr("Set analysis executable")));
  });
  executable.addWidget(&executablePushButton);

  vBoxLayout.addLayout(&executable);

  argumentLayout.addWidget(&arguments[0]);
  moveFileContent.setReadOnly(true);
  moveFileContent.setText(QString::fromStdString(toMoveList(node," ",false)));
  moveFileLayout.addRow(tr("Move file content:"),&moveFileContent);
  argumentLayout.addLayout(&moveFileLayout);
  argumentLayout.addWidget(&arguments[1]);

  const auto& setup=partialMove.first;
  const auto& move=partialMove.second;
  if (move.empty() ? setup.empty() : !node->legalPartialMove(move)) {
    partialMoveCheckBox.setChecked(false);
    partialMoveCheckBox.setEnabled(false);
  }
  partialMoveLayout.addWidget(&partialMoveCheckBox);
  partialMoveArguments[0].setAlignment(Qt::AlignRight);
  partialMoveArguments[1].setAlignment(Qt::AlignCenter);
  partialMoveArguments[2].setAlignment(Qt::AlignLeft);
  partialMoveArguments[1].setReadOnly(true);
  if (partialMoveCheckBox.isEnabled())
    partialMoveArguments[1].setText(QString::fromStdString(node->inSetup() ? toString(setup) : toString(move)));
  for (auto& partialMoveArgument:partialMoveArguments) {
    partialMoveLayout.addWidget(&partialMoveArgument);
    partialMoveArgument.setEnabled(partialMoveCheckBox.isChecked());
  }
  connect(&partialMoveCheckBox,&QCheckBox::toggled,this,[this](int state) {
    for (auto& partialMoveArgument:partialMoveArguments)
      partialMoveArgument.setEnabled(state);
    setWindowTitle();
  });
  argumentLayout.addLayout(&partialMoveLayout);

  argumentLayout.addWidget(&arguments[2]);

  argumentGroupBox.setLayout(&argumentLayout);
  vBoxLayout.addWidget(&argumentGroupBox);

  passSynonymLayout.addRow(tr("&Pass synonyms (space-separated):"),&passSynonyms);
  vBoxLayout.addLayout(&passSynonymLayout);

  connect(&dialogButtonBox,&QDialogButtonBox::accepted,this,[=] {
    setEnabled(false);
    Analysis::CommandLine commandLine;
    commandLine.executable=executableLineEdit.text();
    commandLine.beforeMoves=arguments[0].text();
    commandLine.moves=moveFileContent.text();

    QStringList afterArguments={arguments[1].text()};
    if (partialMoveCheckBox.isChecked()) {
      QStringList partialMoveStrings;
      for (const auto& partialMoveArgument:partialMoveArguments)
        partialMoveStrings.append(partialMoveArgument.text());
      afterArguments.append(partialMoveStrings.join(""));
    }
    afterArguments.append(arguments[2].text());
    afterArguments.removeAll("");
    commandLine.afterMoves=afterArguments.join(" ");

    std::set<std::string> words;
    for (const auto& word:passSynonyms.text().split(" ",QString::SkipEmptyParts))
      words.emplace(word.toStdString());

    const auto analysis=new Analysis(globals,node,partialMoveCheckBox.isChecked() ? partialMove : std::pair<Placements,ExtendedSteps>(),commandLine,words,this);
    connect(&analysis->process,&QProcess::errorOccurred,this,[this]{setEnabled(true);});
    connect(&analysis->process,&QProcess::readyReadStandardOutput,this,[=] {
      setEnabled(true);
      writeSettings(globals.settings);
      analysis->setParent(game,Qt::Window);
      analysis->show();
      close();
    });
    connect(analysis,&Analysis::sendPosition,game,&Game::setPosition);
  });
  connect(&dialogButtonBox,&QDialogButtonBox::rejected,this,&StartAnalysis::close);
  vBoxLayout.addWidget(&dialogButtonBox);

  readSettings(globals.settings);
  setWindowTitle();
  dialogButtonBox.button(QDialogButtonBox::Ok)->setFocus();
}

void StartAnalysis::setWindowTitle()
{
  QString position=QString::fromStdString(node->nextPlyString());
  if (partialMoveCheckBox.isChecked())
    position+=' '+partialMoveArguments[1].text();
  QDialog::setWindowTitle(tr("Analyze %1").arg(position));
}

void StartAnalysis::readSettings(QSettings& settings)
{
  settings.beginGroup("Analysis");
  executableLineEdit.setText(settings.value("executable").toString());
  arguments[0].setText(settings.value("first_arguments","analyze").toString());
  arguments[1].setText(settings.value("middle_arguments","-threads 1").toString());
  if (partialMoveCheckBox.isEnabled())
    partialMoveCheckBox.setChecked(settings.value("partial_move",true).toBool());
  partialMoveArguments[0].setText(settings.value("before_partial_move","-m \"").toString());
  partialMoveArguments[2].setText(settings.value("after_partial_move","\"").toString());
  arguments[2].setText(settings.value("last_arguments").toString());
  passSynonyms.setText(settings.value("pass_synonyms","qpss").toString());
  settings.endGroup();
}

void StartAnalysis::writeSettings(QSettings& settings) const
{
  settings.beginGroup("Analysis");
  settings.setValue("executable",executableLineEdit.text());
  settings.setValue("first_arguments",arguments[0].text());
  settings.setValue("middle_arguments",arguments[1].text());
  if (partialMoveCheckBox.isEnabled()) {
    settings.setValue("partial_move",partialMoveCheckBox.isChecked());
    if (partialMoveCheckBox.isChecked()) {
      settings.setValue("before_partial_move",partialMoveArguments[0].text());
      settings.setValue("after_partial_move",partialMoveArguments[2].text());
    }
  }
  settings.setValue("last_arguments",arguments[2].text());
  settings.setValue("pass_synonyms",passSynonyms.text());
  settings.endGroup();
}
