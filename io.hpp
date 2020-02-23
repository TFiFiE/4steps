#ifndef IO_HPP
#define IO_HPP

#include <sstream>
#include "node.hpp"

const char pieceLetters[]="RrCcDdHhMmEe ";

inline PieceTypeAndSide toPieceTypeAndSide(const char pieceLetter)
{
  const auto pointer=strchr(pieceLetters,pieceLetter);
  if (pointer==nullptr)
    throw std::runtime_error("Unrecognized piece letter: "+std::string(1,pieceLetter));
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

inline SquareIndex toSquare(const char fileLetter,const char rankLetter)
{
  const auto result=toSquare(static_cast<unsigned int>(fileLetter-'a'),
                             static_cast<unsigned int>(rankLetter-'1'));
  if (result==NO_SQUARE)
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

inline SquareIndex toDestination(const SquareIndex origin,const char directionLetter)
{
  switch (directionLetter) {
    case 's': return static_cast<SquareIndex>(origin-NUM_FILES);
    case 'w': return static_cast<SquareIndex>(origin-1);
    case 'e': return static_cast<SquareIndex>(origin+1);
    case 'n': return static_cast<SquareIndex>(origin+NUM_FILES);
    default : throw std::runtime_error("Unrecognized destination letter: "+std::string(1,directionLetter));
  }
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
  ss<<(moveIndex/2)+1<<"gs"[moveIndex%2];
  return ss.str();
}

inline std::string toPlyString(const int moveIndex,const Node& root)
{
  return toPlyString(moveIndex+(root.isGameStart() ? 0 : NUM_SIDES+root.currentState.sideToMove));
}

inline int toMoveIndex(const std::string& input)
{
  if (input.size()<2)
    return -1;
  const Side side=toSide(input.back());
  if (side==NO_SIDE)
    return -1;
  const auto moveNumber=input.substr(0,input.size()-1);
  if (all_of(moveNumber.begin(),moveNumber.end(),isdigit))
    return (stoi(moveNumber)-1)*2+(side==FIRST_SIDE ? 0 : 1);
  else
    return -1;
}

inline Placements toPlacements(const std::string& input)
{
  Placements result;
  std::stringstream ss;
  ss<<input;
  std::string word;
  while (ss>>word) {
    runtime_assert(word.size()==3,"Placement word does not have 3 letters.");
    const PieceTypeAndSide piece=toPieceTypeAndSide(word[0]);
    const SquareIndex square=toSquare(word[1],word[2]);
    result.emplace(Placement{square,piece});
  }
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

inline PieceSteps toMove(const std::string& input)
{
  PieceSteps result;
  std::stringstream ss;
  ss<<input;
  std::string word;
  while (ss>>word) {
    runtime_assert(word.size()==4,"Step word does not have 4 letters.");
    const PieceTypeAndSide piece=toPieceTypeAndSide(word[0]);
    const SquareIndex origin=toSquare(word[1],word[2]);
    if (word[3]=='x') {
      runtime_assert(!result.empty(),"Capture is first word of move.");
      runtime_assert(adjacentTrap(std::get<ORIGIN>(result.back()))==origin,"Capturing step is not adjacent to trap.");
      std::get<TRAPPED_PIECE>(result.back())=piece;
    }
    else {
      const SquareIndex destination=toDestination(origin,word[3]);
      result.emplace_back(origin,destination,NO_PIECE,piece);
    }
  }
  return result;
}

inline std::pair<GameTree,size_t> toTree(const std::string& lines,NodePtr root)
{
  assert(root!=nullptr);
  assert(root->isGameStart());
  GameTree gameTree(1,std::move(root));

  std::stringstream ss;
  ss<<lines;

  std::vector<std::pair<size_t,std::string> > moves;
  int moveIndex=-1;
  std::string move;
  std::string word;
  while (ss>>word) {
    const int newMoveIndex=toMoveIndex(word);
    if (newMoveIndex>=0) {
      if (!move.empty()) {
        runtime_assert(moveIndex>=0,"Move list does not begin with move index.");
        moves.emplace_back(moveIndex,move);
        move.clear();
      }
      moveIndex=newMoveIndex;
    }
    else
      move+=word+' ';
  }
  if (!move.empty()) {
    runtime_assert(moveIndex>=0,"Move list does not begin with move index.");
    moves.emplace_back(moveIndex,move);
  }
  for (const auto& move:moves) {
    auto& node=gameTree.front();
    if (move.second=="takeback") {
      gameTree.emplace(gameTree.begin(),node);
      gameTree.front()=gameTree.front()->previousNode;
    }
    else if (move.first<NUM_SIDES)
      node=Node::addSetup(node,toPlacements(move.second),false);
    else
      node=Node::makeMove(node,toMove(move.second),false);
  }
  return {gameTree,moves.size()};
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
