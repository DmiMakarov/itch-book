#ifndef ITCH_MSG_DISPATCHER_HPP
#define ITCH_MSG_DISPATCHER_HPP


#include <utility>

#include "itch/itch_reader.hpp"
#include "itch/msg_type.hpp"
#include "itch/error_handling/format_error.hpp"

#include "itch/msg/add_order.hpp"
#include "itch/msg/add_order_mpid.hpp"
#include "itch/msg/broken_trade.hpp"
#include "itch/msg/cross_trade.hpp"
#include "itch/msg/direct_listing.hpp"
#include "itch/msg/ipo_quoting_period_update.hpp"
#include "itch/msg/luld_auction_collar.hpp"
#include "itch/msg/market_participant_position.hpp"
#include "itch/msg/mwcb_decline_level.hpp"
#include "itch/msg/mwcb_status.hpp"
#include "itch/msg/noii.hpp"
#include "itch/msg/operational_halt.hpp"
#include "itch/msg/order_cancel.hpp"
#include "itch/msg/order_delete.hpp"
#include "itch/msg/order_executed.hpp"
#include "itch/msg/order_executed_with_price.hpp"
#include "itch/msg/order_replace.hpp"
#include "itch/msg/reg_sho.hpp"
#include "itch/msg/rpii.hpp"
#include "itch/msg/stock_directory.hpp"
#include "itch/msg/stock_trading_action.hpp"
#include "itch/msg/sys_event.hpp"
#include "itch/msg/trade.hpp"

namespace itch
{
    namespace detail
    {
    template <typename T, typename Func, typename Handler>
    void dispatch_helper(Handler& handler, const Message& msg, Func process) 
    {
        if constexpr (requires { handler.on(std::declval<T>()); }) handler.on(process(msg.bytes.first<T::length>())); 
    }
    }

    template <typename Handler>
    void dispatch(const Message& msg, Handler& handler)
    {
        try
        {
            // No default: -Wswitch (part of -Wall) with -Werror fails the build if any MsgType is not handled.
            // An unknown type byte never gets here: the reader throws FormatError earlier.
            switch(msg.type)
            {
                case MsgType::SystemEvent:
                    detail::dispatch_helper<msg::SysEventMsg>(handler, msg, msg::read_sys_event_msg);
                    break;
                case MsgType::StockDirectory:
                    detail::dispatch_helper<msg::StockDirectoryMsg>(handler, msg, msg::read_stock_directory_msg);
                    break;
                case MsgType::StockTradingAction:
                    detail::dispatch_helper<msg::StockTradingActionMsg>(handler, msg, msg::read_stock_trading_action_msg);
                    break;
                case MsgType::RegSHOIndicator:
                    detail::dispatch_helper<msg::RegShoMsg>(handler, msg, msg::read_reg_sho_msg);
                    break;
                case MsgType::MarketParticipantPosition:
                    detail::dispatch_helper<msg::MarketParticipantPositionMsg>(handler, msg, msg::read_market_participant_position_msg);
                    break;
                case MsgType::MWCBDeclineLevel:
                    detail::dispatch_helper<msg::MwcbDeclineLevelMsg>(handler, msg, msg::read_mwcb_decline_level_msg);
                    break;
                case MsgType::MWCBStatus:
                    detail::dispatch_helper<msg::MwcbStatusMsg>(handler, msg, msg::read_mwcb_status_msg);
                    break;
                case MsgType::IPOQuotingPeriodUpdate:
                    detail::dispatch_helper<msg::IpoQuotingPeriodUpdateMsg>(handler, msg, msg::read_ipo_quoting_period_update_msg);
                    break;
                case MsgType::LULDAuctionCollar:
                    detail::dispatch_helper<msg::LuldAuctionCollarMsg>(handler, msg, msg::read_luld_auction_collar_msg);
                    break;
                case MsgType::OperationalHalt:
                    detail::dispatch_helper<msg::OperationalHaltMsg>(handler, msg, msg::read_operational_halt_msg);
                    break;
                case MsgType::AddOrder:
                    detail::dispatch_helper<msg::AddOrderMsg>(handler, msg, msg::read_add_order_msg);
                    break;
                case MsgType::AddOrderMPID:
                    detail::dispatch_helper<msg::AddOrderMpidMsg>(handler, msg, msg::read_add_order_mpid_msg);
                    break;
                case MsgType::OrderExecuted:
                    detail::dispatch_helper<msg::OrderExecutedMsg>(handler, msg, msg::read_order_executed_msg);
                    break;
                case MsgType::OrderExecutedWithPrice:
                    detail::dispatch_helper<msg::OrderExecutedWithPriceMsg>(handler, msg, msg::read_order_executed_with_price_msg);
                    break;
                case MsgType::OrderCancel:
                    detail::dispatch_helper<msg::OrderCancelMsg>(handler, msg, msg::read_order_cancel_msg);
                    break;
                case MsgType::OrderDelete:
                    detail::dispatch_helper<msg::OrderDeleteMsg>(handler, msg, msg::read_order_delete_msg);
                    break;
                case MsgType::OrderReplace:
                    detail::dispatch_helper<msg::OrderReplaceMsg>(handler, msg, msg::read_order_replace_msg);
                    break;
                case MsgType::Trade:
                    detail::dispatch_helper<msg::TradeMsg>(handler, msg, msg::read_trade_msg);
                    break;
                case MsgType::CrossTrade:
                    detail::dispatch_helper<msg::CrossTradeMsg>(handler, msg, msg::read_cross_trade_msg);
                    break;
                case MsgType::BrokenTrade:
                    detail::dispatch_helper<msg::BrokenTradeMsg>(handler, msg, msg::read_broken_trade_msg);
                    break;
                case MsgType::NetOrderImbalanceIndicator:
                    detail::dispatch_helper<msg::NoiiMsg>(handler, msg, msg::read_noii_msg);
                    break;
                case MsgType::RetailPriceImprovementIndicator:
                    detail::dispatch_helper<msg::RpiiMsg>(handler, msg, msg::read_rpii_msg);
                    break;
                case MsgType::DirectListingCapitalRaisePrice:
                    detail::dispatch_helper<msg::DirectListingMsg>(handler, msg, msg::read_direct_listing_msg);
                    break;
            }
        }
        catch (const FormatError& e)
        {
            throw FormatError(e.what(), msg.offset);
        }
    }
}

#endif //ITCH_MSG_DISPATCHER_HPP