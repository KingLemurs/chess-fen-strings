#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"
#include "GameState.h"

// FILE = COL
// RANK = ROW
constexpr int pieceSize = 80;

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;
	void updateAI() override;
    bool gameHasAI() override { return true; }

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;
    void endTurn() override;

    void GenKnightBoards();
    void GenKingBoards();
    int evaluateBoard(const std::string& state);
    int negamax(GameState& gamestate, int depth, int alpha, int beta, int playerColor);

    Grid* _grid;
    GameState gs;
    // instead of generating move bitboards for EVERY possible move for every piece, generate for pieces on the board
    // removes the need for the huge switch statement
    BitBoard _bitboards[13];
    int _bitboardLookup[128]; // for converting char indecies to 

    BitBoard _knightBoards[64];
    BitBoard _kingBoards[64];
    int _pieceSquares[128][64];
    std::vector<BitMove> _moves;
    std::vector<ChessSquare*> _highlights;
    
    int negInfinite = -1000000;
    int posInfinite = 1000000;
    int _countMoves = 0;
};