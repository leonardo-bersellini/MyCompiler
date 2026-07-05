#include "lexer.h"

#include <QDebug>

#include "keywords.h"

Lexer::Lexer() {}

/*
 * Punto di entrata dell'analisi lessicale.
 * Questa funzione analizza una stringa assegnata dividendola in tokens secondo la grammatica del
 * linguaggio. Per dividere i caratteri in token, analizza i singoli caratteri per richiamare funzioni
 * di scan ed estrapolare tutti i caratteri che andranno a formare i tokens.
 */

QList<Token> Lexer::analiseString(const QString &string, ErrorLog &_errorLog)
{
    this->errorLog = &_errorLog;

    QList<Token> tokens;

    buffer = string;

    indexPos = 0;
    currentTextPos.column = 0;
    currentTextPos.line = 0;

    buffer.remove("\r");
    buffer.remove("\t");

    while(!isAtEnd())
    {
        QChar c = peek();

        if(c == ' ') {
            advance();
        }
        else if(c == '\n') {
            advance();
        }
        else if(c.isDigit())
        {
            Token num = scanNumber();
            tokens.append(num);
        }
        else if(c.isLetter())
        {
            Token identifier = scanIdentifier();
            tokens.append(identifier);
        }
        else if(c == '"')
        {
            Token str = scanString();
            tokens.append(str);
        }
        else if(c == '\'')
        {
            Token ch = scanChar();
            tokens.append(ch);
        }
        else if(c == '+') {
            Token tplus = createToken(TokenType::Plus);
            tokens.append(tplus);
        }
        else if(c == '-') {
            Token tmin = createToken(TokenType::Minus);
            tokens.append(tmin);
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
                tokens.append(tdiv);
            }
        }
        else if(c == '*') {
            Token tstar = createToken(TokenType::Star);
            tokens.append(tstar);
        }
        else if(c == '=') {
            if(peek(1) == '=') {
                Token t = createToken(TokenType::EqualEqual);
                t.lexeme.append(advance());
                tokens.append(t);
            } else {
                Token t = createToken(TokenType::Equal);
                tokens.append(t);
            }
        }
        else if(c == '!') {
            if(peek(1) == '=') {
                Token t = createToken((TokenType::NotEqual));
                t.lexeme.append(advance());
                tokens.append(t);
            } else {
                Token t = createToken(TokenType::LogicalNot);
                tokens.append(t);
            }
        }
        else if(c == '&') {
            if(peek(1) == '&') {
                Token t = createToken(TokenType::LogicalAnd);
                t.lexeme.append(advance());
                tokens.append(t);
            } else {
                //gestione &
            }
        }
        else if(c == '|') {
            if(peek(1) == '|') {
                Token t = createToken(TokenType::LogicalOr);
                t.lexeme.append(advance());
                tokens.append(t);
            } else {
                //gestione |
            }
        }
        else if(c == '>') {
            if(peek(1) == '=') {
                Token t = createToken(TokenType::GreaterEqual);
                t.lexeme.append(advance());
                tokens.append(t);
            } else {
                Token t = createToken(TokenType::Greater);
                tokens.append(t);
            }
        }
        else if(c == '<') {
            if(peek(1) == '=') {
                Token t = createToken(TokenType::LessEqual);
                t.lexeme.append(advance());
                tokens.append(t);
            } else {
                Token t = createToken(TokenType::Less);
                tokens.append(t);
            }
        }
        else if(c == '(') {
            Token tlparen = createToken(TokenType::LParen);
            tokens.append(tlparen);
        }
        else if(c == ')') {
            Token trparen = createToken(TokenType::RParen);
            tokens.append(trparen);
        }
        else if(c == '[') {
            Token tlbracket = createToken(TokenType::LBracket);
            tokens.append(tlbracket);
        }
        else if(c == ']') {
            Token trbracket = createToken(TokenType::RBracket);
            tokens.append(trbracket);
        }
        else if(c == '{') {
            Token tlbrace = createToken(TokenType::LBrace);
            tokens.append(tlbrace);
        }
        else if(c == '}') {
            Token trbrace = createToken(TokenType::RBrace);
            tokens.append(trbrace);
        }
        else if(c == ';') {
            Token tsemi = createToken(TokenType::Semicolon);
            tokens.append(tsemi);
        }
        else if(c == ',') {
            Token t = createToken(TokenType::Comma);
            tokens.append(t);
        }
        else
        {
            Token unknown = createToken(TokenType::Unknown);
            tokens.append(unknown);

            errorLog->addError(QString("Carattere non riconosciuto. char: ").append(c), currentTextPos);
        }
    }

    Token eof;
    eof.type = TokenType::EndOfFile;
    eof.position = currentTextPos;
    tokens.append(eof);

    for(Token& t : tokens){
        qDebug() << typeToString(t.type);
    }

    return tokens;
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

QChar Lexer::peek(int offset) const
{
    int position = indexPos + offset;
    if(isAtEnd(position)) return QChar('\0');
    return buffer.at(position);
}

/*
 * Consuma il carattere corrente, aggiornando il buffer, l'indice e la posizione espressa in righe-colonne.
 */

QChar Lexer::advance() {
    if(isAtEnd()) return '\0';

    QChar r = buffer.at(indexPos);

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
 * Questa funzione esegue l'analisi dei caratteri a partire da dei numeri, delimitando dei token
 * che corrispondono a numeri letterali.
 */

Token Lexer::scanNumber()
{
    Token token;
    token.position = currentTextPos;

    QString number; //numero contenuto al termine dello scan

    // Lettura delle cifre
    while((!isAtEnd()) && peek().isDigit()) {
        number.append(advance());
    }

    // Controllo isDouble
    if((peek() == '.') && (peek(1).isDigit())) {
        //consuma il punto
        number.append(advance());

        // Lettura dei decimali
        while((!isAtEnd()) && peek().isDigit()) {
            number.append(advance());
        }

        // Numero double
        token.type = TokenType::DoubleLiteral;
        token.numericValue = number.toDouble();
        token.lexeme = number;

        return token;
    }

    // Numero integer
    token.type = TokenType::IntegerLiteral;
    token.numericValue =number.toInt();
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

    QString identifier; //testo dell'identificatore

    //controllo lettere
    while((!isAtEnd()) && (peek().isLetterOrNumber() || peek() == '_')) {
        QChar l = advance();
        identifier.append(l);
    }

    //check for keyword
    if(keywords.contains(identifier)) {
        token.type = keywords.value(identifier); //tipo corrispondente alla keyword
        token.lexeme = identifier;
        return token;
    }

    token.type = TokenType::Identifier;
    token.lexeme = identifier;

    return token;
}

Token Lexer::scanString() {

    QString stringa;
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

            QChar escaped = advance();

            if (escaped == 'n') stringa.append('\n');
            else if (escaped == '"') stringa.append('"');
            else if (escaped == '\\') stringa.append('\\');
            else {
                errorLog->addError("invalid escape sequence: \\" + QString(escaped), currentTextPos);
                Token errorTok;
                errorTok.type = TokenType::Unknown;
                errorTok.position = pos;
                return errorTok;
            }
        }
        else  stringa.append(advance());
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

    QChar ch = advance(); //char dentro agli apici

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
    tok.lexeme = QString(ch);
    tok.position = pos;
    return tok;
}