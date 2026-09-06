#include "domain/transaction.hpp"

namespace pql {

std::optional<Transaction> Transaction::create(PortfolioId portfolio, const Trade& trade,
                                               Money fees) {
    if (fees.value() < 0.0) {
        return std::nullopt;
    }
    return Transaction{portfolio, trade, fees};
}

}  // namespace pql
