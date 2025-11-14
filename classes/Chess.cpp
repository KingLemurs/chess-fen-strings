#include "Chess.h"
#include <limits>
#include <cmath>
#include <string>
#include <sstream> // Required for std::stringstream
#include <vector>

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    _highlights.reserve(32);

    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    GenKnightBoards();
    GenKingBoards();
    GenAllMoves();
    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(fen); // Create a string stream from the input string

    while (std::getline(tokenStream, token, '/')) { // Read until delimiter
        tokens.push_back(token);
    }

    for (int row = 0; row < tokens.size(); row++) {
        std::string line = tokens[row];
        int offset = 0;
        for (int i = 0; i < line.size(); i++) {
            // add offset
            if (!isalpha(line[i])) {
                offset += line[i] - '0';
                continue;
            }

            int color = 0;
            ChessPiece p;
            if (line[i] < 97) {
                color = 1;
            }

            switch (tolower(line[i])) {
                case 'r':
                    p = ChessPiece::Rook;
                    break;
                case 'p':
                    p = ChessPiece::Pawn;
                    break;
                case 'b':
                    p = ChessPiece::Bishop;
                    break;
                case 'n':
                    p = ChessPiece::Knight;
                    break;
                case 'q':
                    p = ChessPiece::Queen;
                    break;
                case 'k':
                    p = ChessPiece::King;
                    break;
                default:
                    p = ChessPiece::Pawn;
                    break;
            }

            // get and place piece
            Bit* bit = PieceForPlayer(color, p);
            bit->setPosition(_grid->getSquare(i, row)->getPosition());
            bit->setGameTag(p + (color == 0 ? 0 : 128));
            _grid->getSquare(i, row)->setBit(bit);
        }
    }
}

// compile all possible moves for knights FROM each index
void Chess::GenKnightBoards() {
    for (int y = 0; y < _gameOptions.rowX; y++) {
        for (int x = 0; x < _gameOptions.rowY; x++) {
            int index = y * 8 + x;
            u_int64_t data = _knightBoards[index].getData();
            std::pair<int, int> knightOffsets[] = {
                {2,1}, {2,-1}, {-2,1}, {-2,-1},
                {1,2}, {1,-2}, {-1,2}, {-1,-2} 
            };
            
            for (auto [ox, oy] : knightOffsets) {
                // bc y is row and x is col
                int rank = y + ox, file = x + oy;
                if (rank >= 0 && rank < 8 && file >= 0 && file < 8) {
                    // set bit
                    data |= 1ULL << (rank * 8 + file);
                }
            }
            _knightBoards[index].setData(data);
        }
    }
}

void Chess::GenKingBoards() {
    for (int y = 0; y < _gameOptions.rowX; y++) {
        for (int x = 0; x < _gameOptions.rowY; x++) {
            int index = y * 8 + x;
            u_int64_t data = _kingBoards[index].getData();
            std::pair<int, int> kingOffsets[] = {
                {0,1}, {0,-1}, {1,0}, {-1,0},
                {1,1}, {1,-1}, {-1,1}, {-1,-1} 
            };
            
            for (auto [ox, oy] : kingOffsets) {
                // bc y is row and x is col
                int rank = y + ox, file = x + oy;
                if (rank >= 0 && rank < 8 && file >= 0 && file < 8) {
                    // set bit
                    data |= 1ULL << (rank * 8 + file);
                }
            }
            _kingBoards[index].setData(data);
        }
    }
}

void Chess::GenAllMoves() {
    _moves.reserve(32);
    std::string state = stateString();
    u_int64_t occupancy = 0LL;
    u_int64_t blackOccupancy = 0LL;
    BitboardElement whiteKnights;
    BitboardElement whitePawns;
    BitboardElement whiteKing;
    BitboardElement blackKnights;
    BitboardElement blackPawns;
    BitboardElement blackKing;
    u_int64_t knightData = whiteKnights.getData();
    u_int64_t pawnData = whitePawns.getData();
    u_int64_t kingData = whiteKing.getData();
    u_int64_t blackKnightData = whiteKnights.getData();
    u_int64_t blackPawnData = whitePawns.getData();
    u_int64_t blackKingData = blackKing.getData();
    for (int i = 0; i < 64; i++) {
        if (state[i] == 'P') {
            pawnData |= 1ULL << i;
        }
        else if (state[i] == 'N') {
            knightData |= 1ULL << i;
        }
        else if (state[i] == 'K') {
            kingData |= 1ULL << i;
        }
        else if (state[i] == 'p') {
            blackPawnData |= 1ULL << i;
        }
        else if (state[i] == 'n') {
            blackKnightData |= 1ULL << i;
        }
        else if (state[i] == 'k') {
            blackKingData |= 1ULL << i;
        }
    }
    whiteKnights.setData(knightData);
    whitePawns.setData(pawnData);
    whiteKing.setData(kingData);
    blackKnights.setData(blackKnightData);
    blackPawns.setData(blackPawnData);
    blackKing.setData(blackKingData);

    occupancy = knightData | pawnData | kingData;
    blackOccupancy = blackKnightData | blackPawnData | blackKingData;
    GenKnightMoves(_moves, whiteKnights, ~occupancy);
    GenKnightMoves(_moves, blackKnights, ~blackOccupancy);
    GenKingMoves(_moves, whiteKing, ~occupancy);
    GenKingMoves(_moves, blackKing, ~blackOccupancy);

    if (getCurrentPlayer()->playerNumber() == 0) {
        GenPawnMoves(_moves, whitePawns, ~(occupancy | blackOccupancy), blackOccupancy);
    } else {
        GenPawnMoves(_moves, blackPawns, ~(blackOccupancy | occupancy), occupancy);
    }
}

