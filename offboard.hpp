#ifndef OFFBOARD_HPP
#define OFFBOARD_HPP

#include <QWidget>
class Board;
#include "def.hpp"

class OffBoard : public QWidget {
  Q_OBJECT
public:
  OffBoard(Board& board_,const Side side_);
private:
  std::vector<PieceTypeAndSide> missingPieces() const;
  virtual void paintEvent(QPaintEvent*) override;
  virtual QSize sizeHint() const override;

  const Board& board;
  const Side side;
};

#endif // OFFBOARD_HPP
