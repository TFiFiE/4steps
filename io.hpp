#ifndef IO_HPP
#define IO_HPP

#include <sstream>
#include <QCoreApplication>
#include "node.hpp"

const char pieceLetters[]="RrCcDdHhMmEe ";

inline QString sideWord(const Side side)
{
  return side==FIRST_SIDE ? QCoreApplication::translate("","Gold") :
                            QCoreApplication::translate("","Silver");
}

inline QString pieceName(const PieceTypeAndSide piece)
{
  const QString array[]={
    QCoreApplication::translate("",  "Gold rabbit"),
    QCoreApplication::translate("","Silver rabbit"),
    QCoreApplication::translate("",  "Gold cat"),
    QCoreApplication::translate("","Silver cat"),
    QCoreApplication::translate("",  "Gold dog"),
    QCoreApplication::translate("","Silver dog"),
    QCoreApplication::translate("",  "Gold horse"),
    QCoreApplication::translate("","Silver horse"),
    QCoreApplication::translate("",  "Gold camel"),
    QCoreApplication::translate("","Silver camel"),
    QCoreApplication::translate("",  "Gold elephant"),
    QCoreApplication::translate("","Silver elephant"),
    QCoreApplication::translate("","No piece"),
  };
  return array[piece];
}

inline PieceTypeAndSide toPieceTypeAndSide(const char pieceLetter,const bool strict=true)
{
  const auto pointer=strchr(pieceLetters,pieceLetter);
  if (pointer==nullptr) {
    if (strict)
      throw std::runtime_error("Unrecognized piece letter: "+std::string(1,pieceLetter));
    else
      return NO_PIECE;
  }
  return static_cast<PieceTypeAndSide>(pointer-pieceLetters);
}

inline std::array<char,3> toCoordinates(const unsigned int file,const unsigned int rank,const char firstFileChar='a')
{
  return {static_cast<char>(firstFileChar+file),static_cast<char>('1'+rank),'\0'};
}

inline std::array<char,3> toCoordinates(const SquareIndex square,const char firstFileChar='a')
{
  assert(square!=NO_SQUARE);
  return toCoordinates(toFile(square),toRank(square),firstFileChar);
}

inline SquareIndex toSquare(const char fileLetter,const char rankLetter,const bool strict=true)
{
  const auto result=toSquare(static_cast<unsigned int>(fileLetter-'a'),
                             static_cast<unsigned int>(rankLetter-'1'));
  if (result==NO_SQUARE && strict)
    throw std::runtime_error(std::string("Invalid square: ")+fileLetter+rankLetter);
  return result;
}

inline char directionLetter(const SquareIndex origin,const SquareIndex destination)
{
  switch (destination-origin) {
    case -NUM_FILES: return 's';
    case -1        : return 'w';
    case  1        : return 'e';
    case  NUM_FILES: return 'n';
    default: throw std::runtime_error("Squares not orthogonally adjacent.");
  }
}

inline SquareIndex toDestination(const SquareIndex origin,const char directionLetter,const bool strict=true)
{
  const char directionLetters[]="swen";
  const auto pointer=strchr(directionLetters,directionLetter);
  if (pointer==nullptr) {
    if (strict)
      throw std::runtime_error("Unrecognized destination letter: "+std::string(1,directionLetter));
    else
      return NO_SQUARE;
  }
  const auto destination=toDestination(origin,static_cast<Direction>(pointer-directionLetters));
  if (destination==NO_SQUARE && strict)
    throw std::runtime_error(std::string("Direction is off the edge of the board."));
  return destination;
}

inline std::string toString(const PieceTypeAndSide pieceTypeAndSide,const SquareIndex square)
{
  return pieceLetters[pieceTypeAndSide]+std::string(toCoordinates(square).data());
}

inline std::string toString(const Placement& placement)
{
  return toString(placement.piece,placement.location);
}

inline char toLetter(const Side side,const bool newStyle=false)
{
  switch (side) {
    case  FIRST_SIDE: return newStyle ? 'g' : 'w';
    case SECOND_SIDE: return newStyle ? 's' : 'b';
    default: throw std::runtime_error("Not a valid side.");
  }
}

inline Side toSide(const char input)
{
  if (strchr("wg",input)!=nullptr)
    return FIRST_SIDE;
  else if (strchr("bs",input)!=nullptr)
    return SECOND_SIDE;
  else
    return NO_SIDE;
}

