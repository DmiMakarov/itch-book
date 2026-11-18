#ifndef ITCH_MSG_STOCK_DIRECTORY_HPP
#define ITCH_MSG_STOCK_DIRECTORY_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/alpha.hpp"
#include "itch/msg/alpha_validation.hpp"
#include "itch/msg/enum_validation.hpp"
#include "itch/msg/header.hpp"
#include "itch/msg/yes_no.hpp"

namespace itch::msg
{
    enum class MarketCategory: char
    {
        NasdaqGlobalSelect = 'Q',
        NasdaqGlobalMarket = 'G',
        NasdaqCapitalMarket = 'S',
        Nyse = 'N',
        NyseAmerican = 'A',
        NyseArca = 'P',
        BatsZ = 'Z',
        Iex = 'V',
        NotAvailable = ' '
    };

    inline constexpr std::array market_categories{
        MarketCategory::NasdaqGlobalSelect,
        MarketCategory::NasdaqGlobalMarket,
        MarketCategory::NasdaqCapitalMarket,
        MarketCategory::Nyse,
        MarketCategory::NyseAmerican,
        MarketCategory::NyseArca,
        MarketCategory::BatsZ,
        MarketCategory::Iex,
        MarketCategory::NotAvailable
    };

    inline constexpr std::array<bool, 256> valid_market_category = make_valid_table(market_categories);

    enum class FinancialStatus: char
    {
        Deficient = 'D',
        Delinquent = 'E',
        Bankrupt = 'Q',
        Suspended = 'S',
        DeficientBankrupt = 'G',
        DeficientDelinquent = 'H',
        DelinquentBankrupt = 'J',
        DeficientDelinquentBankrupt = 'K',
        EtpCreationsRedemptionsSuspended = 'C',
        Normal = 'N',
        NotAvailable = ' '
    };

    inline constexpr std::array financial_statuses{
        FinancialStatus::Deficient,
        FinancialStatus::Delinquent,
        FinancialStatus::Bankrupt,
        FinancialStatus::Suspended,
        FinancialStatus::DeficientBankrupt,
        FinancialStatus::DeficientDelinquent,
        FinancialStatus::DelinquentBankrupt,
        FinancialStatus::DeficientDelinquentBankrupt,
        FinancialStatus::EtpCreationsRedemptionsSuspended,
        FinancialStatus::Normal,
        FinancialStatus::NotAvailable
    };

    inline constexpr std::array<bool, 256> valid_financial_status = make_valid_table(financial_statuses);

    // Приложение D спецификации
    enum class IssueClassification: char
    {
        AmericanDepositaryShare = 'A',
        Bond = 'B',
        CommonStock = 'C',
        DepositoryReceipt = 'F',
        Rule144A = 'I',
        LimitedPartnership = 'L',
        Notes = 'N',
        OrdinaryShare = 'O',
        PreferredStock = 'P',
        OtherSecurities = 'Q',
        Right = 'R',
        SharesOfBeneficialInterest = 'S',
        ConvertibleDebenture = 'T',
        Unit = 'U',
        UnitsBenifInt = 'V',
        Warrant = 'W'
    };

    inline constexpr std::array issue_classifications{
        IssueClassification::AmericanDepositaryShare,
        IssueClassification::Bond,
        IssueClassification::CommonStock,
        IssueClassification::DepositoryReceipt,
        IssueClassification::Rule144A,
        IssueClassification::LimitedPartnership,
        IssueClassification::Notes,
        IssueClassification::OrdinaryShare,
        IssueClassification::PreferredStock,
        IssueClassification::OtherSecurities,
        IssueClassification::Right,
        IssueClassification::SharesOfBeneficialInterest,
        IssueClassification::ConvertibleDebenture,
        IssueClassification::Unit,
        IssueClassification::UnitsBenifInt,
        IssueClassification::Warrant
    };

    inline constexpr std::array<bool, 256> valid_issue_classification = make_valid_table(issue_classifications);

