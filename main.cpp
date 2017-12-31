#include <QApplication>
#include <QStyle>
#include <QDesktopWidget>
#include "globals.hpp"
#include "mainwindow.hpp"

static void loadPieceIcons(QSvgRenderer pieceIcons[NUM_PIECE_SIDE_COMBINATIONS])
{
  const QString sideFileString[NUM_SIDES]={"white","black"};
  const QString pieceFileString[NUM_PIECE_TYPES]={"Rabbit","Cat","Dog","Horse","Camel","Elephant"};
  for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side))
    for (PieceType pieceType=FIRST_PIECE_TYPE;pieceType<NUM_PIECE_TYPES;increment(pieceType)) {
      QSvgRenderer& qSvgRenderer=pieceIcons[toPieceTypeAndSide(pieceType,side)];
      qSvgRenderer.load(":/piece icons/"+sideFileString[side]+'-'+pieceFileString[pieceType]+".svg");
      const QRectF viewBoxF=qSvgRenderer.viewBoxF();
      const qreal width =viewBoxF.width();
      const qreal height=viewBoxF.height();
      if (width>height) {
        const qreal margin=(width-height)/2;
        qSvgRenderer.setViewBox(viewBoxF+QMarginsF(0,margin,0,margin));
      }
      else if (width<height) {
        const qreal margin=(height-width)/2;
        qSvgRenderer.setViewBox(viewBoxF+QMarginsF(margin,0,margin,0));
      }
    }
}

int main(int argc,char* argv[])
{
  const QApplication a(argc,argv);

  Globals globals("4steps.ini",QSettings::IniFormat);
  loadPieceIcons(globals.pieceIcons);
  MainWindow mainWindow(globals);
  mainWindow.setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,mainWindow.size(),qApp->desktop()->availableGeometry()));
  mainWindow.show();

  srand(time(nullptr));
  return a.exec();
}