inline std::string toPlyString(const int moveIndex)
{
  assert(moveIndex>=0);
  std::stringstream ss;
  ss<<(moveIndex/NUM_SIDES)+1<<"gs"[moveIndex%NUM_SIDES];
  return ss.str();
}

inline std::string toPlyString(const int moveIndex,const Node& root)
{
  return toPlyString(moveIndex+(root.isGameStart() ? 0 : NUM_SIDES+root.currentState.sideToMove));
}

inline std::pair<Side,std::string> toMoveStart(std::string input)
{
  if (!input.empty()) {
    const Side side=toSide(input.back());
    if (side!=NO_SIDE) {
      input.pop_back();
      if (all_of(input.begin(),input.end(),isdigit))
        return {side,input};
    }
  }
  return {NO_SIDE,""};
}

inline int toMoveIndex(const std::string& input)
{
  const auto pair=toMoveStart(input);
  if (pair.first==NO_SIDE || pair.second.empty())
    return -1;
  else
    return (stoi(pair.second)-1)*NUM_SIDES+(pair.first==FIRST_SIDE ? 0 : 1);
}

inline Placement toPlacement(const std::string& word,const bool strict=true)
{
  if (word.size()!=3) {
    if (strict)
      throw std::runtime_error("Placement word does not have 3 letters.");
    else
      return Placement::invalid();
  }
  const PieceTypeAndSide piece=toPieceTypeAndSide(word[0],strict);
  const SquareIndex square=toSquare(word[1],word[2],strict);
  return {square,piece};
}

inline Placements toPlacements(const std::string& input)
{
  Placements result;
  std::stringstream ss;
  ss<<input;
  std::string word;
  while (ss>>word)
    result.emplace(toPlacement(word));
  return result;
}

inline std::string toString(const ExtendedStep& step)
{
  const SquareIndex origin=std::get<ORIGIN>(step);
  const SquareIndex destination=std::get<DESTINATION>(step);
  const PieceTypeAndSide trappedPiece=std::get<TRAPPED_PIECE>(step);
  PieceTypeAndSide movedPiece=std::get<RESULTING_STATE>(step).currentPieces[destination];
  if (movedPiece==NO_PIECE) {
    assert(isTrap(destination));
    assert(trappedPiece!=NO_PIECE);
    movedPiece=trappedPiece;
  }
  std::string result=toString(movedPiece,origin)+directionLetter(origin,destination);
  if (trappedPiece==NO_PIECE)
    return result;
  else
    return result+' '+toString(trappedPiece,adjacentTrap(origin))+'x';
}

template<class Iterator>
inline std::string toString(const Iterator begin,const Iterator end)
{
  std::string result;
  for (auto element=begin;element!=end;++element) {
    if (element!=begin)
      result+=' ';
    result+=toString(*element);
  }
  return result;
}

template<class Container>
inline std::string toString(const Container& sequence)
{
  return toString(begin(sequence),end(sequence));
}

inline std::pair<Placement,SquareIndex> toDisplacement(const std::string& word,const bool strict=true)
{
  if (word.size()==4) {
    const Placement placement=toPlacement(word.substr(0,3),strict);
    if (placement.isValid()) {
      if (word[3]=='x')
        return {placement,NO_SQUARE};
      else {
        const SquareIndex destination=toDestination(placement.location,word[3],strict);
        if (destination!=NO_SQUARE)
          return {placement,destination};
      }
    }
  }
  else if (strict)
    throw std::runtime_error("Step word does not have 4 letters.");
  return {Placement::invalid(),NO_SQUARE};
}

inline PieceSteps toMove(const std::string& input)
{
  PieceSteps result;
  std::stringstream ss;
  ss<<input;
  std::string word;
  while (ss>>word) {
    const auto displacement=toDisplacement(word);
    const PieceTypeAndSide piece=displacement.first.piece;
    const SquareIndex origin=displacement.first.location;
    const SquareIndex destination=displacement.second;
    if (destination==NO_SQUARE) {
      runtime_assert(!result.empty(),"Capture is first word of move.");
      runtime_assert(adjacentTrap(std::get<ORIGIN>(result.back()))==origin,"Capturing step is not adjacent to trap.");
      std::get<TRAPPED_PIECE>(result.back())=piece;
    }
    else
      result.emplace_back(origin,destination,NO_PIECE,piece);
  }
  return result;
}