    // Приложение E спецификации: коды из 1–2 символов, дополненные пробелом
    inline constexpr std::array issue_sub_types{
        Alpha<2>{"A"},  Alpha<2>{"AI"}, Alpha<2>{"B"},  Alpha<2>{"C"},  Alpha<2>{"CB"}, Alpha<2>{"CF"},
        Alpha<2>{"CL"}, Alpha<2>{"CM"}, Alpha<2>{"CO"}, Alpha<2>{"CT"}, Alpha<2>{"CU"}, Alpha<2>{"CW"},
        Alpha<2>{"D"},  Alpha<2>{"E"},  Alpha<2>{"EG"}, Alpha<2>{"EI"}, Alpha<2>{"EM"}, Alpha<2>{"EN"},
        Alpha<2>{"EU"}, Alpha<2>{"F"},  Alpha<2>{"FI"}, Alpha<2>{"FL"}, Alpha<2>{"G"},  Alpha<2>{"I"},
        Alpha<2>{"IR"}, Alpha<2>{"IW"}, Alpha<2>{"IX"}, Alpha<2>{"J"},  Alpha<2>{"L"},  Alpha<2>{"LL"},
        Alpha<2>{"M"},  Alpha<2>{"MF"}, Alpha<2>{"ML"}, Alpha<2>{"MT"}, Alpha<2>{"N"},  Alpha<2>{"O"},
        Alpha<2>{"P"},  Alpha<2>{"PP"}, Alpha<2>{"PU"}, Alpha<2>{"Q"},  Alpha<2>{"R"},  Alpha<2>{"RC"},
        Alpha<2>{"RF"}, Alpha<2>{"RT"}, Alpha<2>{"RU"}, Alpha<2>{"S"},  Alpha<2>{"SC"}, Alpha<2>{"SI"},
        Alpha<2>{"T"},  Alpha<2>{"TC"}, Alpha<2>{"TU"}, Alpha<2>{"U"},  Alpha<2>{"V"},  Alpha<2>{"W"},
        Alpha<2>{"WC"}, Alpha<2>{"X"},  Alpha<2>{"Y"},  Alpha<2>{"Z"}
    };

    enum class Authenticity: char
    {
        Production = 'P',
        Test = 'T'
    };

    inline constexpr std::array authenticities{
        Authenticity::Production,
        Authenticity::Test
    };

    inline constexpr std::array<bool, 256> valid_authenticity = make_valid_table(authenticities);

    enum class LuldReferencePriceTier: char
    {
        Tier1 = '1',
        Tier2 = '2',
        NotAvailable = ' '
    };

    inline constexpr std::array luld_reference_price_tiers{
        LuldReferencePriceTier::Tier1,
        LuldReferencePriceTier::Tier2,
        LuldReferencePriceTier::NotAvailable
    };

    inline constexpr std::array<bool, 256> valid_luld_reference_price_tier = make_valid_table(luld_reference_price_tiers);

    struct StockDirectoryMsg
    {
        Header header{};
        Alpha<8> stock;
        MarketCategory market_category{};
        FinancialStatus financial_status{};
        std::uint32_t round_lot_size{};
        YesNo round_lots_only{};
        IssueClassification issue_classification{};
        Alpha<2> issue_sub_type;
        Authenticity authenticity{};
        YesNo short_sale_threshold{};
        YesNo ipo_flag{};
        LuldReferencePriceTier luld_reference_price_tier{};
        YesNo etp_flag{};
        std::uint32_t etp_leverage_factor{};
        YesNo inverse_indicator{};

        bool operator==(const StockDirectoryMsg& msg) const = default;

        static constexpr MsgType type = MsgType::StockDirectory;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr StockDirectoryMsg read_stock_directory_msg(const std::span<const std::byte, StockDirectoryMsg::length> bytes)
    {
        return StockDirectoryMsg{
            .header = read_header(bytes),
            .stock = Alpha<8>{bytes.subspan<11, 8>()},
            .market_category = read_enum<MarketCategory, 19>(bytes, valid_market_category),
            .financial_status = read_enum<FinancialStatus, 20>(bytes, valid_financial_status),
            .round_lot_size = itch::endian::be32(bytes.subspan<21, 4>()),
            .round_lots_only = read_enum<YesNo, 25>(bytes, valid_yes_no),
            .issue_classification = read_enum<IssueClassification, 26>(bytes, valid_issue_classification),
            .issue_sub_type = read_alpha_code<2, 27>(bytes, issue_sub_types),
            .authenticity = read_enum<Authenticity, 29>(bytes, valid_authenticity),
            .short_sale_threshold = read_enum<YesNo, 30>(bytes, valid_yes_no_or_na),
            .ipo_flag = read_enum<YesNo, 31>(bytes, valid_yes_no_or_na),
            .luld_reference_price_tier = read_enum<LuldReferencePriceTier, 32>(bytes, valid_luld_reference_price_tier),
            .etp_flag = read_enum<YesNo, 33>(bytes, valid_yes_no_or_na),
            .etp_leverage_factor = itch::endian::be32(bytes.subspan<34, 4>()),
            .inverse_indicator = read_enum<YesNo, 38>(bytes, valid_yes_no),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_STOCK_DIRECTORY_HPP
