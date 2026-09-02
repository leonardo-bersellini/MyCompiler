#ifndef TEMPLATE_VISITOR_H
#define TEMPLATE_VISITOR_H

// variadic template: insieme indefinito di classi
template<class... Ts>

// overloaded: eredita da dall'insieme di classi, per ognuna usa l'operatore () 
// using sull'operatore è necessario per evitare errori di ambiguity con operatori con lo stesso nome
struct overloaded : Ts... { using Ts::operator()...; };

// deduction guide: questa direttiva specifica che gli argomenti dedotti (es lambda) si devono usare
// per generare un template di questo tipo
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>; 

#endif //TEMPLATE_VISITOR_H