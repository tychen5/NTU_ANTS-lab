/****************************************************************************
  FileName     [ cmdParser.h ]
  PackageName  [ cmd ]
  Synopsis     [ Define class CmdParser ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2007-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/
#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "cmdCharDef.h"

using namespace std;

void mybeep();
char mygetc(istream&);
ParseChar getChar(istream&);


template <class T>
class CmdParser {
#define READ_BUF_SIZE 65536
#define TAB_POSITION 8
#define PG_OFFSET 10

public:
    CmdParser(): _readBufPtr(_readBuf), _readBufEnd(_readBuf),
                 _historyIdx(0), _tempCmdStored(false)
    {
        _t = new T();
    }
    virtual ~CmdParser() { delete _t; }
    void readCmd();

private:
    // Private member functions
    void resetBufAndPrintPrompt()
    {
        _readBufPtr = _readBufEnd = _readBuf;
        *_readBufPtr = 0;
        printPrompt();
    }
    void readCmdInt(istream&);
    void printPrompt() const { if (_history.size() > 0) cout << "\n"; cout << "Client> "; }
    bool moveBufPtr(char* const);
    bool deleteChar();
    void insertChar(char, int = 1);
    void deleteLine();
    void moveToHistory(int index);
    void addHistory();
    void retrieveHistory();

    // Data members
    char _readBuf[READ_BUF_SIZE];
    char* _readBufPtr;
    char* _readBufEnd;
    vector<string> _history;
    int _historyIdx;
    bool _tempCmdStored;

    T* _t;
};

template <class T>
void CmdParser<T>::readCmd()
{
   readCmdInt(cin);
}

template <class T>
void CmdParser<T>::readCmdInt(istream& istr)
{
   resetBufAndPrintPrompt();

   while (1) {
      ParseChar pch = getChar(istr);
      if (pch == INPUT_END_KEY) { _t->disconnect(); break; }
      switch (pch) {
         case LINE_BEGIN_KEY :
         case HOME_KEY       : moveBufPtr(_readBuf); break;
         case LINE_END_KEY   :
         case END_KEY        : moveBufPtr(_readBufEnd); break;
         case BACK_SPACE_KEY : if (moveBufPtr(_readBufPtr - 1)) deleteChar(); break;
         case DELETE_KEY     : deleteChar(); break;
         case NEWLINE_KEY    : addHistory();
                               cout << char(NEWLINE_KEY);
                               _t->handle(string(_readBuf)); 
                               resetBufAndPrintPrompt(); break;
         case ARROW_UP_KEY   : moveToHistory(_historyIdx - 1); break;
         case ARROW_DOWN_KEY : moveToHistory(_historyIdx + 1); break;
         case ARROW_RIGHT_KEY: moveBufPtr(_readBufPtr + 1); break;
         case ARROW_LEFT_KEY : moveBufPtr(_readBufPtr - 1); break;
         case PG_UP_KEY      : moveToHistory(_historyIdx - PG_OFFSET); break;
         case PG_DOWN_KEY    : moveToHistory(_historyIdx + PG_OFFSET); break;
         case TAB_KEY        : insertChar(' ',
                               TAB_POSITION - (_readBufPtr -_readBuf) % TAB_POSITION
                               ); break;
         case INSERT_KEY     : // not yet supported; fall through to UNDEFINE
         case UNDEFINED_KEY:   mybeep(); break;
         default:  // printable character
            insertChar(char(pch)); break;
      }
   }
}
template <class T>
bool CmdParser<T>::moveBufPtr(char* const ptr)
{
   // TODO...
   if ((ptr < _readBuf) | (ptr > _readBufEnd)) {
      mybeep();
      return false;
   }
   if (ptr <= _readBufPtr) {
      int step = _readBufPtr - ptr;
      for (int i = 0; i < step; i++) {
         cout << '\b';
      }
   }
   else {
      int step = ptr - _readBufPtr;
      for (int i = 0; i < step; i++) {
         cout << *(_readBufPtr + i);
      }
   }
   cout.flush();
   _readBufPtr = ptr;
   return true;
}

template <class T>
bool CmdParser<T>::deleteChar()
{
   // TODO...
   if (_readBufPtr == _readBufEnd) {
      mybeep();
      return false;
   }
   char* ptr = _readBufPtr;
   while (ptr < _readBufEnd) {
      *ptr = *(ptr + 1);
      if (*ptr == '\0') cout << " ";
      else cout << *ptr;
      ptr++;
   }
   *(--_readBufEnd) = '\0';
   for (int i = 0; i <= _readBufEnd - _readBufPtr; i++) {
      cout << '\b';
   }
   cout.flush();
   return true;
}

template <class T>
void CmdParser<T>::insertChar(char ch, int repeat)
{
   // TODO...
   assert(repeat >= 1);
   _readBufEnd += repeat;
   char* eos = _readBufEnd;
   while (eos - repeat >= _readBufPtr) {
      *eos = *(eos - repeat);
      eos--; 
   }
   
   for (int i = 0; i < repeat; i++) {
      *(_readBufPtr + i) = ch;
   }
   cout << _readBufPtr;
   cout.flush();

   _readBufPtr += repeat;
   eos = _readBufEnd;
   for (int i = 0; i < _readBufEnd - _readBufPtr; i++) {
      cout << '\b';
   }
   cout.flush();

}

template <class T>
void CmdParser<T>::deleteLine()
{
   // TODO...
   for (int i = 0; i < _readBufPtr - _readBuf; i++) {
      cout << '\b';
   }
   for (size_t i = 0; i < strlen(_readBuf); i++) {
      cout << ' ';
   }
   cout.flush();
   for (size_t i = 0; i < strlen(_readBuf); i++) {
      cout << '\b';
   }
   cout.flush();
   _readBufPtr = _readBufEnd = _readBuf;
   *_readBufPtr = 0;
}

template <class T>
void CmdParser<T>::moveToHistory(int index)
{
   // TODO...
   if (_historyIdx == 0 && index < 0) {
      mybeep();
      return;
   }
   if (_historyIdx >= (int)_history.size()-1 && index > _historyIdx) {
      mybeep();
      return;
   }

   if (!_tempCmdStored) {
      _history.push_back(_readBuf);
      _tempCmdStored = true; 
   } else if (_historyIdx == (int)_history.size()-1) {
      _history.back() = _readBuf;
   }

   if (index < _historyIdx) {
      _historyIdx = max(0, index);
   }
   if (index > _historyIdx) {
      _historyIdx = min((int)_history.size()-1, index);
   }

   retrieveHistory();
   return;
}

template <class T>
void CmdParser<T>::addHistory()
{
   // TODO...
   if (_tempCmdStored) {
      _history.pop_back();
   }
   char* ch = _readBuf;

   // Trim leading space
   while (isspace(*ch)) ch++;
   if (*ch != 0) {

      // Trim trailing space
      char *back = _readBuf + strlen(_readBuf) - 1;
      while (back > ch && isspace(*back)) back--;

      *(back + 1) = '\0';
      _history.push_back(ch);
   }
   _tempCmdStored = false;
   _historyIdx = _history.size();
}

template <class T>
void CmdParser<T>::retrieveHistory()
{
   deleteLine();

   strcpy(_readBuf, _history[_historyIdx].c_str());
   cout << _readBuf;
   _readBufPtr = _readBufEnd = _readBuf + _history[_historyIdx].size();
}

#endif // CMD_PARSER_H
