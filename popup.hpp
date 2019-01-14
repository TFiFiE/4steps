#ifndef POPUP_HPP
#define POPUP_HPP

#include <QWidget>
class Board;
#include "def.hpp"

class Popup : public QWidget {
  Q_OBJECT
public:
  explicit Popup(Board& board_,const SquareIndex affectedSquare_);
private:
  void paintEvent(QPaintEvent*) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

  Board& board;
  const SquareIndex affectedSquare;
  bool pieceTypeAndSideAtMax[NUM_PIECE_SIDE_COMBINATIONS];
  enum {
    ROWS_PER_SIDE=2,
    ROWS=NUM_SIDES*ROWS_PER_SIDE,
    COLUMNS=4
  };
  std::array<std::array<PieceTypeAndSide,COLUMNS>,ROWS> pieceOnSquare;
  const Side upSide;
  const bool left;
  bool dragging;
};

#endif // POPUP_HPP
