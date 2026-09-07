#ifndef TEMPLATE_STACK_H
#define TEMPLATE_STACK_H

#include <vector>
#include <unordered_map>
#include <optional>

template<typename A_, typename B_>

class Stack 
{
public:
    virtual ~Stack() = default;

    virtual void declareSymbol(const A_& first, const B_& second) =0;
    virtual std::optional<B_> lookupSymbol(const A_& key) const =0;

    virtual void push() =0;
    virtual void pop() =0;

protected:
    std::vector<std::unordered_map<A_, B_>> m_stack;
};


#endif //TEMPLATE_STACK_H