#include <QApplication>
#include <QScrollBar>
#include <QLabel>
#include <QPushButton>
#include <QStyleOptionButton>
#include <QContextMenuEvent>
#include <QMenu>
#include <QClipboard>
#include "analysis.hpp"
#include "messagebox.hpp"
#include "io.hpp"

Analysis::Analysis(Globals& globals_,NodePtr node,const std::pair<Placements,ExtendedSteps>& partialMove,const CommandLine& commandLine,std::set<std::string>& passSynonyms_,QWidget* parent) :
  QWidget(parent,Qt::Window),
  globals(globals_),
  passSynonyms(passSynonyms_),
  process(this),
  scrolledDown(true),
  state(ROW_START),
  startPosition{node,partialMove.first,partialMove.second},
  currentPosition(startPosition),
  layout(this),
  vBoxLayout(&scrollAreaChild),
  lastHBoxLayout(nullptr),
  lastLabel(nullptr)
{
  layout.addWidget(&scrollArea);
  scrollArea.setWidget(&scrollAreaChild);
  scrollArea.setWidgetResizable(true);
  vBoxLayout.addStretch();

  connect(&process,&QProcess::readyReadStandardOutput,this,&Analysis::processStandardOutput);
  connect(&process,&QProcess::errorOccurred,this,[this](const QProcess::ProcessError error) {
    if (isVisible())
      setWindowTitle();
    else
      close();
    QString message;
    switch (error) {
      case QProcess::FailedToStart: message=tr("Failed to start."); break;
      case QProcess::Crashed:       message=tr("Crashed."); break;
      case QProcess::Timedout:      message=tr("Timed out."); break;
      case QProcess::ReadError:     message=tr("Read error."); break;
      case QProcess::WriteError:    message=tr("Write error."); break;
      case QProcess::UnknownError:  message=tr("Unknown error."); break;
    };
    MessageBox(QMessageBox::Critical,tr("Error analyzing"),message,QMessageBox::NoButton,this).exec();
  });
  connect(&process,static_cast<void (QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),this,[this] {
    if (isVisible()) {
      QApplication::alert(this);
      setWindowTitle();
    }
  });

  const auto verticalScrollBar=scrollArea.verticalScrollBar();
  connect(verticalScrollBar,&QScrollBar::rangeChanged,this,[=] {
    if (scrolledDown)
      verticalScrollBar->setValue(verticalScrollBar->maximum());
  });

  moveFile.open();
  QTextStream moveFileStream(&moveFile);
  moveFileStream<<commandLine.moves;
  QStringList processStrings=commandLine.beforeMoves;
  processStrings.append(moveFile.fileName());
  processStrings.append(commandLine.afterMoves);
  process.start(commandLine.executable,processStrings);

  globals.settings.beginGroup("Analysis");
  const auto size=globals.settings.value("size").toSize();
  globals.settings.endGroup();
  if (size.isValid())
    resize(size);

  setWindowTitle();
  setAttribute(Qt::WA_DeleteOnClose);
}

Analysis::~Analysis()
{
  disconnect(&process,&QProcess::errorOccurred,this,nullptr);
  process.close();
}

void Analysis::setWindowTitle()
{
  const auto& startingNode=std::get<0>(currentPosition);
  auto position=startingNode->nextPlyString();
  if (startingNode->inSetup()) {
    const auto& startingSetup=std::get<1>(currentPosition);
    if (!startingSetup.empty())
      position+=' '+toString(startingSetup);
  }
  else {
    const auto& startingMove=std::get<2>(currentPosition);
    if (!startingMove.empty())
      position+=' '+toString(startingMove);
  }

  QString windowTitle;
  if (process.state()==QProcess::NotRunning) {
    if (process.exitStatus()==QProcess::NormalExit)
      windowTitle=tr("Finished analyzing %1").arg(QString::fromStdString(position));
    else
      windowTitle=tr("Stopped analyzing %1").arg(QString::fromStdString(position));
  }
  else
    windowTitle=tr("Analyzing %1").arg(QString::fromStdString(position));
  QWidget::setWindowTitle(windowTitle);
}

