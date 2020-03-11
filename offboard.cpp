#include <QPainter>
#include "offboard.hpp"
#include "board.hpp"
#include "globals.hpp"

OffBoard::OffBoard(Board& board_,const Side side_) :
  QWidget(&board_),
  board(board_),
  side(side_)
{
  connect(&board,&Board::boardChanged,this,static_cast<void (OffBoard::*)()>(&OffBoard::update));
}

std::vector<PieceTypeAndSide> OffBoard::missingPieces() const
{
  std::array<unsigned int,NUM_PIECE_TYPES> missingPiecesPerType=numStartingPiecesPerType;
  for (const auto& placement:board.gameState().placements(side))
    --missingPiecesPerType[toPieceType(placement.piece)];
  std::vector<PieceTypeAndSide> result;
  for (int pieceType=NUM_PIECE_TYPES-1;pieceType>=FIRST_PIECE_TYPE;--pieceType)
    result.insert(result.end(),missingPiecesPerType[pieceType],toPieceTypeAndSide(static_cast<PieceType>(pieceType),side));
  return result;
}

void OffBoard::paintEvent(QPaintEvent*)
{
  QPainter qPainter(this);

  const int numStartingPieces=std::accumulate(std::begin(numStartingPiecesPerType),std::end(numStartingPiecesPerType),0);
  const bool tall=height()>width();
  int squareWidth,squareHeight;
  if (tall) {
    squareWidth=width();
    squareHeight=height()/numStartingPieces;
  }
  else {
    squareWidth=width()/numStartingPieces;
    squareHeight=height();
  }

  QRect qRect(0,0,squareWidth,squareHeight);
  for (const auto missingPiece:missingPieces()) {
    board.globals.pieceIcons.drawPiece(qPainter,board.iconSet,missingPiece,qRect);
    if (tall)
      qRect.translate(0,squareHeight);
    else
      qRect.translate(squareWidth,0);
  }

  qPainter.end();
}

QSize OffBoard::sizeHint() const
{
  return {board.squareWidth(),board.squareHeight()};
}
