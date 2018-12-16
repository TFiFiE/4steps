#ifndef PIECEICONS_HPP
#define PIECEICONS_HPP

#include <QSvgRenderer>
#include <QImage>
#include "def.hpp"

class PieceIcons {
public:
  enum Set {
    VECTOR,
    RASTER
  };

  PieceIcons();
  void drawPiece(QPainter& painter,const Set set,const PieceTypeAndSide piece,const QRect rectangle);
private:
  QSvgRenderer vector[NUM_PIECE_SIDE_COMBINATIONS];
  QImage raster[NUM_PIECE_SIDE_COMBINATIONS];
};

#endif // PIECEICONS_HPP
