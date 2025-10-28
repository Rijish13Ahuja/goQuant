#include "fees/fee_calculator.hpp"

namespace GoQuant
{

    FeeCalculator::FeeCalculator(const FeeStructure &structure)
        : fee_structure_(structure) {}

    FeeCalculation FeeCalculator::calculate_fees(const Trade &trade, double notional_value) const
    {
        FeeCalculation calc;

        double fee_rate = (trade.aggressor_side == "BUY" || trade.aggressor_side == "SELL")
                              ? fee_structure_.taker_fee
                              : fee_structure_.maker_fee;

        calc.total_fee = notional_value * fee_rate;
        calc.net_amount = notional_value - calc.total_fee;

        if (trade.aggressor_side == "BUY" || trade.aggressor_side == "SELL")
        {
            calc.taker_fee = calc.total_fee;
        }
        else
        {
            calc.maker_fee = calc.total_fee;
        }

        return calc;
    }

    void FeeCalculator::set_fee_structure(const FeeStructure &structure)
    {
        fee_structure_ = structure;
    }

    FeeStructure FeeCalculator::get_fee_structure() const
    {
        return fee_structure_;
    }

}