#include "lexer.h"

#include <iostream>
#include <cctype>

#include "keywords.h"

Lexer::Lexer() {}

/*
 * Punto di entrata dell'analisi lessicale.
 * Questa funzione analizza una stringa assegnata dividendola in tokens secondo la grammatica del
 * linguaggio. Per dividere i caratteri in token, analizza i singoli caratteri per richiamare funzioni
 * di scan ed estrapolare tutti i caratteri che andranno a formare i tokens.
 */

std::vector<Token> Lexer::analiseString(const std::string &string, ErrorLog &_errorLog)
{
    this->errorLog = &_errorLog;

    m_tokens.clear();

    buffer = string;

    indexPos = 0;
    currentTextPos.column = 0;
    currentTextPos.line = 0;

    removeAll(buffer, "\r");
    removeAll(buffer, "\t");

    while(!isAtEnd())
    {
        char c = peek();

        if(c == ' ') {
            advance();
        }
        else if(c == '\n') {
            advance();
        }
        else if(isDigit(c))
        {
            Token num = scanNumber();
            m_tokens.push_back(num);
        }
        else if(isAlpha(c))
        {
            Token identifier = scanIdentifier();
            m_tokens.push_back(identifier);
        }
        else if(c == '"')
        {
            Token str = scanString();
            m_tokens.push_back(str);
        }
        else if(c == '\'')
        {
            Token ch = scanChar();
            m_tokens.push_back(ch);
        }
        else if(c == '+') {
            Token tplus = createToken(TokenType::Plus);
            m_tokens.push_back(tplus);
        }
        else if(c == '-') {
            Token tmin = createToken(TokenType::Minus);
            m_tokens.push_back(tmin);
        }
        else if(c == '/')
        {
            if(peek(1) == '/') {
                // commento '//'
                while(!isAtEnd() && peek() != '\n') {
                    advance();
                }
            }
            else if(peek(1) == '*') {
                // commento '/*'
                while(!isAtEnd() && !(peek() == '*' && peek(1) == '/')) {
                    advance();
                }

                if(isAtEnd()) {
                    errorLog->addError("Unterminated multi-line comment", currentTextPos);
                } else {
                    advance(); //consuma '*'
                    advance(); //consuma '/'
                }
            } else {
                Token tdiv = createToken(TokenType::Slash);
                m_tokens.push_back(tdiv);
            }
        }
        else if(c == '*') {
            Token tstar = createToken(TokenType::Star);
            m_tokens.push_back(tstar);
        }
        else if(c == '=') {
            if(peek(1) == '=') {
                Token t = createToken(TokenType::EqualEqual);
                t.lexeme.push_back(advance());
                m_tokens.push_back(t);
            } else {
                Token t = createToken(TokenType::Equal);
                m_tokens.push_back(t);
            }
        }
        else if(c == '!') {
            if(peek(1) == '=') {
                Token t = createToken((TokenType::NotEqual));
                t.lexeme.push_back(advance());
                m_tokens.push_back(t);
            } else {
                Token t = createToken(TokenType::LogicalNot);
                m_tokens.push_back(t);
            }
        }
        else if(c == '&') {
            if(peek(1) == '&') {
                Token t = createToken(TokenType::LogicalAnd);
                t.lexeme.push_back(advance());
                m_tokens.push_back(t);
            } else {
                //gestione &
            }
        }
        else if(c == '|') {
            if(peek(1) == '|') {
                Token t = createToken(TokenType::LogicalOr);
                t.lexeme.push_back(advance());
                m_tokens.push_back(t);
            } else {
                //gestione |
            }
        }
        else if(c == '>') {
            if(peek(1) == '=') {
                Token t = createToken(TokenType::GreaterEqual);
                t.lexeme.push_back(advance());
                m_tokens.push_back(t);
            } else {
                Token t = createToken(TokenType::Greater);
                m_tokens.push_back(t);
            }
        }
        else if(c == '<') {
            if(peek(1) == '=') {
                Token t = createToken(TokenType::LessEqual);
                t.lexeme.push_back(advance());
                m_tokens.push_back(t);
            } else {
                Token t = createToken(TokenType::Less);
                m_tokens.push_back(t);
            }
        }
        else if(c == '(') {
            Token tlparen = createToken(TokenType::LParen);
            m_tokens.push_back(tlparen);
        }
        else if(c == ')') {
            Token trparen = createToken(TokenType::RParen);
            m_tokens.push_back(trparen);
        }
        else if(c == '[') {
            Token tlbracket = createToken(TokenType::LBracket);
            m_tokens.push_back(tlbracket);
        }
        else if(c == ']') {
            Token trbracket = createToken(TokenType::RBracket);
            m_tokens.push_back(trbracket);
        }
        else if(c == '{') {
            Token tlbrace = createToken(TokenType::LBrace);
            m_tokens.push_back(tlbrace);
        }
        else if(c == '}') {
            Token trbrace = createToken(TokenType::RBrace);
            m_tokens.push_back(trbrace);
        }
        else if(c == ';') {
            Token tsemi = createToken(TokenType::Semicolon);
            m_tokens.push_back(tsemi);
        }
        else if(c == ',') {
            Token t = createToken(TokenType::Comma);
            m_tokens.push_back(t);
        }
        else
        {
            Token unknown = createToken(TokenType::Unknown);
            m_tokens.push_back(unknown);

            errorLog->addError("Carattere non riconosciuto. char: " + c, currentTextPos);
        }
    }

    Token eof;
    eof.type = TokenType::EndOfFile;
    eof.position = currentTextPos;
    m_tokens.push_back(eof);

    return m_tokens;
}