void Analysis::processStandardOutput()
{
  QApplication::alert(this);
  const auto verticalScrollBar=scrollArea.verticalScrollBar();
  scrolledDown=(verticalScrollBar->value()==verticalScrollBar->maximum());

  const auto additionalOutput=process.readAllStandardOutput();
  output+=additionalOutput;
  ss.clear();
  ss<<additionalOutput.toStdString();
  std::string unprocessed;
  while (true) {
    auto c=ss.peek();
    if (c==EOF) {
      processPlainText(unprocessed);
      break;
    }
    else if (isspace(c)) {
      ss.get();
      if (c=='\n' && state==PROCESSING_MOVES) {
        state=ROW_START;
        currentPosition=startPosition;
      }
      else
        unprocessed+=c;
    }
    else {
      auto& currentNode=std::get<0>(currentPosition);
      auto& currentSetup=std::get<1>(currentPosition);
      auto& currentMove=std::get<2>(currentPosition);
      const auto posBefore=ss.tellg();
      const auto result=parseChunk(ss,currentNode,currentSetup,currentMove,true);
      auto newNode=std::get<0>(result);
      auto chunk=std::get<1>(result);
      bool action=(newNode!=nullptr);

      if (action)
        currentNode=newNode;
      else {
        ss.clear();
        ss.seekg(posBefore);
        ss>>chunk;
        if (passSynonyms.find(chunk)!=passSynonyms.end() && !currentNode->inSetup() && currentNode->legalMove(currentMove)==MoveLegality::LEGAL) {
          currentNode=Node::makeMove(currentNode,currentMove,true);
          assert(currentSetup.empty());
          currentMove.clear();
          action=true;
        }
      }

      if (action) {
        if (state==PROCESSING_MOVES) {
          if (unprocessed!=" ")
            lastHBoxLayout->insertSpacing(lastHBoxLayout->count()-1,10);
          unprocessed.clear();
        }
        else {
          processPlainText(unprocessed);
          lastHBoxLayout=new QHBoxLayout();
          vBoxLayout.insertLayout(vBoxLayout.count()-1,lastHBoxLayout);
          assert(lastHBoxLayout->parent()!=nullptr);
          lastHBoxLayout->setSpacing(0);
          lastHBoxLayout->addStretch();
        }
        const auto pushButton=new QPushButton(QString::fromStdString(chunk));
        lastHBoxLayout->insertWidget(lastHBoxLayout->count()-1,pushButton);
        pushButton->installEventFilter(this);
        pushButton->setFocusPolicy(Qt::TabFocus);
        connect(pushButton,&QPushButton::pressed,pushButton,[pushButton]{pushButton->setFocus(Qt::MouseFocusReason);});
        connect(pushButton,&QPushButton::clicked,this,[=] {
          emit sendPosition(currentNode,{currentSetup,currentMove});
        });
        pushButton->setFont(monospace());

        const auto textSize=pushButton->fontMetrics().size(Qt::TextShowMnemonic,pushButton->text());
        QStyleOptionButton styleOptionButton;
        styleOptionButton.initFrom(pushButton);
        styleOptionButton.rect.setSize(textSize);
        pushButton->setFixedSize(pushButton->style()->sizeFromContents(QStyle::CT_PushButton,&styleOptionButton,textSize,pushButton));

        state=PROCESSING_MOVES;
      }
      else {
        unprocessed+=chunk;
        if (state==PROCESSING_MOVES) {
          std::string line;
          getline(ss,line);
          unprocessed+=line;
          processPlainText(unprocessed);
        }
      }
    }
  }
}

void Analysis::processPlainText(std::string& text)
{
  if (!text.empty()) {
    if (state==APPENDING_TEXT) {
      assert(lastLabel!=nullptr);
      lastLabel->setText(lastLabel->text()+QString::fromStdString(text));
    }
    else {
      lastLabel=new QLabel(QString::fromStdString(text));
      if (state==ROW_START) {
        vBoxLayout.insertWidget(vBoxLayout.count()-1,lastLabel);
        state=APPENDING_TEXT;
      }
      else {
        assert(state==PROCESSING_MOVES);
        assert(lastHBoxLayout!=nullptr);
        lastHBoxLayout->insertWidget(lastHBoxLayout->count()-1,lastLabel);
        state=ROW_START;
        currentPosition=startPosition;
      }
      lastLabel->installEventFilter(this);
      assert(lastLabel->parent()!=nullptr);
      lastLabel->setFont(monospace());
      lastLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }
    text.clear();
  }
}

bool Analysis::event(QEvent* event)
{
  if (event->type()==QEvent::WindowActivate) {
    globals.settings.beginGroup("Analysis");
    globals.settings.setValue("size",normalGeometry().size());
    globals.settings.endGroup();
  }
  return QWidget::event(event);
}

bool Analysis::eventFilter(QObject* watched,QEvent* event)
{
  switch (event->type()) {
    case QEvent::FocusIn:
      if (const auto& pushButton=dynamic_cast<QPushButton*>(watched))
        pushButton->click();
    break;
    case QEvent::ContextMenu: {
      const auto& label=dynamic_cast<QLabel*>(watched);
      if (label!=nullptr && !label->hasSelectedText()) {
        contextMenuEvent(static_cast<QContextMenuEvent*>(event));
        return true;
      }
    }
    break;
    default: break;
  }
  return QWidget::eventFilter(watched,event);
}

void Analysis::contextMenuEvent(QContextMenuEvent*)
{
  auto menu=new QMenu(this);
  menu->setAttribute(Qt::WA_DeleteOnClose);

  const auto copyAll=new QAction(tr("Copy all to clipboard"),menu);
  connect(copyAll,&QAction::triggered,this,[this] {
    QGuiApplication::clipboard()->setText(output);
  });
  menu->addAction(copyAll);

  const auto stop=new QAction(tr("Stop analysis"),menu);
  if (process.state()==QProcess::NotRunning)
    stop->setEnabled(false);
  else
    connect(stop,&QAction::triggered,this,[this] {
      disconnect(&process,&QProcess::errorOccurred,this,nullptr);
      process.close();
    });
  menu->addAction(stop);

  menu->popup(QCursor::pos());
}
