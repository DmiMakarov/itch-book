// Справочные и административные сообщения: R, H, Y, L, V, W, K, J, h.
// На каждый тип: разбор образца целиком (в том числе при компиляции) и исчерпывающая проверка
// каждого флага по списку значений из спецификации.
#include <gtest/gtest.h>

#include <string_view>

#include "msg_samples.hpp"
#include "msg_test_utils.hpp"

using namespace itch::msg;
using namespace itch::test::samples;
using itch::test::expect_flag_domain;
using itch::test::expect_format_error_at;
using itch::test::MsgBuilder;

namespace
{
    static_assert(read_stock_directory_msg(stock_directory_bytes()) == stock_directory_expected());
    static_assert(read_stock_trading_action_msg(stock_trading_action_bytes()) == stock_trading_action_expected());
    static_assert(read_reg_sho_msg(reg_sho_bytes()) == reg_sho_expected());
    static_assert(read_market_participant_position_msg(market_participant_position_bytes()) == market_participant_position_expected());
    static_assert(read_mwcb_decline_level_msg(mwcb_decline_level_bytes()) == mwcb_decline_level_expected());
    static_assert(read_mwcb_status_msg(mwcb_status_bytes()) == mwcb_status_expected());
    static_assert(read_ipo_quoting_period_update_msg(ipo_quoting_period_update_bytes()) == ipo_quoting_period_update_expected());
    static_assert(read_luld_auction_collar_msg(luld_auction_collar_bytes()) == luld_auction_collar_expected());
    static_assert(read_operational_halt_msg(operational_halt_bytes()) == operational_halt_expected());

    // Проверка многосимвольного кода: каждый код из списка спецификации принимается, остальные — FormatError
    template <std::size_t N, typename Parse, typename Get>
    void expect_alpha_codes(Parse parse, std::array<std::byte, N> bytes, std::size_t offset, std::size_t len,
                            std::initializer_list<std::string_view> valid, std::initializer_list<std::string_view> invalid,
                            Get get)
    {
        for (const auto code : valid)
        {
            SCOPED_TRACE(testing::Message() << "valid code '" << code << '\'');
            for (std::size_t i = 0; i < len; ++i)
            {
                bytes[offset + i] = static_cast<std::byte>(i < code.size() ? code[i] : ' ');
            }
            EXPECT_EQ(get(parse(bytes)).view(), code);
        }
        for (const auto code : invalid)
        {
            SCOPED_TRACE(testing::Message() << "invalid code '" << code << '\'');
            for (std::size_t i = 0; i < len; ++i)
            {
                bytes[offset + i] = static_cast<std::byte>(i < code.size() ? code[i] : ' ');
            }
            expect_format_error_at(parse, bytes, offset);
        }
    }
}

// ---------- R — Stock Directory ----------

TEST(MsgStockDirectoryTest, DecodesAllFields)
{
    EXPECT_EQ(read_stock_directory_msg(stock_directory_bytes()), stock_directory_expected());
}

TEST(MsgStockDirectoryTest, FlagDomainsMatchSpec)
{
    const auto bytes = stock_directory_bytes();
    const auto parse = read_stock_directory_msg;

    expect_flag_domain(parse, bytes, 19, "QGSNAPZV ", [](const auto& m) { return m.market_category; });
    expect_flag_domain(parse, bytes, 20, "DEQSGHJKCN ", [](const auto& m) { return m.financial_status; });
    expect_flag_domain(parse, bytes, 25, "YN", [](const auto& m) { return m.round_lots_only; });
    expect_flag_domain(parse, bytes, 26, "ABCFILNOPQRSTUVW", [](const auto& m) { return m.issue_classification; });
    expect_flag_domain(parse, bytes, 29, "PT", [](const auto& m) { return m.authenticity; });
    expect_flag_domain(parse, bytes, 30, "YN ", [](const auto& m) { return m.short_sale_threshold; });
    expect_flag_domain(parse, bytes, 31, "YN ", [](const auto& m) { return m.ipo_flag; });
    expect_flag_domain(parse, bytes, 32, "12 ", [](const auto& m) { return m.luld_reference_price_tier; });
    expect_flag_domain(parse, bytes, 33, "YN ", [](const auto& m) { return m.etp_flag; });
    expect_flag_domain(parse, bytes, 38, "YN", [](const auto& m) { return m.inverse_indicator; });
}

