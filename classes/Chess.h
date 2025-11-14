#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"

// FILE = COL
// RANK = ROW
constexpr int pieceSize = 80;
constexpr u_int64_t AFileMask(0xFEFEFEFEFEFEFEFEULL); // A-File Mask
constexpr u_int64_t HFileMask(0x7F7F7F7F7F7F7F7FULL); // H-File Mask
constexpr u_int64_t Rank3(0x0000000000FF0000ULL); // Rank 3 Mask
constexpr u_int64_t Rank6(0x0000FF0000000000ULL); // Rank 6 Mask

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

    void GenAllMoves();
    void GenKnightBoards();
    void GenKingBoards();
    void GenKnightMoves(std::vector<BitMove>& moves, BitboardElement board, u_int64_t emptySquares);
    void GenPawnMoves(std::vector<BitMove>& moves, BitboardElement board, u_int64_t emptySquares, u_int64_t enemyPieces);
    void GenKingMoves(std::vector<BitMove>& moves, BitboardElement board, u_int64_t emptySquares);

    Grid* _grid;
    BitboardElement _knightBoards[64];
    BitboardElement _kingBoards[64];
    std::vector<BitMove> _moves;
    std::vector<ChessSquare*> _highlights;
};