/*
 * Emette i tokens raccolti in output a console
 */

void Lexer::printTokens()
{
    std::cout << "\nProgram Tokens:\n" << std::endl;
    for(Token& t : m_tokens){
        std::cout << typeToString(t.type) << std::endl;
    }
}

/*
 * Genera un token basandosi su un tipo specifico
 */

Token Lexer::createToken(TokenType type) {
    Token t;
    t.type = type;
    t.position = currentTextPos;
    t.lexeme = advance();
    return t;
}

/*
 * Analizza i caratteri futuri nel buffer che sta vanendo analizzato, scorrendo di un numero
 * assegnato di posizioni.
 */

char Lexer::peek(int offset) const
{
    int position = indexPos + offset;
    if(isAtEnd(position)) return char('\0');
    return buffer.at(position);
}

/*
 * Consuma il carattere corrente, aggiornando il buffer, l'indice e la posizione espressa in righe-colonne.
 */

char Lexer::advance() {
    if(isAtEnd()) return '\0';

    char r = buffer.at(indexPos);

    if(r == '\n') {
        currentTextPos.line++;
        currentTextPos.column = 1;
    }
    else currentTextPos.column++;

    indexPos++;
    return r;
}

/*
 * Queste due versioni della stessa funzione controllano se il buffer è terminato, in base alla
 * posizione dell'indice oppure ad una assegnata
 */

bool Lexer::isAtEnd() const {
    return (indexPos >= buffer.size());
}

bool Lexer::isAtEnd(int pos) const {
    return (pos >= buffer.size());
}

/*
 * Queste due funzioni sono utiliti necessarie per rendere il codice leggibile.
 * Si occupano del controllo con cctype di un carattere.
 */

bool Lexer::isDigit(const char& c) const {
    return std::isdigit(static_cast<unsigned char>(c));
}

bool Lexer::isAlpha(const char& c) const {
    return std::isalpha(static_cast<unsigned char>(c));
}

/*
 * Questa funzione esegue l'analisi dei caratteri a partire da dei numeri, delimitando dei token
 * che corrispondono a numeri letterali.
 */