TEST(MsgStockDirectoryTest, IssueSubTypeCodesMatchSpec)
{
    // Приложение E — все 58 кодов
    expect_alpha_codes(read_stock_directory_msg, stock_directory_bytes(), 27, 2,
                       {"A", "AI", "B", "C", "CB", "CF", "CL", "CM", "CO", "CT", "CU", "CW", "D", "E", "EG", "EI",
                        "EM", "EN", "EU", "F", "FI", "FL", "G", "I", "IR", "IW", "IX", "J", "L", "LL", "M", "MF",
                        "ML", "MT", "N", "O", "P", "PP", "PU", "Q", "R", "RC", "RF", "RT", "RU", "S", "SC", "SI",
                        "T", "TC", "TU", "U", "V", "W", "WC", "X", "Y", "Z"},
                       {"", "ZZ", "a", "AA", " A", "H", "K"},
                       [](const auto& m) { return m.issue_sub_type; });
}

TEST(MsgStockDirectoryTest, RealisticEtp)
{
    // SPY: NYSE Arca, ETF (sub-type "I"), лот 100, tier 1
    const auto bytes = MsgBuilder<39>{'R'}
                           .put_str(11, 8, "SPY")
                           .put_char(19, 'P')
                           .put_char(20, ' ')
                           .put_uint(21, 4, 100)
                           .put_char(25, 'N')
                           .put_char(26, 'Q')
                           .put_str(27, 2, "I")
                           .put_char(29, 'P')
                           .put_char(30, 'N')
                           .put_char(31, ' ')
                           .put_char(32, '1')
                           .put_char(33, 'Y')
                           .put_uint(34, 4, 1)
                           .put_char(38, 'N')
                           .bytes();

    const auto m = read_stock_directory_msg(bytes);
    EXPECT_EQ(m.stock.view(), "SPY");
    EXPECT_EQ(m.market_category, MarketCategory::NyseArca);
    EXPECT_EQ(m.financial_status, FinancialStatus::NotAvailable);
    EXPECT_EQ(m.round_lot_size, 100u);
    EXPECT_EQ(m.issue_sub_type.view(), "I");
    EXPECT_EQ(m.etp_flag, YesNo::Yes);
    EXPECT_EQ(m.etp_leverage_factor, 1u);
}

// ---------- H — Stock Trading Action ----------

TEST(MsgStockTradingActionTest, DecodesAllFields)
{
    EXPECT_EQ(read_stock_trading_action_msg(stock_trading_action_bytes()), stock_trading_action_expected());
}

TEST(MsgStockTradingActionTest, FlagDomainsMatchSpec)
{
    expect_flag_domain(read_stock_trading_action_msg, stock_trading_action_bytes(), 19, "HPQT",
                       [](const auto& m) { return m.trading_state; });
}

TEST(MsgStockTradingActionTest, ReservedByteIsIgnored)
{
    auto bytes = stock_trading_action_bytes();
    bytes[20] = std::byte{0xEE};
    EXPECT_EQ(read_stock_trading_action_msg(bytes), stock_trading_action_expected());
}

TEST(MsgStockTradingActionTest, ReasonCodesMatchSpec)
{
    // Приложение C; "" — четыре пробела, причина не указана
    expect_alpha_codes(read_stock_trading_action_msg, stock_trading_action_bytes(), 21, 4,
                       {"T1", "T2", "T3", "T5", "T6", "T7", "T8", "T12", "H4", "H9", "H10", "H11", "O1", "LUDP",
                        "LUDS", "MWC0", "MWC1", "MWC2", "MWC3", "MWCQ", "IPO1", "IPOQ", "IPOE", "M1", "M2", "R1",
                        "R2", "R4", "R9", "C3", "C4", "C9", "C11", ""},
                       {"T4", "V1", "LUD", "ludp", " T1", "XXXX"},
                       [](const auto& m) { return m.reason; });
}

// ---------- Y — Reg SHO ----------

