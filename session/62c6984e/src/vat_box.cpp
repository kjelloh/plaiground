#include "vat_box.hpp"

#include <algorithm>
#include <array>

namespace cratchit {

namespace {

// Order = element order in SKV's complete example file. Do not sort by box number.
constexpr std::array<VatBoxInfo, 29> boxes{{
    {5, "ForsMomsEjAnnan", "Taxable sales not included in boxes 06, 07 or 08"},
    {6, "UttagMoms", "Taxable withdrawals"},
    {7, "UlagMargbesk", "Taxable base, profit-margin taxation"},
    {8, "HyrinkomstFriv", "Rental income, voluntary tax liability"},
    {20, "InkopVaruAnnatEg", "Purchase of goods from another EU country"},
    {21, "InkopTjanstAnnatEg", "Purchase of services from another EU country"},
    {22, "InkopTjanstUtomEg", "Purchase of services from outside the EU"},
    {23, "InkopVaruSverige", "Purchase of goods in Sweden, reverse charge"},
    {24, "InkopTjanstSverige", "Purchase of services in Sweden, reverse charge"},
    {50, "MomsUlagImport", "Taxable base on import"},
    {35, "ForsVaruAnnatEg", "Sale of goods to another EU country"},
    {36, "ForsVaruUtomEg", "Sale of goods outside the EU"},
    {37, "InkopVaruMellan3p", "Intermediary purchase, triangulation"},
    {38, "ForsVaruMellan3p", "Intermediary sale, triangulation"},
    {39, "ForsTjSkskAnnatEg", "Sale of services to taxable person in another EU country"},
    {40, "ForsTjOvrUtomEg", "Other sales of services outside Sweden"},
    {41, "ForsKopareSkskSverige", "Sales where the buyer is liable for tax in Sweden"},
    {42, "ForsOvrigt", "Other sales etc."},
    {10, "MomsUtgHog", "Output VAT 25 %"},
    {11, "MomsUtgMedel", "Output VAT 12 %"},
    {12, "MomsUtgLag", "Output VAT 6 %"},
    {30, "MomsInkopUtgHog", "Output VAT 25 % on purchases in boxes 20-24"},
    {31, "MomsInkopUtgMedel", "Output VAT 12 % on purchases in boxes 20-24"},
    {32, "MomsInkopUtgLag", "Output VAT 6 % on purchases in boxes 20-24"},
    {60, "MomsImportUtgHog", "Output VAT 25 % on import"},
    {61, "MomsImportUtgMedel", "Output VAT 12 % on import"},
    {62, "MomsImportUtgLag", "Output VAT 6 % on import"},
    {48, "MomsIngAvdr", "Input VAT to deduct"},
    {49, "MomsBetala", "VAT to pay (+) or receive (-)"},
}};

} // namespace

std::span<VatBoxInfo const> vat_boxes() { return boxes; }

VatBoxInfo const* find_vat_box(VatBoxNumber number) {
  auto const it = std::ranges::find(boxes, number, &VatBoxInfo::m_number);
  return it == boxes.end() ? nullptr : &*it;
}

VatBoxInfo const* find_vat_box(std::string_view element) {
  auto const it = std::ranges::find(boxes, element, &VatBoxInfo::m_element);
  return it == boxes.end() ? nullptr : &*it;
}

} // namespace cratchit
