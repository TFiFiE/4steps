#include <QPainter>
#include "pieceicons.hpp"

PieceIcons::PieceIcons()
{
  const QString oldSideFileString[NUM_SIDES]={"white","black"};
  const QString newSideFileString[NUM_SIDES]={"Gold","Silver"};
  const QString pieceFileString[NUM_PIECE_TYPES]={"Rabbit","Cat","Dog","Horse","Camel","Elephant"};
  const QString directoryString=":/piece_icons";
  for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side))
    for (PieceType pieceType=FIRST_PIECE_TYPE;pieceType<NUM_PIECE_TYPES;increment(pieceType)) {
      const PieceTypeAndSide piece=toPieceTypeAndSide(pieceType,side);

      QSvgRenderer& qSvgRenderer=vector[piece];
      qSvgRenderer.load(directoryString+"/vector/"+oldSideFileString[side]+'-'+pieceFileString[pieceType]+".svg");
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

      raster[piece].load(directoryString+"/raster/"+newSideFileString[side]+pieceFileString[pieceType]+".png");
    }
}

void PieceIcons::drawPiece(QPainter& painter,const Set set,const PieceTypeAndSide piece,const QRect rectangle)
{
  switch (set) {
    case VECTOR:
      vector[piece].render(&painter,rectangle);
    break;
    case RASTER:
      painter.drawImage(rectangle,raster[piece]);
    break;
  }
}
