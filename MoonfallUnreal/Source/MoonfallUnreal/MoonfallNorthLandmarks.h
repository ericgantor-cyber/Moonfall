#pragma once
// Static landmark silhouettes for the nine province greybox hubs.
// Nonblocking visual prototypes, NOT finished biome art, hazards or NPCs.
// Every province gets a pair of landmark forms for distinct silhouettes.
#include "MoonfallNorthContinentLayout.h"
namespace MoonfallNorth {
struct FLandmark {
 int Province;
 int OffsetXcm;
 int OffsetYcm;
 int HalfWidthCm;
 int HeightCm;
};
inline constexpr int LandmarksPerProvince=2;
inline constexpr int LandmarkCount=ZoneCount*LandmarksPerProvince;
inline constexpr FLandmark Landmarks[LandmarkCount] = {
 {0,-2200, 2100,320,650}, {0, 2300,-2100,450,380}, // ice spires, stranded hull
 {1,-2300, 2100,260,1100},{1, 2400,-2200,340,900}, // giant forest silhouettes
 {2,-2300, 2200,520,500}, {2, 2200,-2100,480,400}, // glacial boulders
 {3,-2300, 2100,360,900}, {3, 2300,-2200,400,750}, // necropolis obelisks
 {4,-2300, 2100,520,1250},{4, 2300,-2100,420,1000},// living basin columns
 {5,-2200, 2100,500,1400},{5, 2300,-2100,420,1300},// mountain pillars
 {6,-2300, 2100,520,950}, {6, 2300,-2100,480,1150},// ruined city towers
 {7,-2300, 2100,350,1300},{7, 2300,-2100,320,1000},// star crystals
 {8,-2300, 2100,480,1600},{8, 2300,-2100,400,1250}// eclipse monoliths
};
constexpr bool ValidLandmarks() {
 if (LandmarkCount!=18) return false;
 for(int Province=0;Province<ZoneCount;++Province) {
  for(int N=0;N<LandmarksPerProvince;++N) {
   const auto& L=Landmarks[Province*LandmarksPerProvince+N];
   if(L.Province!=Province || L.HalfWidthCm<200 || L.HalfWidthCm>600
     || L.HeightCm<300 || L.HeightCm>2000) return false;
   const int OffsetX=L.OffsetXcm<0?-L.OffsetXcm:L.OffsetXcm;
   const int OffsetY=L.OffsetYcm<0?-L.OffsetYcm:L.OffsetYcm;
   // Outside plaza footprint; never collision-block the main hub.
   if(OffsetX<ProvincePadHalfSizeCm+L.HalfWidthCm
     || OffsetY<ProvincePadHalfSizeCm+L.HalfWidthCm) return false;
  }
 }
 return true;
}
static_assert(ValidLandmarks(),"Nine biomes require safe, distinct landmarks");
} // namespace MoonfallNorth