struct runtime_error : public std::runtime_error {
  runtime_error(const QString& what_arg) : std::runtime_error(what_arg.toStdString()) {}
};

inline std::tuple<GameTree,size_t,bool> toTree(const std::string& input,NodePtr node,const bool replaceNode=false)
{
  assert(node!=nullptr);
  GameTree gameTree;
  unsigned int nodeChanges=0;

  Placements setup;
  ExtendedSteps move;
  if (replaceNode) {
    const auto& previousNode=node->previousNode;
    assert(previousNode!=nullptr);
    if (previousNode->inSetup())
      setup=node->currentState.playedPlacements();
    else
      move=node->move;
    node=previousNode;
  }

  std::stringstream ss;
  ss<<input;
  std::string word;
  for (auto posBefore=ss.tellg();ss>>word;posBefore=ss.tellg()) {
    bool endMove=true;
    if (word=="takeback") {
      gameTree.push_front(node);
      node=node->previousNode;
      ++nodeChanges;
    }
    else if (node->inSetup()) {
      const auto placement=toPlacement(word,false);
      if (placement.isValid()) {
        if (node->currentState.sideToMove==toSide(placement.piece)) {
          if (isSetupSquare(node->currentState.sideToMove,placement.location)) {
            if (find_if(setup.cbegin(),setup.cend(),[&placement](const Placement& existing) {
                  return placement.location==existing.location;
                })==setup.cend()) {
              const auto pieceType=toPieceType(placement.piece);
              const auto pieceTypeNumber=count_if(setup.cbegin(),setup.cend(),[&placement](const Placement& existing) {
                return placement.piece==existing.piece;
              });
              if (pieceTypeNumber<int(numStartingPiecesPerType[pieceType])) {
                setup.emplace(placement);
                continue;
              }
              else
                throw runtime_error(QCoreApplication::translate("","Out of piece type: ")+pieceName(placement.piece));
            }
            else
              throw runtime_error(QCoreApplication::translate("","Duplicate setup square: ")+toCoordinates(placement.location).data());
          }
          else
            throw runtime_error(QCoreApplication::translate("","Illegal setup square: ")+toCoordinates(placement.location).data());
        }
        else
          ss.seekg(posBefore);
      }
      else {
        const Side side=toMoveStart(word).first;
        if (side!=NO_SIDE) {
          if (node->currentState.sideToMove==side)
            endMove=false;
        }
        else if (word=="pass" || toDisplacement(word,false).first.isValid())
          ss.seekg(posBefore);
        else
          throw runtime_error(QCoreApplication::translate("","Invalid word: ")+QString::fromStdString(word));
      }
      if (endMove) {
        if (setup.size()<numStartingPieces)
          throw runtime_error(QCoreApplication::translate("","Illegal end of setup: ")+QString::fromStdString(word));
        else {
          node=Node::addSetup(node,setup,false);
          ++nodeChanges;
        }
      }
    }
    else {
      const GameState& gameState=move.empty() ? node->currentState : std::get<RESULTING_STATE>(move.back());
      const Side side=toMoveStart(word).first;
      if (node->result.endCondition!=NO_END && node->currentState.sideToMove!=side)
        throw runtime_error(QCoreApplication::translate("","Play in finished position: ")+QString::fromStdString(word));
      else if (side!=NO_SIDE) {
        if (node->currentState.sideToMove==side)
          endMove=false;
      }
      else if (gameState.stepsAvailable>0) {
        if (word!="pass") {
          const auto displacement=toDisplacement(word,false);
          if (displacement.first.isValid()) {
            const SquareIndex destination=displacement.second;
            if (destination==NO_SQUARE)
              throw runtime_error(QCoreApplication::translate("","Capture without step: ")+QString::fromStdString(word));
            else {
              std::string captureWord;
              posBefore=ss.tellg();
              if (ss>>captureWord) {
                const auto capture=toDisplacement(captureWord,false);
                if (capture.first.isValid() && capture.second==NO_SQUARE)
                  word+=' '+captureWord;
                else
                  ss.seekg(posBefore);
              }
              const SquareIndex origin=displacement.first.location;
              if (gameState.legalStep(origin,destination)) {
                const auto step=GameState(gameState).takeExtendedStep(origin,destination);
                if (toString(step)==word) {
                  move.emplace_back(step);
                  continue;
                }
              }
              throw runtime_error(QCoreApplication::translate("","Invalid step: ")+QString::fromStdString(word));
            }
          }
          else
            throw runtime_error(QCoreApplication::translate("","Invalid word: ")+QString::fromStdString(word));
        }
      }
      else
        ss.seekg(posBefore);
      if (endMove) {
        switch (node->legalMove(gameState)) {
          case MoveLegality::LEGAL:
            node=Node::makeMove(node,move,false);
            ++nodeChanges;
          break;
          case MoveLegality::ILLEGAL_PUSH_INCOMPLETION:
            throw runtime_error(QCoreApplication::translate("","Incomplete push: ")+QString::fromStdString(toString(move)));
          case MoveLegality::ILLEGAL_PASS:
            throw runtime_error(QCoreApplication::translate("","Illegal pass: ")+QString::fromStdString(toString(move)));
          case MoveLegality::ILLEGAL_REPETITION:
            throw runtime_error(QCoreApplication::translate("","Illegal repetition: ")+QString::fromStdString(toString(move)));
        }
      }
    }
    setup.clear();
    move.clear();
  }

  bool completeLastMove;
  if (!setup.empty()) {
    node=Node::addSetup(node,setup,false);
    ++nodeChanges;
    completeLastMove=false;
  }
  else if (!move.empty()) {
    node=Node::makeMove(node,move,false);
    ++nodeChanges;
    completeLastMove=false;
  }
  else
    completeLastMove=true;

  gameTree.push_front(node);

  return make_tuple(gameTree,nodeChanges,completeLastMove);
}