TEST(MsgRegShoTest, DecodesAllFields)
{
    EXPECT_EQ(read_reg_sho_msg(reg_sho_bytes()), reg_sho_expected());
}

TEST(MsgRegShoTest, FlagDomainsMatchSpec)
{
    expect_flag_domain(read_reg_sho_msg, reg_sho_bytes(), 19, "012", [](const auto& m) { return m.action; });
}

// ---------- L — Market Participant Position ----------

TEST(MsgMarketParticipantPositionTest, DecodesAllFields)
{
    EXPECT_EQ(read_market_participant_position_msg(market_participant_position_bytes()),
              market_participant_position_expected());
}

TEST(MsgMarketParticipantPositionTest, FlagDomainsMatchSpec)
{
    const auto bytes = market_participant_position_bytes();
    const auto parse = read_market_participant_position_msg;

    expect_flag_domain(parse, bytes, 23, "YN", [](const auto& m) { return m.primary_market_maker; });
    expect_flag_domain(parse, bytes, 24, "NPSRL", [](const auto& m) { return m.market_maker_mode; });
    expect_flag_domain(parse, bytes, 25, "AEWSD", [](const auto& m) { return m.market_participant_state; });
}

// ---------- V — MWCB Decline Level ----------

TEST(MsgMwcbDeclineLevelTest, DecodesAllFields)
{
    EXPECT_EQ(read_mwcb_decline_level_msg(mwcb_decline_level_bytes()), mwcb_decline_level_expected());
}

TEST(MsgMwcbDeclineLevelTest, LevelsArePrice8)
{
    // 2 830,00 пунктов S&P 500 с 8 знаками после запятой
    const auto bytes = MsgBuilder<35>{'V'}
                           .put_uint(11, 8, 283'000'000'000)
                           .put_uint(19, 8, 263'000'000'000)
                           .put_uint(27, 8, 243'000'000'000)
                           .bytes();
    const auto m = read_mwcb_decline_level_msg(bytes);
    EXPECT_DOUBLE_EQ(m.level1.to_double(), 2830.0);
    EXPECT_DOUBLE_EQ(m.level3.to_double(), 2430.0);
}

// ---------- W — MWCB Status ----------

TEST(MsgMwcbStatusTest, DecodesAllFields)
{
    EXPECT_EQ(read_mwcb_status_msg(mwcb_status_bytes()), mwcb_status_expected());
}

TEST(MsgMwcbStatusTest, FlagDomainsMatchSpec)
{
    expect_flag_domain(read_mwcb_status_msg, mwcb_status_bytes(), 11, "123",
                       [](const auto& m) { return m.breached_level; });
}

// ---------- K — IPO Quoting Period Update ----------

TEST(MsgIpoQuotingPeriodUpdateTest, DecodesAllFields)
{
    EXPECT_EQ(read_ipo_quoting_period_update_msg(ipo_quoting_period_update_bytes()),
              ipo_quoting_period_update_expected());
}

TEST(MsgIpoQuotingPeriodUpdateTest, FlagDomainsMatchSpec)
{
    expect_flag_domain(read_ipo_quoting_period_update_msg, ipo_quoting_period_update_bytes(), 23, "AC",
                       [](const auto& m) { return m.release_qualifier; });
}

// ---------- J — LULD Auction Collar ----------

TEST(MsgLuldAuctionCollarTest, DecodesAllFields)
{
    EXPECT_EQ(read_luld_auction_collar_msg(luld_auction_collar_bytes()), luld_auction_collar_expected());
}

// ---------- h — Operational Halt ----------

TEST(MsgOperationalHaltTest, DecodesAllFields)
{
    EXPECT_EQ(read_operational_halt_msg(operational_halt_bytes()), operational_halt_expected());
}

TEST(MsgOperationalHaltTest, FlagDomainsMatchSpec)
{
    const auto bytes = operational_halt_bytes();
    expect_flag_domain(read_operational_halt_msg, bytes, 19, "QBX", [](const auto& m) { return m.market_code; });
    expect_flag_domain(read_operational_halt_msg, bytes, 20, "HT", [](const auto& m) { return m.action; });
}