void Chess::GenKnightMoves(std::vector<BitMove>& moves, BitboardElement board, u_int64_t emptySquares) {
    board.forEachBit([&](int fromSquare) {
        // and-ing with emptySquares will remove any moves that are impossible in the current context
        BitboardElement moveBitboard = BitboardElement(_knightBoards[fromSquare].getData() & emptySquares);

        // use bit logic to efficiently go through each set bit
        moveBitboard.forEachBit([&](int toSquare) {
            // emplace_back does not copy so better performance
            moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

void Chess::GenKingMoves(std::vector<BitMove>& moves, BitboardElement board, u_int64_t emptySquares) {
    board.forEachBit([&](int fromSquare) {
        // and-ing with emptySquares will remove any moves that are impossible in the current context
        BitboardElement moveBitboard = BitboardElement(_kingBoards[fromSquare].getData() & emptySquares);

        // use bit logic to efficiently go through each set bit
        moveBitboard.forEachBit([&](int toSquare) {
            // emplace_back does not copy so better performance
            moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}

void Chess::GenPawnMoves(std::vector<BitMove>& moves, BitboardElement board, u_int64_t emptySquares, u_int64_t enemyPieces) {
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;

    // single moves
    // shifting 8 moves up bits one rank
    BitboardElement singleMoves = (currentPlayer == 0) ? (board.getData() << 8) & emptySquares : (board.getData() >> 8) & emptySquares;

    // we have the mask that shifts 2 ranks
    BitboardElement doubleMoves = (currentPlayer == 0) ? ((singleMoves.getData() & Rank3) << 8) & emptySquares : ((singleMoves.getData() & Rank6) >> 8) & emptySquares;

    // shift 7 so that the bits align one left, AFile mask blocks the first col
    BitboardElement leftMoves = (currentPlayer == 0) ? ((board.getData() & AFileMask) << 7) & enemyPieces : ((board.getData() & HFileMask) >> 9) & enemyPieces;
    // shift 9 so that the bits align one right, HFile mask blocks the last col
    BitboardElement rightMoves = (currentPlayer == 0) ? ((board.getData() & HFileMask) << 9) & enemyPieces : ((board.getData() & AFileMask) >> 7) & enemyPieces;

    int dir = (currentPlayer == 0) ? 8 : -8;
    int captureLeft = (currentPlayer == 0) ? 7 : -9;
    int captureRight = (currentPlayer == 0) ? 9 : -7;
    singleMoves.forEachBit([&](int toSquare){
        _moves.emplace_back(toSquare - dir, toSquare, Pawn);
    });

    leftMoves.forEachBit([&](int toSquare){
        _moves.emplace_back(toSquare - captureLeft, toSquare, Pawn);
    });

    rightMoves.forEachBit([&](int toSquare){
        _moves.emplace_back(toSquare - captureRight, toSquare, Pawn);
    });

    doubleMoves.forEachBit([&](int toSquare){
        _moves.emplace_back(toSquare - dir * 2, toSquare, Pawn);
    });
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

void Chess::endTurn() {
    _moves.clear();

    // remove highlights
    for (ChessSquare* sq : _highlights) {
        sq->setHighlighted(false);
    }
    _highlights.clear();
    Game::endTurn();
    GenAllMoves();
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // remove highlights
    for (ChessSquare* sq : _highlights) {
        sq->setHighlighted(false);
    }
    _highlights.clear();

    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;

    if (pieceColor == currentPlayer) {
        //if the piece is yours
        bool ret = false;
        ChessSquare* square = (ChessSquare *)&src;
        if(square) {
            int squareIndex = square->getSquareIndex();
            for(auto move : _moves) { //check each move
                if(move.from == squareIndex) {
                    auto dest = _grid->getSquareByIndex(move.to);

                    if (dest->bit() && dest->bit()->getOwner() == getCurrentPlayer()) continue;

                    ret = true;
                    dest->setHighlighted(true);
                    _highlights.emplace_back(dest);
                }
            }
        }
        return ret;
    }
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    for (BitMove move : _moves) {
        if (move.from == static_cast<ChessSquare&>(src).getSquareIndex() && move.to == static_cast<ChessSquare&>(dst).getSquareIndex()) {
            return true;
        }
    }
    return false;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}
