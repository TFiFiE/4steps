#ifndef WORDBUFFER_HPP
#define WORDBUFFER_HPP

#include <sstream>
#include <queue>

class WordBuffer {
public:
  WordBuffer(const std::string& string)
  {
    ss<<string;
  }
  bool get(std::string* const word)
  {
    if (peekedWords.empty()) {
      if (word==nullptr) {
        std::string temp;
        return static_cast<bool>(ss>>temp);
      }
      else
        return static_cast<bool>(ss>>*word);
    }
    else {
      if (word!=nullptr)
        *word=peekedWords.front();
      peekedWords.pop();
      return true;
    }
  }
  bool peek(std::string& word)
  {
    if (peekedWords.empty()) {
      if (ss>>word) {
        peekedWords.push(word);
        return true;
      }
      else
        return false;
    }
    else {
      word=peekedWords.front();
      return true;
    }
  }
private:
  std::stringstream ss;
  std::queue<std::string> peekedWords;
};

#endif // WORDBUFFER_HPP
