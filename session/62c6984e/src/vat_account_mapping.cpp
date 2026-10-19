#include "vat_account_mapping.hpp"

#include <algorithm>
#include <array>

namespace cratchit {

namespace {

constexpr int debit = +1;
constexpr int credit = -1;

// Only accounts whose box is known are listed. A 26xx account that is missing here makes the
// VAT calculation fail rather than silently leave its amount out.
constexpr std::array<VatAccountRule, 33> rules{{
    // Sales, taxable base (?) only the accounts seen in our books so far
    {3001, 5, credit},   // Försäljning inom Sverige, 25 % moms
    {3002, 5, credit},   // Försäljning inom Sverige, 12 % moms
    {3003, 5, credit},   // Försäljning inom Sverige, 6 % moms
    {3308, 39, credit},  // Försäljning tjänster till annat EU-land

    // Output VAT 25 %
    {2610, 10, credit},  // Utgående moms, 25 %
    {2611, 10, credit},  // Utgående moms på försäljning inom Sverige, 25 %
    {2612, 10, credit},  // Utgående moms på egna uttag, 25 %
    {2613, 10, credit},  // Utgående moms för uthyrning, 25 %
    {2614, 30, credit},  // Utgående moms omvänd skattskyldighet, 25 %
    {2615, 60, credit},  // Utgående moms import av varor, 25 %
    {2617, 30, credit},  // Utgående moms omvänd skattskyldighet i Sverige, 25 %

    // Output VAT 12 %
    {2620, 11, credit},
    {2621, 11, credit},
    {2622, 11, credit},
    {2623, 11, credit},
    {2624, 31, credit},
    {2625, 61, credit},
    {2627, 31, credit},

    // Output VAT 6 %
    {2630, 12, credit},
    {2631, 12, credit},
    {2632, 12, credit},
    {2633, 12, credit},
    {2634, 32, credit},
    {2635, 62, credit},
    {2637, 32, credit},

    // Input VAT (2648 Vilande ingående moms is not deductible yet and deliberately not listed)
    {2640, 48, debit},  // Ingående moms
    {2641, 48, debit},  // Debiterad ingående moms
    {2642, 48, debit},  // Debiterad ingående moms i anslutning till frivillig skattskyldighet
    {2645, 48, debit},  // Beräknad ingående moms på förvärv från utlandet
    {2646, 48, debit},  // Ingående moms på uthyrning
    {2647, 48, debit},  // Ingående moms omvänd skattskyldighet varor och tjänster i Sverige

    // Fortnox base accounts (not BAS), named "(Momsrapport ruta NN)" in our chart of accounts
    {9021, 20, debit},  // Varuvärde inköp annat EG-land (ruta 20)
    {9022, 50, debit},  // Beskattningsunderlag vid import (ruta 50)
}};

} // namespace

std::span<VatAccountRule const> default_vat_account_rules() { return rules; }

VatAccountRule const* find_vat_account_rule(AccountNumber account) {
  auto const it = std::ranges::find(rules, account, &VatAccountRule::m_account);
  return it == rules.end() ? nullptr : &*it;
}

bool requires_vat_account_rule(AccountNumber account) {
  return account >= 2600 && account <= 2699 && account != 2650 && account != 2651;
}

} // namespace cratchit
