#include <QApplication>
#include <QStyle>
#include <QScreen>
#include "globals.hpp"
#include "mainwindow.hpp"

int main(int argc,char* argv[])
{
  const QApplication a(argc,argv);

  Globals globals("4steps.ini",QSettings::IniFormat);
  MainWindow mainWindow(globals);
  mainWindow.setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,mainWindow.size(),qApp->primaryScreen()->availableGeometry()));
  mainWindow.show();

  srand(time(nullptr));
  return a.exec();
}
