#include "MoonfallSetBonusThresholds.h"
#include <cassert>
int main() {
 using namespace MoonfallSetBonusThresholds;
 for (int n = 0; n <= 3; ++n) {
  assert(Lost(n,n) == 0 && Gained(n,n) == 0);
 }
 assert(Lost(3,2) == 3 && Lost(2,1) == 2);
 assert(Lost(3,1) == 3 && Lost(1,0) == 0);
 assert(Lost(2,0) == 2 && Lost(1,2) == 0);
 assert(Gained(1,2) == 2 && Gained(2,3) == 3);
 assert(Gained(0,3) == 3 && Gained(0,2) == 2);
 assert(Gained(3,2) == 0 && Gained(1,1) == 0);
 // Nine-slot sets may have 4-9 matching pieces; replacing one should not
 // invent a lost three-piece bonus while three or more still remain.
 for (int before = 4; before <= 9; ++before) {
  assert(Lost(before, before - 1) == 0);
  assert(Gained(before - 1, before) == 0);
 }
 assert(Lost(3, 2) == 3);
 assert(Gained(2, 3) == 3);
 assert(Lost(9, 3) == 0);
 assert(Gained(3, 9) == 0);

 for (int before = 0; before <= 3; ++before) {
  for (int after = 0; after <= 3; ++after) {
   assert((Lost(before, after) > 0) == (after < before && before >= 2 &&
      ((before >= 3 && after < 3) || (before >= 2 && after < 2))));
   assert((Gained(before, after) > 0) == (after > before &&
      ((before < 2 && after >= 2) || (before < 3 && after >= 3))));
   assert(!(Lost(before,after) && Gained(before,after)));
  }
 }
}
