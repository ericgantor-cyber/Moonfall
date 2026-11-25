#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallGuardianFootprint.h"
#include <cassert>
#include <limits>
using namespace MoonfallGuardianFootprint;
int main(){
 const float NaN=std::numeric_limits<float>::quiet_NaN();
 for(int t=0;t<4;++t) {
  const Shape shape=static_cast<Shape>(t);
  assert(Name(shape)[0]!='\0');
  assert(!Hit(shape,0,0,0,0,1,0,NaN,0,100));
  assert(!Hit(shape,0,0,0,0,1,0,0,0,-1));
 }
 assert(!Hit(Shape::MoonboundRing,0,0,0,0,1,0,0,0,100));
 assert(Hit(Shape::MoonboundRing,0,0,0,0,1,0,60,0,100));
 assert(!Hit(Shape::MoonboundRing,0,0,0,0,1,0,120,0,100));
 assert(Hit(Shape::EmberCharge,0,0,0,0,1,0,90,4,100));
 assert(!Hit(Shape::EmberCharge,0,0,0,0,1,0,-90,4,100));
 assert(!Hit(Shape::EmberCharge,0,0,0,0,1,0,90,50,100));
 assert(Hit(Shape::VenomPool,0,0,100,20,1,0,100,20,100));
 assert(!Hit(Shape::VenomPool,0,0,100,20,1,0,0,0,100));
 assert(Hit(Shape::EclipseCross,0,0,0,0,1,0,80,4,100));
 assert(Hit(Shape::EclipseCross,0,0,0,0,1,0,4,80,100));
 assert(!Hit(Shape::EclipseCross,0,0,0,0,1,0,60,60,100));
 return 0;
}
