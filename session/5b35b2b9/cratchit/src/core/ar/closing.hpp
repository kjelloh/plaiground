#pragma once

#include "core/ar/statement.hpp"
#include "core/date.hpp"

#include <string>
#include <vector>

namespace cratchit::ar {

// The closing entries BAS asks for at the balance day that the books of a
// fiscal year lack, one note each:
// - Moms (2610-2659) netting to a receivable: move it to 1650 Momsfordran.
// - Skatteskulder (2500-2599) netting to a receivable: move it to 1640 Skattefordringar.
// - 1630 Skattekonto with a credit balance: move it to 2850.
// - Redovisningsmedel (1990-1999) not matched by Avräkning för annans räkning (2830-2839).
// - 2999 OBS-konto not zero.
std::vector<std::string> check_closing(Balances const& balances, Date balance_day);

} // namespace cratchit::ar
