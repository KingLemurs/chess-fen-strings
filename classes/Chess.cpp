#include "Chess.h"
#include "Logger.h"
#include "PieceSquare.h"
#include <limits>
#include <cmath>
#include <string>
#include <sstream> // Required for std::stringstream
#include <vector>
#include <chrono>
#include <iomanip>

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
    _gameOptions.AIMAXDepth = 5;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    _highlights.reserve(32);

    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    GenKnightBoards();
    GenKingBoards();
    //GenAllMoves(_moves, stateString(), getCurrentPlayer()->playerNumber() * 128 == 0 ? 1 : -1);
    gs.init(stateString().c_str(), 1);
    _moves = gs.generateAllMoves();

    memcpy(_pieceSquares['p'], pawnTable, 64);
    memcpy(_pieceSquares['P'], pawnTable, 64);
    memcpy(_pieceSquares['r'], rookTable, 64);
    memcpy(_pieceSquares['R'], rookTable, 64);
    memcpy(_pieceSquares['b'], bishopTable, 64);
    memcpy(_pieceSquares['B'], bishopTable, 64);
    memcpy(_pieceSquares['n'], knightTable, 64);
    memcpy(_pieceSquares['N'], knightTable, 64);
    memcpy(_pieceSquares['q'], queenTable, 64);
    memcpy(_pieceSquares['Q'], queenTable, 64);
    memcpy(_pieceSquares['k'], kingTable, 64);
    memcpy(_pieceSquares['K'], kingTable, 64);
    startGame();

    if (gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }
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
    //GenAllMoves(_moves, stateString(), getCurrentPlayer()->playerNumber() * 128 == 0 ? 1 : -1);
    gs.init(stateString().c_str(), getCurrentPlayer()->playerNumber() * 128 == 0 ? 1 : -1);
    _moves = gs.generateAllMoves();
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
    gs.shutdown();
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


static std::map<char, int> evaluateScores = {
    {'P', 100}, {'p', -100},    // Pawns
    {'N', 200}, {'n', -200},    // Knights
    {'B', 230}, {'b', -230},    // Bishops
    {'R', 400}, {'r', -400},    // Rooks
    {'Q', 900}, {'q', -900},    // Queens
    {'K', 2000}, {'k', -2000},  // Kings
    {'0', 0}                     // Empty squares
};

int Chess::evaluateBoard(const std::string& state) {
    int value = 0;
    int index = 0;
    for(char ch : state) {
        value += evaluateScores[ch];
        value += _pieceSquares[ch][index];
        index++;
    }
    return value;
}

void Chess::updateAI()
{
    const auto searchStart = std::chrono::steady_clock::now();
    int bestVal = negInfinite;
    BitMove bestMove;
    GameState newGs;
    newGs.init(stateString().c_str(), -1);
    _countMoves = 0;

    // Search through current legal moves
    for(auto move : _moves) {
        // std::cout << (int)move.from << ": " << (int)move.to << std::endl;
        newGs.pushMove(move);

        // Call negamax to evaluate this move
        int moveVal = -negamax(newGs, _gameOptions.AIMAXDepth, negInfinite, posInfinite, 1);

        newGs.popState();

        // Track the best move found
        if (moveVal > bestVal) {
            bestMove = move;
            bestVal = moveVal;
        }
    }

    std::cout << bestVal << std::endl;
    // Execute the best move on the actual board
    // I’m kind of amazed this code works and will be improving it
    if(bestVal != negInfinite) {
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - searchStart).count();
        const double boardsPerSecond = seconds > 0.0 ? static_cast<double>(_countMoves) / seconds : 0.0;
        std::cout << "Moves checked: " << _countMoves
                    << " (" << std::fixed << std::setprecision(2) << boardsPerSecond
                    << " boards/s)" << std::defaultfloat << std::endl;

        int srcSquare = bestMove.from;
        int dstSquare = bestMove.to;
        BitHolder& src = getHolderAt(srcSquare&7, srcSquare/8);
        BitHolder& dst = getHolderAt(dstSquare&7, dstSquare/8);
        Bit* bit = src.bit();
        dst.dropBitAtPoint(bit, ImVec2(0, 0));
        src.setBit(nullptr);
        bitMovedFromTo(*bit, src, dst);
    }
}

int Chess::negamax(GameState& gamestate, int depth, int alpha, int beta, int playerColor)
{
    _countMoves++;

    // Base case: at leaf nodes, evaluate the position
    if (depth == 0) {
        return evaluateBoard(gamestate.state) * playerColor;
    }

    // Generate moves for THIS board state (critical!)
    std::vector<BitMove> newMoves = gamestate.generateAllMoves();

    int bestVal = negInfinite; // Start with worst possible value

    for(const auto& move : newMoves) {
        gamestate.pushMove(move);
        bestVal = std::max(bestVal, -negamax(gamestate, depth - 1, -beta, -alpha, -playerColor));
        // Undo the move
        gamestate.popState();
        // alpha beta cut-off
        alpha = std::max(alpha, bestVal);
        if (alpha >= beta) {
            break;
        }
    }

    return bestVal;
}
