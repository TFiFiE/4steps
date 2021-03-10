#ifndef PALETTE_HPP
#define PALETTE_HPP

#include <array>
#include <QWidget>
class Board;
#include "def.hpp"

class Palette : public QWidget {
  Q_OBJECT
public:
  explicit Palette(Board& board_,const SquareIndex affectedSquare_);
private:
  virtual void paintEvent(QPaintEvent*) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;

  Board& board;
  const SquareIndex affectedSquare;
  const std::array<bool,NUM_PIECE_SIDE_COMBINATIONS> pieceTypeAndSideAtMax;
  static constexpr auto ROWS_PER_SIDE=2;
  static constexpr auto ROWS=NUM_SIDES*ROWS_PER_SIDE;
  static constexpr auto COLUMNS=4;
  std::array<std::array<PieceTypeAndSide,COLUMNS>,ROWS> pieceOnSquare;
  const Side upSide;
  const bool left;
  bool dragging;
};

#endif // PALETTE_HPP
