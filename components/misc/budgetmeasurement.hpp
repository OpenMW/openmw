#ifndef OPENMW_COMPONENTS_MISC_BUDGETMEASUREMENT_H
#define OPENMW_COMPONENTS_MISC_BUDGETMEASUREMENT_H


namespace Misc
{

class BudgetMeasurement
{
    std::array<float, 4> mBudgetHistory;
    std::array<unsigned int, 4> mBudgetStepCount;

public:
    BudgetMeasurement(const float default_expense)
    {
        mBudgetHistory = {default_expense, default_expense, default_expense, default_expense};
        mBudgetStepCount = {1, 1, 1, 1};
    }

    void reset(const float default_expense)
    {
        mBudgetHistory = {default_expense, default_expense, default_expense, default_expense};
        mBudgetStepCount = {1, 1, 1, 1};
    }

    void update(double delta, unsigned int stepCount, size_t cursor)
    {
        mBudgetHistory[cursor%4] = delta;
        mBudgetStepCount[cursor%4] = stepCount;
    }

    double get() const
    {
        float sum = (mBudgetHistory[0] + mBudgetHistory[1] + mBudgetHistory[2] + mBudgetHistory[3]);
        unsigned int stepCountSum = (mBudgetStepCount[0] + mBudgetStepCount[1] + mBudgetStepCount[2] + mBudgetStepCount[3]);
        return sum/float(stepCountSum);
    }
};

}

#endif