Token Lexer::scanNumber()
{
    Token token;
    token.position = currentTextPos;

    std::string number; //numero contenuto al termine dello scan

    // Lettura delle cifre
    while((!isAtEnd()) && isDigit(peek())) {
        number.push_back(advance());
    }

    // Controllo isDouble
    if((peek() == '.') && isDigit(peek(1))) {
        //consuma il punto
        number.push_back(advance());

        // Lettura dei decimali
        while((!isAtEnd()) && isDigit(peek())) {
            number.push_back(advance());
        }

        // Numero double
        token.type = TokenType::DoubleLiteral;
        token.numericValue = std::stod(number);
        token.lexeme = number;

        return token;
    }

    // Numero integer
    token.type = TokenType::IntegerLiteral;
    token.numericValue = std::stoi(number);
    token.lexeme = number;

    return token;
}

/*
 * Questa funzione analizza i caratteri a partire da lettere, per delimitare degli identificatori.
 * Una volta delimitato un identificatore, controlla anche se corrisponde ad una keyword, per
 * classificarla come tale.
 */

Token Lexer::scanIdentifier() {
    Token token;
    token.position = currentTextPos;

    std::string identifier; //testo dell'identificatore

    //controllo lettere
    while(!isAtEnd() && (isDigit(peek()) || isAlpha(peek()) || peek() == '_')) {
        char l = advance();
        identifier.push_back(l);
    }

    //check for keyword
    if(keywords.contains(identifier)) {
        token.type = keywords.at(identifier); //tipo corrispondente alla keyword
        token.lexeme = identifier;
        return token;
    }

    token.type = TokenType::Identifier;
    token.lexeme = identifier;

    return token;
}

Token Lexer::scanString() {

    std::string stringa;
    TextPosition pos = currentTextPos;

    advance(); //consuma "

    while(!isAtEnd() && (peek() != '"')) {

        //escape sequence
        if(peek() == '\\') {
            advance(); //consuma escape

            if (isAtEnd()) {
                // backslash a fine input, senza carattere successivo
                errorLog->addError("invalid escape sequence at end of input", currentTextPos);
                Token errorTok;
                errorTok.type = TokenType::Unknown;
                errorTok.position = pos;
                return errorTok;
            }

            char escaped = advance();

            if (escaped == 'n') stringa.push_back('\n');
            else if (escaped == '"') stringa.push_back('"');
            else if (escaped == '\\') stringa.push_back('\\');
            else {
                errorLog->addError("invalid escape sequence: \\" + std::string(1, escaped), currentTextPos);
                Token errorTok;
                errorTok.type = TokenType::Unknown;
                errorTok.position = pos;
                return errorTok;
            }
        }
        else  stringa.push_back(advance());
    }

    Token returnTok;
    returnTok.position = pos;
    returnTok.lexeme = stringa;
    returnTok.type = TokenType::StringLiteral;

    if(isAtEnd()) {
        //stringa non terminata
        errorLog->addError("missing terminating \" character", currentTextPos);
        returnTok.type = TokenType::Unknown;
    }
    else advance(); //consuma " terminatore

    return returnTok;
}

Token Lexer::scanChar() {
    TextPosition pos = currentTextPos;
    advance(); //consuma '

    if(isAtEnd() || peek() == '\'') {
        // char non chiuso o vuoto
        errorLog->addError("empty or invalid char literal", pos);
        Token errorTok;
        errorTok.type = TokenType::Unknown;
        errorTok.position = pos;
        return errorTok;
    }

    char ch = advance(); //char dentro agli apici

    if (isAtEnd() || peek() != '\'') {
        // char non chiuso o multi-char
        errorLog->addError("char literal must contain exactly one character", pos);
        Token errorTok;
        errorTok.type = TokenType::Unknown;
        errorTok.position = pos;
        return errorTok;
    }

    advance(); // consuma '

    Token tok;
    tok.type = TokenType::CharLiteral;
    tok.lexeme = std::string(1, ch);
    tok.position = pos;
    return tok;
}

std::string Lexer::removeAll(std::string str, const std::string& sub)
{
    if (sub.empty())
        return str;

    std::size_t pos = 0;
    while ((pos = str.find(sub, pos)) != std::string::npos) {
        str.erase(pos, sub.length());
    }

    return str;
}