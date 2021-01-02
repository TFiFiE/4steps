#ifndef STARTANALYSIS_HPP
#define STARTANALYSIS_HPP

class QSettings;
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QTextEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
struct Globals;
class Game;
#include "gamestate.hpp"

class StartAnalysis : public QDialog {
  Q_OBJECT
public:
  StartAnalysis(Globals& globals_,const NodePtr& node_,const std::pair<Placements,ExtendedSteps>& partialMove_,Game* const game=nullptr);
private:
  static QStringList split(const QTextEdit& textEdit);
  void setWindowTitle();
  void readSettings(QSettings& settings);
  void writeSettings(QSettings& settings) const;

  Globals& globals;
  const NodePtr node;
  const std::pair<Placements,ExtendedSteps> partialMove;

  QVBoxLayout vBoxLayout;
    QHBoxLayout executable;
      QLabel executableLabel;
      QLineEdit executableLineEdit;
      QPushButton executablePushButton;
    QGroupBox argumentGroupBox;
      QVBoxLayout argumentLayout;
        std::array<QTextEdit,3> arguments;
        QFormLayout moveFileLayout;
          QLineEdit moveFileContent;
        QGroupBox partialMoveGroupBox;
          QVBoxLayout partialMoveLayout;
            std::array<QTextEdit,2> partialMoveArguments;
            QLineEdit partialMoveLine;
    QFormLayout passSynonymLayout;
      QLineEdit passSynonyms;
    QDialogButtonBox dialogButtonBox;
};

#endif // STARTANALYSIS_HPP
