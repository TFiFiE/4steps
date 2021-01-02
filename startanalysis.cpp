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
  argumentGroupBox(tr("Arguments (line-separated)")),
  partialMoveGroupBox(tr("Partial &move")),
  dialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel)
{
  executable.addWidget(&executableLabel);
  executable.addWidget(&executableLineEdit);

  connect(&executablePushButton,&QPushButton::clicked,this,[this] {
    executableLineEdit.setText(QFileDialog::getOpenFileName(this,tr("Set analysis executable")));
  });
  executable.addWidget(&executablePushButton);

  vBoxLayout.addLayout(&executable);

  for (auto& textEdit:arguments) {
    textEdit.setAcceptRichText(false);
    textEdit.setLineWrapMode(QTextEdit::NoWrap);
  }

  argumentLayout.addWidget(&arguments[0]);
  moveFileContent.setReadOnly(true);
  moveFileContent.setText(QString::fromStdString(toMoveList(node," ",false)));
  moveFileLayout.addRow(tr("Move file content:"),&moveFileContent);
  argumentLayout.addLayout(&moveFileLayout);
  argumentLayout.addWidget(&arguments[1]);

  const auto& setup=partialMove.first;
  const auto& move=partialMove.second;
  const bool partialMovePossible=(move.empty() ? !setup.empty() : node->legalPartialMove(move));
  partialMoveGroupBox.setCheckable(true);
  partialMoveGroupBox.setChecked(partialMovePossible);
  partialMoveGroupBox.setEnabled(partialMovePossible);
  partialMoveLine.setReadOnly(true);
  if (partialMovePossible)
    partialMoveLine.setText(QString::fromStdString(node->inSetup() ? toString(setup) : toString(move)));
  for (auto& partialMoveArgument:partialMoveArguments) {
    partialMoveArgument.setAcceptRichText(false);
    partialMoveArgument.setLineWrapMode(QTextEdit::NoWrap);
    partialMoveLayout.addWidget(&partialMoveArgument);
    partialMoveArgument.setEnabled(partialMoveGroupBox.isChecked());
  }
  partialMoveLayout.insertWidget(1,&partialMoveLine);
  partialMoveLine.setEnabled(partialMoveGroupBox.isChecked());
  connect(&partialMoveGroupBox,&QGroupBox::toggled,this,[this](int state) {
    for (auto& partialMoveArgument:partialMoveArguments)
      partialMoveArgument.setEnabled(state);
    partialMoveLine.setEnabled(state);
    setWindowTitle();
  });
  partialMoveGroupBox.setLayout(&partialMoveLayout);
  argumentLayout.addWidget(&partialMoveGroupBox);

  argumentLayout.addWidget(&arguments[2]);

  argumentGroupBox.setLayout(&argumentLayout);
  vBoxLayout.addWidget(&argumentGroupBox);

  passSynonymLayout.addRow(tr("&Pass synonyms (space-separated):"),&passSynonyms);
  vBoxLayout.addLayout(&passSynonymLayout);

  connect(&dialogButtonBox,&QDialogButtonBox::accepted,this,[=] {
    setEnabled(false);
    Analysis::CommandLine commandLine;
    commandLine.executable=executableLineEdit.text();
    commandLine.beforeMoves=split(arguments[0]);
    commandLine.moves=moveFileContent.text();

    commandLine.afterMoves=split(arguments[1]);
    if (partialMoveGroupBox.isChecked()) {
      commandLine.afterMoves.append(split(partialMoveArguments[0]));
      commandLine.afterMoves.append(partialMoveLine.text());
      commandLine.afterMoves.append(split(partialMoveArguments[1]));
    }
    commandLine.afterMoves.append(split(arguments[2]));

    std::set<std::string> words;
    for (const auto& word:passSynonyms.text().split(" ",QString::SkipEmptyParts))
      words.emplace(word.toStdString());

    const auto analysis=new Analysis(globals,node,partialMoveGroupBox.isChecked() ? partialMove : std::pair<Placements,ExtendedSteps>(),commandLine,words,this);
    if (analysis->isHidden() && analysis->process.state()==QProcess::NotRunning)
      setEnabled(true);
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

QStringList StartAnalysis::split(const QTextEdit& textEdit)
{
  return textEdit.toPlainText().split('\n',QString::SkipEmptyParts);
}

void StartAnalysis::setWindowTitle()
{
  QString position=QString::fromStdString(node->nextPlyString());
  if (partialMoveGroupBox.isChecked())
    position+=' '+partialMoveLine.text();
  QDialog::setWindowTitle(tr("Analyze %1").arg(position));
}

void StartAnalysis::readSettings(QSettings& settings)
{
  settings.beginGroup("Analysis");
  executableLineEdit.setText(settings.value("executable").toString());
  arguments[0].setText(settings.value("first_arguments","analyze").toString());
  arguments[1].setText(settings.value("middle_arguments","-threads\n1").toString());
  if (partialMoveGroupBox.isEnabled())
    partialMoveGroupBox.setChecked(settings.value("partial_move",true).toBool());
  partialMoveArguments[0].setText(settings.value("before_partial_move","-m").toString());
  partialMoveArguments[1].setText(settings.value("after_partial_move").toString());
  arguments[2].setText(settings.value("last_arguments").toString());
  passSynonyms.setText(settings.value("pass_synonyms","qpss").toString());
  settings.endGroup();
}

void StartAnalysis::writeSettings(QSettings& settings) const
{
  settings.beginGroup("Analysis");
  settings.setValue("executable",executableLineEdit.text());
  settings.setValue("first_arguments",arguments[0].toPlainText());
  settings.setValue("middle_arguments",arguments[1].toPlainText());
  if (partialMoveGroupBox.isEnabled()) {
    settings.setValue("partial_move",partialMoveGroupBox.isChecked());
    if (partialMoveGroupBox.isChecked()) {
      settings.setValue("before_partial_move",partialMoveArguments[0].toPlainText());
      settings.setValue("after_partial_move",partialMoveArguments[1].toPlainText());
    }
  }
  settings.setValue("last_arguments",arguments[2].toPlainText());
  settings.setValue("pass_synonyms",passSynonyms.text());
  settings.endGroup();
}
