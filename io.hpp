#ifndef IO_HPP
#define IO_HPP

#include <sstream>
#include <QCoreApplication>
#include "node.hpp"

const char pieceLetters[]="RrCcDdHhMmEe ";
const char directionLetters[]="swen";

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
  return directionLetters[toDirection(origin,destination)];
}

inline SquareIndex toDestination(const SquareIndex origin,const char directionLetter,const bool strict=true)
{
  const auto pointer=strchr(directionLetters,directionLetter);
  if (pointer==nullptr) {
    if (strict)
      throw std::runtime_error("Unrecognized destination letter: "+std::string(1,directionLetter));
    else
      return NO_SQUARE;
  }
  return toDestination(origin,static_cast<Direction>(pointer-directionLetters),strict);
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

inline std::string toPlyString(const int moveIndex,const bool newStyle=true)
{
  assert(moveIndex>=0);
  std::stringstream ss;
  ss<<(moveIndex/NUM_SIDES)+1<<toLetter(static_cast<Side>(moveIndex%NUM_SIDES),newStyle);
  return ss.str();
}

inline std::string toPlyString(const int moveIndex,const Node& root,const bool newStyle=true)
{
  return toPlyString(moveIndex+(root.isGameStart() ? 0 : NUM_SIDES+root.gameState.sideToMove),newStyle);
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
  PieceTypeAndSide movedPiece=std::get<RESULTING_STATE>(step).squarePieces[destination];
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

inline std::string toMoveList(const NodePtr& node,const std::string& separator,const bool newStyle)
{
  std::vector<std::string> moves;
  const auto ancestors=Node::selfAndAncestors(node);
  auto ancestor=ancestors.rbegin();
  const auto root=ancestor->lock();
  assert(root->previousNode==nullptr);
  unsigned int moveIndex=0;
  if (!root->isGameStart()) {
    for (Side side=FIRST_SIDE;side<NUM_SIDES;increment(side),++moveIndex) {
      const auto placements=root->gameState.placements(side);
      assert(!placements.empty());
      moves.emplace_back(toPlyString(moveIndex,newStyle)+' '+toString(placements));
    }
    if (root->gameState.sideToMove==SECOND_SIDE) {
      moves.emplace_back(toPlyString(moveIndex,newStyle)+" pass");
      ++moveIndex;
    }
  }
  for (++ancestor;ancestor!=ancestors.rend();++ancestor,++moveIndex)
    moves.emplace_back(toPlyString(moveIndex,newStyle)+' '+ancestor->lock()->toString());
  return join(moves,separator);
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
  runtime_error(const QString& what_arg="") : std::runtime_error(what_arg.toStdString()) {}
};

inline std::tuple<Subnode,std::string,runtime_error> parseChunk(std::stringstream& ss,Subnode subnode,const bool after)
{
  auto posBefore=ss.tellg();
  std::string chunk;
  if (ss>>chunk) {
    NodePtr& node=std::get<0>(subnode);
    Placements& setup=std::get<1>(subnode);
    ExtendedSteps& move=std::get<2>(subnode);
    bool endMove=true;
    if (chunk=="takeback")
      node=node->previousNode;
    else if (node->inSetup()) {
      const auto placement=toPlacement(chunk,false);
      if (placement.isValid()) {
        if (node->gameState.sideToMove==toSide(placement.piece)) {
          if (isSetupSquare(node->gameState.sideToMove,placement.location)) {
            if (find_if(setup.cbegin(),setup.cend(),[&placement](const Placement& existing) {
                  return placement.location==existing.location;
                })==setup.cend()) {
              const auto pieceType=toPieceType(placement.piece);
              const auto pieceTypeNumber=count_if(setup.cbegin(),setup.cend(),[&placement](const Placement& existing) {
                return placement.piece==existing.piece;
              });
              if (pieceTypeNumber<int(numStartingPiecesPerType[pieceType])) {
                setup.emplace(placement);
                return make_tuple(subnode,chunk,runtime_error());
              }
              else
                return make_tuple(Subnode(),chunk,runtime_error(QCoreApplication::translate("","Out of piece type: ")+pieceName(placement.piece)));
            }
            else
              return make_tuple(Subnode(),chunk,runtime_error(QCoreApplication::translate("","Duplicate setup square: ")+toCoordinates(placement.location).data()));
          }
          else
            return make_tuple(Subnode(),chunk,runtime_error(QCoreApplication::translate("","Illegal setup square: ")+toCoordinates(placement.location).data()));
        }
        else
          ss.seekg(posBefore);
      }
      else {
        const Side side=toMoveStart(chunk).first;
        if (side!=NO_SIDE) {
          if (node->gameState.sideToMove==side)
            endMove=false;
        }
        else if (chunk=="pass" || toDisplacement(chunk,false).first.isValid())
          ss.seekg(posBefore);
        else
          return make_tuple(Subnode(),chunk,runtime_error(QCoreApplication::translate("","Invalid word: ")+QString::fromStdString(chunk)));
      }
      if (endMove) {
        if (setup.size()<numStartingPieces)
          return make_tuple(Subnode(),chunk,runtime_error(QCoreApplication::translate("","Illegal end of setup: ")+QString::fromStdString(chunk)));
        else
          node=Node::addSetup(node,setup,after);
      }
    }
    else {
      const GameState& gameState=move.empty() ? node->gameState : resultingState(move);
      const Side side=toMoveStart(chunk).first;
      if (node->result.endCondition!=NO_END && node->gameState.sideToMove!=side)
        return make_tuple(Subnode(),chunk,runtime_error(QCoreApplication::translate("","Play in finished position: ")+QString::fromStdString(chunk)));
      else if (side!=NO_SIDE) {
        if (node->gameState.sideToMove==side)
          endMove=false;
      }
      else if (gameState.stepsAvailable>0) {
        if (chunk!="pass") {
          const auto displacement=toDisplacement(chunk,false);
          if (displacement.first.isValid()) {
            const SquareIndex destination=displacement.second;
            if (destination==NO_SQUARE)
              return make_tuple(Subnode(),chunk,runtime_error(QCoreApplication::translate("","Capture without step: ")+QString::fromStdString(chunk)));
            else {
              std::string captureWord;
              posBefore=ss.tellg();
              if (ss>>captureWord) {
                const auto capture=toDisplacement(captureWord,false);
                if (capture.first.isValid() && capture.second==NO_SQUARE)
                  chunk+=' '+captureWord;
                else
                  ss.seekg(posBefore);
              }
              else {
                ss.clear();
                ss.seekg(posBefore);
              }
              const SquareIndex origin=displacement.first.location;
              if (gameState.legalStep(origin,destination)) {
                const auto step=GameState(gameState).takeExtendedStep(origin,destination);
                if (toString(step)==chunk) {
                  move.emplace_back(step);
                  return make_tuple(subnode,chunk,runtime_error());
                }
              }
              return make_tuple(Subnode(),chunk,runtime_error(QCoreApplication::translate("","Invalid step: ")+QString::fromStdString(chunk)));
            }
          }
          else
            return make_tuple(Subnode(),chunk,runtime_error(QCoreApplication::translate("","Invalid word: ")+QString::fromStdString(chunk)));
        }
      }
      else
        ss.seekg(posBefore);
      if (endMove) {
        switch (node->legalMove(gameState)) {
          case MoveLegality::LEGAL:
            node=Node::makeMove(node,move,after);
          break;
          case MoveLegality::ILLEGAL_PUSH_INCOMPLETION:
            return make_tuple(Subnode(),chunk,runtime_error(QCoreApplication::translate("","Incomplete push: ")+QString::fromStdString(toString(move))));
          case MoveLegality::ILLEGAL_PASS:
            return make_tuple(Subnode(),chunk,runtime_error(QCoreApplication::translate("","Illegal pass: ")+QString::fromStdString(toString(move))));
          case MoveLegality::ILLEGAL_REPETITION:
            return make_tuple(Subnode(),chunk,runtime_error(QCoreApplication::translate("","Illegal repetition: ")+QString::fromStdString(toString(move))));
        }
      }
    }
    setup.clear();
    move.clear();
    if (ss.tellg()==posBefore)
      return parseChunk(ss,subnode,after);
    else
      return make_tuple(subnode,chunk,runtime_error());
  }
  else
    return make_tuple(Subnode(),chunk,runtime_error());
}

inline std::tuple<GameTree,size_t> toTree(const std::string& input,NodePtr node,Placements& setup,ExtendedSteps& move)
{
  assert(node!=nullptr);
  GameTree gameTree;
  unsigned int nodeChanges=0;

  std::stringstream ss;
  ss<<input;
  while (true) {
    const auto result=parseChunk(ss,Subnode(node,setup,move),false);
    const auto& newPosition=std::get<0>(result);
    const auto& newNode=std::get<0>(newPosition);
    setup=std::get<1>(newPosition);
    move=std::get<2>(newPosition);
    if (newNode==nullptr) {
      const auto error=std::get<2>(result);
      if (error.what()==std::string())
        break;
      else
        throw error;
    }
    else if (newNode!=node) {
      if (newNode==node->previousNode)
        gameTree.push_front(node);
      node=newNode;
      ++nodeChanges;
    }
  }
  gameTree.push_front(node);

  return make_tuple(gameTree,nodeChanges);
}

inline std::tuple<GameTree,size_t,bool> toTree(const std::string& input,const NodePtr& startingNode)
{
  Placements setup;
  ExtendedSteps move;
  auto result=toTree(input,startingNode,setup,move);
  auto& gameTree=std::get<0>(result);
  auto& node=gameTree.front();
  auto& nodeChanges=std::get<1>(result);

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

  return make_tuple(gameTree,nodeChanges,completeLastMove);
}

inline TurnState customizedTurnState(const std::string& input,TurnState turnState=TurnState())
{
  std::stringstream ss;
  ss<<input;
  std::string word;
  while (ss>>word) {
    if (word=="pass")
      continue;
    const Side side=toMoveStart(word).first;
    if (side!=NO_SIDE)
      turnState.sideToMove=side;
    else {
      const auto placement=toPlacement(word,false);
      if (placement.isValid()) {
        if (turnState.squarePieces[placement.location]!=placement.piece) {
          if (turnState.piecesAtMax()[placement.piece])
            throw runtime_error(QCoreApplication::translate("","Side out of pieces of type: ")+pieceName(placement.piece));
          else
            turnState.squarePieces[placement.location]=placement.piece;
        }
      }
      else {
        const auto displacement=toDisplacement(word,false);
        const auto& placement=displacement.first;
        if (placement.isValid()) {
          if (turnState.squarePieces[placement.location]==placement.piece) {
            turnState.squarePieces[placement.location]=NO_PIECE;
            if (displacement.second!=NO_SQUARE)
              turnState.squarePieces[displacement.second]=placement.piece;
          }
          else
            throw runtime_error(QCoreApplication::translate("","Piece type not on %1: ").arg(toCoordinates(placement.location).data())+pieceName(placement.piece));
        }
        else
          throw runtime_error(QCoreApplication::translate("","Invalid word: ")+QString::fromStdString(word));
      }
    }
  }
  return turnState;
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
