#ifndef ANALYSIS_HPP
#define ANALYSIS_HPP

#include <sstream>
class QHBoxLayout;
class QLabel;
#include <QWidget>
#include <QProcess>
#include <QTemporaryFile>
#include <QVBoxLayout>
#include <QScrollArea>
#include "globals.hpp"
#include "gamestate.hpp"

class Analysis : public QWidget {
  Q_OBJECT
public:
  struct CommandLine {
    QString executable,moves;
    QStringList beforeMoves,afterMoves;
  };

  explicit Analysis(Globals& globals_,NodePtr node,const std::pair<Placements,ExtendedSteps>& partialMove,const CommandLine& commandLine,std::set<std::string>& passSynonyms_,QWidget* parent=nullptr);
  ~Analysis();
private:
  void setWindowTitle();
  void processStandardOutput();
  void processPlainText(std::string& text);
  virtual bool event(QEvent* event) override;
  virtual bool eventFilter(QObject* watched,QEvent* event) override;
  virtual void contextMenuEvent(QContextMenuEvent* event) override;

  Globals& globals;
  std::set<std::string> passSynonyms;
  QProcess process;
  QTemporaryFile moveFile;
  QByteArray output;
  std::stringstream ss;
  bool scrolledDown;
  enum State {
    ROW_START,
    APPENDING_TEXT,
    PROCESSING_MOVES
  } state;
  std::tuple<NodePtr,Placements,ExtendedSteps> startPosition,currentPosition;

  QVBoxLayout layout;
    QScrollArea scrollArea;
      QWidget scrollAreaChild;
        QVBoxLayout vBoxLayout;
          QHBoxLayout* lastHBoxLayout;
          QLabel* lastLabel;
signals:
  void sendPosition(const NodePtr& node,const std::pair<Placements,ExtendedSteps>& partialMove);

  friend class StartAnalysis;
};

#endif // ANALYSIS_HPP
