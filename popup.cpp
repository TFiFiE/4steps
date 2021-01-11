#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QScreen>
#include "popup.hpp"
#include "board.hpp"
#include "globals.hpp"

Popup::Popup(Board& board_,const SquareIndex affectedSquare_) :
  QWidget(&board_,Qt::Popup),
  board(board_),
  affectedSquare(affectedSquare_),
  pieceTypeAndSideAtMax{board.gameState().piecesAtMax()},
  upSide(board.southIsUp ? FIRST_SIDE : SECOND_SIDE),
  left((toFile(affectedSquare)*2<NUM_FILES)!=board.southIsUp),
  dragging(false)
{
  for (auto& row:pieceOnSquare)
    row.fill(NO_PIECE);
  for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side))
    for (PieceType pieceType=FIRST_PIECE_TYPE;pieceType<NUM_PIECE_TYPES;increment(pieceType)) {
      auto row=pieceType%ROWS_PER_SIDE;
      if (side==upSide)
        row=1-row;
      else
        row+=ROWS_PER_SIDE;
      auto column=pieceType/ROWS_PER_SIDE;
      if (left)
        column=COLUMNS-1-column;
      pieceOnSquare[row][column]=toPieceTypeAndSide(pieceType,side);
    }

  resize(board.squareWidth()*COLUMNS,board.squareHeight()*ROWS);
  auto newPos=QCursor::pos()-QPoint(0,board.squareHeight()*ROWS_PER_SIDE);
  if (left)
    newPos-=QPoint(board.squareWidth()*COLUMNS,0);

  const auto topLeftRange=qApp->primaryScreen()->availableGeometry().size()-size();
  newPos.rx()=clipped(newPos.x(),0,topLeftRange.width()-1);
  newPos.ry()=clipped(newPos.y(),0,topLeftRange.height()-1);

  move(newPos);
  setAttribute(Qt::WA_DeleteOnClose);
  show();
}

void Popup::paintEvent(QPaintEvent*)
{
  const QColor sideColors[NUM_SIDES]={Qt::white,Qt::black};
  QPainter qPainter(this);

  QPen qPen(Qt::SolidPattern,1);
  qPen.setColor(sideColors[board.sideToMove()]);
  qPainter.setPen(qPen);
  qPainter.setBrush(board.neutralColor);

  QRect qRect(0,0,board.squareWidth(),board.squareHeight());
  for (auto row=pieceOnSquare.cbegin();row!=pieceOnSquare.cend();++row) {
    const auto rowIndex=std::distance(pieceOnSquare.cbegin(),row);
    const bool showEmpty=((rowIndex<ROWS_PER_SIDE)==(upSide==board.sideToMove()));
    for (auto cell=row->cbegin();cell!=row->cend();++cell)
      if (showEmpty || *cell!=NO_PIECE) {
        qRect.moveTo(board.squareWidth()*std::distance(row->cbegin(),cell),board.squareHeight()*rowIndex);
        qPainter.drawRect(qRect);
        if (*cell!=NO_PIECE) {
          if (pieceTypeAndSideAtMax[*cell])
            qPainter.setOpacity(.25);
          board.globals.pieceIcons.drawPiece(qPainter,board.iconSet,*cell,qRect);
          qPainter.setOpacity(1);
        }
      }
  }

  const Side side=otherSide(board.sideToMove());
  qRect.setHeight(board.squareHeight()*ROWS_PER_SIDE);
  qRect.moveTo(left ? 0 : board.squareWidth()*static_cast<int>(pieceOnSquare.cbegin()->size()-1),side==upSide ? 0 : board.squareHeight()*ROWS_PER_SIDE);
  qPainter.setBrush(sideColors[side]);
  qPainter.drawRect(qRect);

  qPainter.end();
}

void Popup::mouseMoveEvent(QMouseEvent* event)
{
  if ((event->buttons()&Qt::RightButton)!=0)
    dragging=true;
}

void Popup::mouseReleaseEvent(QMouseEvent* event)
{
  if (dragging || event->button()!=Qt::RightButton) {
    const auto pos=event->pos();
    const auto row=floorDiv(pos.y(),board.squareHeight());
    const auto column=floorDiv(pos.x(),board.squareWidth());
    if (row>=0 && row<ROWS && column>=0 && column<COLUMNS) {
      const auto piece=pieceOnSquare[row][column];
      if (piece==NO_PIECE) {
        if ((row<ROWS_PER_SIDE)==(upSide==board.sideToMove()))
          board.potentialSetup.squarePieces[affectedSquare]=NO_PIECE;
        else
          board.potentialSetup.sideToMove=otherSide(board.potentialSetup.sideToMove);
        emit board.boardChanged();
      }
      else if (pieceTypeAndSideAtMax[piece])
        return;
      else {
        board.potentialSetup.squarePieces[affectedSquare]=piece;
        emit board.boardChanged();
      }
    }
    close();
  }
}
