#ifndef RECOVERY_HANDLER_H
#define RECOVERY_HANDLER_H

#include <vector>

#include "constants/token.h"
#include "../errorlog.h" 

class RecoveryHandler
{
public:
    RecoveryHandler(ErrorLog*& log, std::vector<Token>& tokens, int& currentPos);

    void synchronize(const std::vector<TokenType>& syncPoints);
    void skipToken();
    void ghostToken(TokenType type);

private:
    std::vector<Token>& m_tokens;
    int& m_index;

    ErrorLog*& errorLog;
};

#endif //RECOVERY_HANDLER_H