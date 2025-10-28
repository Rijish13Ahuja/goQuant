#ifndef FEE_CALCULATOR_HPP
#define FEE_CALCULATOR_HPP

#include "core/trade.hpp"

namespace GoQuant
{

    struct FeeStructure
    {
        double maker_fee;
        double taker_fee;

        FeeStructure(double maker = 0.001, double taker = 0.002)
            : maker_fee(maker), taker_fee(taker) {}
    };

    struct FeeCalculation
    {
        double maker_fee;
        double taker_fee;
        double total_fee;
        double net_amount;

        FeeCalculation() : maker_fee(0.0), taker_fee(0.0), total_fee(0.0), net_amount(0.0) {}
    };

    class FeeCalculator
    {
    public:
        FeeCalculator(const FeeStructure &structure = FeeStructure());

        FeeCalculation calculate_fees(const Trade &trade, double notional_value) const;
        void set_fee_structure(const FeeStructure &structure);
        FeeStructure get_fee_structure() const;

    private:
        FeeStructure fee_structure_;
    };

}

#endif