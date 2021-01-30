#include <QApplication>
#include <QStyle>
#include <QScreen>
#include "globals.hpp"
#include "mainwindow.hpp"
#include "iconengine.hpp"

int main(int argc,char* argv[])
{
  const QApplication a(argc,argv);

  std::random_device rd;
  Globals globals(rd()^time(nullptr),"4steps.ini",QSettings::IniFormat);
  MainWindow mainWindow(globals);
  mainWindow.setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,mainWindow.size(),qApp->primaryScreen()->availableGeometry()));
  QApplication::setWindowIcon(QIcon(new IconEngine));
  mainWindow.show();

  return a.exec();
}
