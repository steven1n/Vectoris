// Negative Compile Probe: Incompatible Dimension Addition Rejection
// Expected compile failure: Adding quantities of different dimensions (Length + Time) must be rejected.
#include "Vectoris/Numerics/Units/BaseUnits/Length.h"
#include "Vectoris/Numerics/Units/BaseUnits/Time.h"

int main() {
    using namespace vectoris::numerics::Units;
    Meter len(5.0);
    Second dur(2.0);
    // Invalid dimensional addition
    auto invalid = len + dur;
    (void)invalid;
    return 0;
}