inline GameState customizedGameState(const std::string& input,GameState gameState=GameState())
{
  std::stringstream ss;
  ss<<input;
  std::string word;
  while (ss>>word) {
    const Side side=toMoveStart(word).first;
    if (side!=NO_SIDE)
      gameState.sideToMove=side;
    else {
      const auto placement=toPlacement(word,false);
      if (placement.isValid()) {
        if (gameState.currentPieces[placement.location]!=placement.piece) {
          if (gameState.piecesAtMax()[placement.piece])
            throw runtime_error(QCoreApplication::translate("","Side out of pieces of type: ")+pieceName(placement.piece));
          else
            gameState.currentPieces[placement.location]=placement.piece;
        }
      }
      else {
        const auto displacement=toDisplacement(word,false);
        const auto& placement=displacement.first;
        if (placement.isValid()) {
          if (gameState.currentPieces[placement.location]==placement.piece) {
            gameState.currentPieces[placement.location]=NO_PIECE;
            if (displacement.second!=NO_SQUARE)
              gameState.currentPieces[displacement.second]=placement.piece;
          }
          else
            throw runtime_error(QCoreApplication::translate("","Piece type not on %1: ").arg(toCoordinates(placement.location).data())+pieceName(placement.piece));
        }
        else
          throw runtime_error(QCoreApplication::translate("","Invalid word: ")+QString::fromStdString(word));
      }
    }
  }
  return gameState;
}

inline EndCondition toEndCondition(const char letter)
{
  switch (letter) {
    case 'g': return GOAL;
    case 'm': return IMMOBILIZATION;
    case 'e': return ELIMINATION;
    case 't': return TIME_OUT;
    case 'r': return RESIGNATION;
    case 'i': return ILLEGAL_MOVE;
    case 'f': return FORFEIT;
    case 'a': return ABANDONMENT;
    case 's': return SCORE;
    case 'p': return REPETITION;
    case 'n': return MUTUAL_ELIMINATION;
    default : throw std::runtime_error("Unrecognized termination code: "+std::string(1,letter));
  }
}

inline char toChar(const EndCondition endCondition)
{
  switch (endCondition) {
    case GOAL:               return 'g';
    case IMMOBILIZATION:     return 'm';
    case ELIMINATION:        return 'e';
    case TIME_OUT:           return 't';
    case RESIGNATION:        return 'r';
    case ILLEGAL_MOVE:       return 'i';
    case FORFEIT:            return 'f';
    case ABANDONMENT:        return 'a';
    case SCORE:              return 's';
    case REPETITION:         return 'p';
    case MUTUAL_ELIMINATION: return 'n';
    default: throw std::runtime_error("Not an end condition.");
  }
}

#endif // IO_HPP
