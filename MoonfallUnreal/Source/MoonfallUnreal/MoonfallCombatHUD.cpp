#include "MoonfallCombatHUD.h"
#include "MoonfallTalentWeb.h"
#include "MoonfallLivingSimulation.h"
#include "MoonfallLivingWorldPolicy.h"
#include "MoonfallUnrealGameMode.h"
#include "MoonfallChronicle.h"
#include "MoonfallChronicleUI.h"
#include "MoonfallSpellBook.h"
#include "MoonfallForgeForecast.h"
#include "MoonfallGearAdvisorPolicy.h"
#include "MoonfallWorldQuest.h"
#include "MoonfallQuestXP.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallEnemy.h"
#include "MoonfallWorldNPC.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
void AMoonfallCombatHUD::DrawHUD()
{
 Super::DrawHUD();
 if (!Canvas) return;
 const AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this,0));
 if (!IsValid(Hero)) { bAdvisorScoreCached = false; return; }
 // A hidden bag/character sheet must not retain a previously computed score.
 // Reopening the bag always recomputes advice for its current contents.
 if (!Hero->bInventoryOpen || Hero->bCharacterStatsOpen) bAdvisorScoreCached = false;
 // Menus do not pause the hostile world. Indicate actual incoming HP damage
 // behind every modal, without an extra HUD message or a gameplay-time pause.
 // This drawing is constant-cost and never scans nearby actors.
 // Draw AFTER the menu contents. Drawing a tint before a nearly opaque
 // modal lets that panel hide the warning from the player.
 const auto DrawLiveMenuHitCue = [this, Hero]() {
 const bool bLiveMenuOpen = Hero->bTalentWebOpen || Hero->bOptionsOpen
     || Hero->bInventoryOpen || Hero->bAtlasOpen || Hero->bResidentContextOpen;
 if (bLiveMenuOpen && GetWorld() && FMath::IsFinite(Hero->LastRealDamageAt)) {
  const float Now=GetWorld()->GetTimeSeconds();
  const float Age=Now-Hero->LastRealDamageAt;
  if (FMath::IsFinite(Age) && Age>=0.f && Age<0.8f) {
   // Reduce Motion opts out of animation; both paths avoid strobing.
   const float Alpha=Hero->bReduceMotion ? 0.12f
       : 0.20f*(1.f-Age/0.8f);
   const FLinearColor HitRed(0.78f,0.035f,0.06f,Alpha);
   DrawRect(HitRed,0.f,0.f,Canvas->SizeX,Canvas->SizeY);
   const float Border=FMath::Min(14.f, FMath::Min(Canvas->SizeX,Canvas->SizeY)*0.03f);
   DrawRect(HitRed,0.f,0.f,Canvas->SizeX,Border);
   DrawRect(HitRed,0.f,Canvas->SizeY-Border,Canvas->SizeX,Border);
   DrawRect(HitRed,0.f,0.f,Border,Canvas->SizeY);
   DrawRect(HitRed,Canvas->SizeX-Border,0.f,Border,Canvas->SizeY);
  }
 }
 };
 // Shared, fixed-cost panel frame for all major Unreal menus.
 // Painting accents does not change mouse hit targets, layouts, or controls.
 const auto DrawModalChrome = [this](float L,float T,float W,float H,
                                      const FLinearColor& Accent) {
  if (!FMath::IsFinite(L) || !FMath::IsFinite(T)
      || !FMath::IsFinite(W) || !FMath::IsFinite(H)
      || W < 48.f || H < 48.f) return;
  const FLinearColor Edge(0.61f,0.53f,0.40f,0.90f);
  DrawRect(Accent,L,T,W,4.f);
  DrawRect(Edge,L,T,2.f,H);
  DrawRect(Edge,L+W-2.f,T,2.f,H);
  DrawRect(Edge,L,T+H-2.f,W,2.f);
  DrawRect(FLinearColor(0.59f,0.51f,0.36f,0.65f),L+12.f,T+44.f,
      FMath::Max(0.f,W-24.f),1.f);
  // Original Moonfall engraving: four forged-corner chevrons, inner
  // filigree rails and an eclipse diamond. All inside the panel border.
  // Decorations have no click targets or art/mesh dependencies.
  const float Inset=7.f, Corner=11.f;
  const FLinearColor Foil(0.66f,0.53f,0.32f,0.72f);
  DrawRect(Foil,L+Inset,T+Inset,Corner,1.f);
  DrawRect(Foil,L+Inset,T+Inset,1.f,Corner);
  DrawRect(Foil,L+W-Inset-Corner,T+Inset,Corner,1.f);
  DrawRect(Foil,L+W-Inset-1.f,T+Inset,1.f,Corner);
  DrawRect(Foil,L+Inset,T+H-Inset-1.f,Corner,1.f);
  DrawRect(Foil,L+Inset,T+H-Inset-Corner,1.f,Corner);
  DrawRect(Foil,L+W-Inset-Corner,T+H-Inset-1.f,Corner,1.f);
  DrawRect(Foil,L+W-Inset-1.f,T+H-Inset-Corner,1.f,Corner);
  if (H>=170.f && W>=300.f) {
   const float LowerY=T+H-11.f;
   Canvas->K2_DrawLine(FVector2D(L+W-30.f,LowerY),
     FVector2D(L+W-25.f,LowerY-5.f),1.f,Foil);
   Canvas->K2_DrawLine(FVector2D(L+W-25.f,LowerY-5.f),
     FVector2D(L+W-20.f,LowerY),1.f,Foil);
   Canvas->K2_DrawLine(FVector2D(L+W-20.f,LowerY),
     FVector2D(L+W-25.f,LowerY+5.f),1.f,Foil);
   Canvas->K2_DrawLine(FVector2D(L+W-25.f,LowerY+5.f),
     FVector2D(L+W-30.f,LowerY),1.f,Foil);
   // Forged relief: opposing side motifs and a second inset border, all
   // confined to the decorative margins so labels stay unobstructed.
   const FLinearColor Etch(0.38f,0.42f,0.52f,0.54f);
   DrawRect(Etch,L+5.f,T+59.f,1.f,FMath::Max(0.f,H-76.f));
   DrawRect(Etch,L+W-6.f,T+59.f,1.f,FMath::Max(0.f,H-76.f));
   const float MidY=T+H*0.5f;
   Canvas->K2_DrawLine(FVector2D(L+3.f,MidY-13.f),
     FVector2D(L+10.f,MidY),1.f,Foil);
   Canvas->K2_DrawLine(FVector2D(L+10.f,MidY),
     FVector2D(L+3.f,MidY+13.f),1.f,Foil);
   Canvas->K2_DrawLine(FVector2D(L+W-3.f,MidY-13.f),
     FVector2D(L+W-10.f,MidY),1.f,Foil);
   Canvas->K2_DrawLine(FVector2D(L+W-10.f,MidY),
     FVector2D(L+W-3.f,MidY+13.f),1.f,Foil);
  }
 };
 // Original vector sigil language: etched eclipse, ward, crown and compass.
 // A fixed 12-segment halo and up to seven lines per glyph. No textures,
 // actors, transient meshes, frame animation or new save dependencies.
 const auto DrawMoonfallSigil = [this](
    float Cx,float Cy,float Radius,const FLinearColor& Ink,int32 Type) {
  if (!Canvas || !FMath::IsFinite(Cx) || !FMath::IsFinite(Cy)
      || !FMath::IsFinite(Radius) || Radius<3.f || Radius>35.f) return;
  constexpr int32 Segments=12;
  for (int32 I=0;I<Segments;++I) {
   const float A=float(I)*2.f*PI/float(Segments);
   const float B=float(I+1)*2.f*PI/float(Segments);
   Canvas->K2_DrawLine(
     FVector2D(Cx+FMath::Cos(A)*Radius,Cy+FMath::Sin(A)*Radius),
     FVector2D(Cx+FMath::Cos(B)*Radius,Cy+FMath::Sin(B)*Radius),
     1.1f,Ink);
  }
  const auto Stroke=[this,&Ink](float X0,float Y0,float X1,float Y1) {
   Canvas->K2_DrawLine(FVector2D(X0,Y0),FVector2D(X1,Y1),1.4f,Ink);
  };
  const float R=Radius*0.72f;
  switch (Type%4) {
   case 0: // Eclipse: twin crescents cradling a central star.
    Stroke(Cx-R,Cy,Cx,Cy-R);
    Stroke(Cx,Cy-R,Cx+R,Cy);
    Stroke(Cx+R,Cy,Cx,Cy+R);
    Stroke(Cx,Cy+R,Cx-R,Cy);
    Stroke(Cx-R*0.42f,Cy,Cx+R*0.42f,Cy);
    break;
   case 1: // Wayfinder compass.
    Stroke(Cx,Cy-R,Cx,Cy+R);
    Stroke(Cx-R,Cy,Cx+R,Cy);
    Stroke(Cx-R*0.45f,Cy-R*0.45f,Cx+R*0.45f,Cy+R*0.45f);
    Stroke(Cx+R*0.45f,Cy-R*0.45f,Cx-R*0.45f,Cy+R*0.45f);
    break;
   case 2: // Protective shield rune.
    Stroke(Cx-R*0.7f,Cy-R*0.7f,Cx+R*0.7f,Cy-R*0.7f);
    Stroke(Cx-R*0.7f,Cy-R*0.7f,Cx-R*0.62f,Cy+R*0.35f);
    Stroke(Cx+R*0.7f,Cy-R*0.7f,Cx+R*0.62f,Cy+R*0.35f);
    Stroke(Cx-R*0.62f,Cy+R*0.35f,Cx,Cy+R);
    Stroke(Cx+R*0.62f,Cy+R*0.35f,Cx,Cy+R);
    Stroke(Cx,Cy-R*0.5f,Cx,Cy+R*0.6f);
    break;
   default: // Four-point talisman for the grimoire and rare equipment.
    Stroke(Cx,Cy-R,Cx+R*0.38f,Cy);
    Stroke(Cx+R*0.38f,Cy,Cx,Cy+R);
    Stroke(Cx,Cy+R,Cx-R*0.38f,Cy);
    Stroke(Cx-R*0.38f,Cy,Cx,Cy-R);
    Stroke(Cx-R,Cy,Cx+R,Cy);
    break;
  }
 };
 // Six hand-drawn school emblems tied to MoonfallSpellBook::Spells order:
 // Ember, Frost, Eclipse, Mending, Storm and Thorn. Pure fixed-cost artwork.
 const auto DrawSpellGlyph = [this](float Cx,float Cy,float R,int32 School) {
  if (!Canvas || School<0 || School>=6 || !FMath::IsFinite(Cx)
      || !FMath::IsFinite(Cy) || !FMath::IsFinite(R)
      || R<4.f || R>20.f) return;
  const FLinearColor Ink[] = {
   FLinearColor(1.f,0.55f,0.27f),FLinearColor(0.50f,0.87f,0.99f),
   FLinearColor(0.78f,0.56f,0.98f),FLinearColor(0.65f,0.98f,0.72f),
   FLinearColor(0.46f,0.74f,1.f),FLinearColor(0.58f,0.89f,0.47f)
  };
  const auto Line = [this,&Ink,School](float X1,float Y1,float X2,float Y2) {
   Canvas->K2_DrawLine(FVector2D(X1,Y1),FVector2D(X2,Y2),1.6f,Ink[School]);
  };
  DrawRect(FLinearColor(0.05f,0.07f,0.14f,0.96f),
    Cx-R-2.f,Cy-R-2.f,2.f*R+4.f,2.f*R+4.f);
  // Four narrow cardinal marks make schools recognizable in compact layouts.
  Line(Cx-R,Cy,Cx-R*0.76f,Cy);
  Line(Cx+R*0.76f,Cy,Cx+R,Cy);
  Line(Cx,Cy-R,Cx,Cy-R*0.76f);
  Line(Cx,Cy+R*0.76f,Cx,Cy+R);
  const float P=R*0.67f;
  switch (School) {
   case 0: // Ember: rising flame, three forked tongues.
    Line(Cx,Cy+P,Cx-P*0.6f,Cy-P*0.2f);
    Line(Cx-P*0.6f,Cy-P*0.2f,Cx,Cy-P);
    Line(Cx,Cy-P,Cx+P*0.72f,Cy+P*0.18f);
    Line(Cx+P*0.72f,Cy+P*0.18f,Cx,Cy+P);
    Line(Cx,Cy+P,Cx,Cy-P*0.14f);
    break;
   case 1: // Frost: six-armed flake.
    for (int32 I=0;I<6;++I) {
     const float A=2.f*PI*I/6.f;
     const float X=Cx+FMath::Cos(A)*P;
     const float Y=Cy+FMath::Sin(A)*P;
     Line(Cx,Cy,X,Y);
    }
    break;
   case 2: // Eclipse: crescent and lance.
    Line(Cx-P*0.73f,Cy-P*0.8f,Cx-P*0.25f,Cy);
    Line(Cx-P*0.25f,Cy,Cx-P*0.73f,Cy+P*0.8f);
    Line(Cx+P*0.42f,Cy-P,Cx+P*0.42f,Cy+P);
    Line(Cx+P*0.1f,Cy-P*0.42f,Cx+P*0.75f,Cy-P*0.42f);
    break;
   case 3: // Restoration: rising four-leaf bloom.
    Line(Cx,Cy-P,Cx,Cy+P);
    Line(Cx-P,Cy,Cx+P,Cy);
    Line(Cx-P*0.65f,Cy-P*0.65f,Cx+P*0.65f,Cy+P*0.65f);
    Line(Cx+P*0.65f,Cy-P*0.65f,Cx-P*0.65f,Cy+P*0.65f);
    break;
   case 4: // Storm: cracked bolt.
    Line(Cx+P*0.3f,Cy-P,Cx-P*0.55f,Cy);
    Line(Cx-P*0.55f,Cy,Cx+P*0.2f,Cy);
    Line(Cx+P*0.2f,Cy,Cx-P*0.35f,Cy+P);
    break;
   default: // Thorn: stem and twin sharpened leaves.
    Line(Cx-P*0.55f,Cy+P,Cx+P*0.25f,Cy-P);
    Line(Cx+P*0.25f,Cy-P,Cx+P*0.65f,Cy-P*0.55f);
    Line(Cx+P*0.65f,Cy-P*0.55f,Cx+P*0.1f,Cy-P*0.35f);
    Line(Cx-P*0.32f,Cy+P*0.45f,Cx-P*0.75f,Cy);
    Line(Cx-P*0.75f,Cy,Cx-P*0.07f,Cy+P*0.1f);
    break;
  }
 };
 // Real gear-slot iconography, not extra equipped slots or paper-doll art.
 // Type 0 sword; 1 armor; 2 talisman. Rarity remains the encoded item color.
 const auto DrawGearGlyph = [this](float Cx,float Cy,float R,
                                  int32 Slot,const FLinearColor& Ink) {
  if (!Canvas || Slot<0 || Slot>2 || !FMath::IsFinite(Cx)
      || !FMath::IsFinite(Cy) || !FMath::IsFinite(R)
      || R<4.f || R>19.f) return;
  const auto Line=[this,&Ink](float X0,float Y0,float X1,float Y1) {
   Canvas->K2_DrawLine(FVector2D(X0,Y0),FVector2D(X1,Y1),1.7f,Ink);
  };
  switch (Slot) {
   case 0:
    Line(Cx-R*0.18f,Cy+R*0.8f,Cx+R*0.52f,Cy-R*0.88f);
    Line(Cx+R*0.52f,Cy-R*0.88f,Cx+R*0.68f,Cy-R*0.34f);
    Line(Cx-R*0.42f,Cy+R*0.22f,Cx+R*0.23f,Cy+R*0.47f);
    Line(Cx-R*0.22f,Cy+R*0.65f,Cx-R*0.48f,Cy+R);
    break;
   case 1:
    Line(Cx-R*0.80f,Cy-R*0.84f,Cx+R*0.80f,Cy-R*0.84f);
    Line(Cx-R*0.80f,Cy-R*0.84f,Cx-R*0.72f,Cy+R*0.38f);
    Line(Cx+R*0.80f,Cy-R*0.84f,Cx+R*0.72f,Cy+R*0.38f);
    Line(Cx-R*0.72f,Cy+R*0.38f,Cx,Cy+R);
    Line(Cx+R*0.72f,Cy+R*0.38f,Cx,Cy+R);
    Line(Cx,Cy-R*0.64f,Cx,Cy+R*0.53f);
    break;
   default:
    Line(Cx,Cy-R,Cx+R*0.74f,Cy);
    Line(Cx+R*0.74f,Cy,Cx,Cy+R);
    Line(Cx,Cy+R,Cx-R*0.74f,Cy);
    Line(Cx-R*0.74f,Cy,Cx,Cy-R);
    Line(Cx-R*0.40f,Cy,Cx+R*0.40f,Cy);
    break;
  }
 };
 // Distinct original heraldry for the six ACTUAL regions in Atlas order:
 // Ashen Wilds, Moonveil Forest, Hollow Depths, Iron Foundry,
 // Frostbound Reach and Eclipse Wastes. Region IDs carry no hidden map data.
 const auto DrawRegionEmblem = [this](float X,float Y,float R,int32 Region,
                                      const FLinearColor& Color) {
  if (!Canvas || Region<0 || Region>=6 || !FMath::IsFinite(X)
      || !FMath::IsFinite(Y) || !FMath::IsFinite(R)
      || R<5.f || R>24.f) return;
  // Inner medallion frame has twelve bounded strokes and no animation.
  constexpr int32 Edges=8;
  for (int32 I=0;I<Edges;++I) {
   const float A=2.f*PI*float(I)/float(Edges)-PI*0.5f;
   const float B=2.f*PI*float(I+1)/float(Edges)-PI*0.5f;
   Canvas->K2_DrawLine(FVector2D(X+FMath::Cos(A)*R,Y+FMath::Sin(A)*R),
     FVector2D(X+FMath::Cos(B)*R,Y+FMath::Sin(B)*R),1.2f,Color);
  }
  const auto Stroke=[this,&Color](float X0,float Y0,float X1,float Y1) {
   Canvas->K2_DrawLine(FVector2D(X0,Y0),FVector2D(X1,Y1),1.7f,Color);
  };
  const float P=R*0.72f;
  switch (Region) {
   case 0: // Ashen Wilds: five-point volcanic spire.
    Stroke(X-P*0.85f,Y+P,X-P*0.30f,Y-P*0.25f);
    Stroke(X-P*0.30f,Y-P*0.25f,X,Y+P*0.12f);
    Stroke(X,Y+P*0.12f,X+P*0.28f,Y-P);
    Stroke(X+P*0.28f,Y-P,X+P*0.84f,Y+P);
    Stroke(X-P*0.85f,Y+P,X+P*0.84f,Y+P);
    break;
   case 1: // Moonveil Forest: ancient tree and twin boughs.
    Stroke(X,Y+P,X,Y-P);
    Stroke(X,Y-P*0.65f,X-P*0.74f,Y);
    Stroke(X,Y-P*0.65f,X+P*0.74f,Y);
    Stroke(X-P*0.74f,Y,X-P*0.45f,Y-P*0.57f);
    Stroke(X+P*0.74f,Y,X+P*0.45f,Y-P*0.57f);
    Stroke(X-P*0.42f,Y+P,X+P*0.42f,Y+P);
    break;
   case 2: // Hollow Depths: descending cavern strata.
    Stroke(X-P,Y-P*0.47f,X,Y-P*0.08f);
    Stroke(X,Y-P*0.08f,X+P,Y-P*0.47f);
    Stroke(X-P,Y+P*0.04f,X,Y+P*0.42f);
    Stroke(X,Y+P*0.42f,X+P,Y+P*0.04f);
    Stroke(X-P*0.42f,Y+P*0.74f,X+P*0.42f,Y+P*0.74f);
    break;
   case 3: // Iron Foundry: toothed forge anvil.
    Stroke(X-P,Y-P*0.47f,X+P,Y-P*0.47f);
    Stroke(X-P,Y-P*0.47f,X-P*0.55f,Y+P*0.13f);
    Stroke(X+P,Y-P*0.47f,X+P*0.54f,Y+P*0.13f);
    Stroke(X-P*0.55f,Y+P*0.13f,X+P*0.54f,Y+P*0.13f);
    Stroke(X,Y+P*0.13f,X,Y+P*0.85f);
    Stroke(X-P*0.53f,Y+P*0.85f,X+P*0.53f,Y+P*0.85f);
    break;
   case 4: // Frostbound Reach: six-rayed glacial star.
    for (int32 I=0;I<6;++I) {
     const float A=PI*float(I)/3.f;
     Stroke(X,Y,X+FMath::Cos(A)*P,Y+FMath::Sin(A)*P);
    }
    break;
   default: // Eclipse Wastes: split moon over a horizon.
    Stroke(X-P,Y+P*0.44f,X+P,Y+P*0.44f);
    Stroke(X-P*0.76f,Y-P*0.44f,X,Y-P);
    Stroke(X,Y-P,X+P*0.76f,Y-P*0.44f);
    Stroke(X-P*0.76f,Y-P*0.44f,X-P*0.25f,Y+P*0.20f);
    Stroke(X+P*0.76f,Y-P*0.44f,X+P*0.25f,Y+P*0.20f);
    break;
  }
 };
 // Five faction crests in the same order as the real save-backed
 // Reputation ledger. A purely decorative identity, never a reputation award.
 const auto DrawFactionCrest = [this](float X,float Y,float R,int32 Faction,
                                      const FLinearColor& Ink) {
  if (!Canvas || Faction<0 || Faction>=5 || !FMath::IsFinite(X)
      || !FMath::IsFinite(Y) || !FMath::IsFinite(R)
      || R<5.f || R>21.f) return;
  const auto Stroke=[this,&Ink](float A,float B,float C,float D) {
   Canvas->K2_DrawLine(FVector2D(A,B),FVector2D(C,D),1.7f,Ink);
  };
  const float P=R*0.72f;
  DrawRect(FLinearColor(0.07f,0.10f,0.15f,0.93f),
    X-R,Y-R,2.f*R,2.f*R);
  switch(Faction) {
   case 0: // Wanderers: converging roads / wayfinder.
    Stroke(X-P,Y+P,X,Y-P);
    Stroke(X+P,Y+P,X,Y-P);
    Stroke(X-P*0.38f,Y+P*0.25f,X+P*0.38f,Y+P*0.25f);
    Stroke(X,Y-P,X,Y-P*0.45f);
    break;
   case 1: // Verdant Circle: three branches and a seed.
    Stroke(X,Y+P,X,Y-P);
    Stroke(X,Y-P*0.25f,X-P*0.83f,Y-P*0.76f);
    Stroke(X,Y+P*0.15f,X+P*0.83f,Y-P*0.46f);
    Stroke(X-P*0.80f,Y+P*0.66f,X+P*0.80f,Y+P*0.66f);
    break;
   case 2: // Stone Hearth: protective mountain and centered flame.
    Stroke(X-P,Y+P*0.45f,X-P*0.32f,Y-P);
    Stroke(X-P*0.32f,Y-P,X+P*0.15f,Y+P*0.06f);
    Stroke(X+P*0.15f,Y+P*0.06f,X+P*0.42f,Y-P*0.51f);
    Stroke(X+P*0.42f,Y-P*0.51f,X+P,Y+P*0.45f);
    Stroke(X-P,Y+P*0.45f,X+P,Y+P*0.45f);
    break;
   case 3: // Moon Coven: opposing lunar blades.
    Stroke(X-P*0.72f,Y-P,X-P*0.13f,Y);
    Stroke(X-P*0.13f,Y,X-P*0.72f,Y+P);
    Stroke(X+P*0.72f,Y-P,X+P*0.13f,Y);
    Stroke(X+P*0.13f,Y,X+P*0.72f,Y+P);
    Stroke(X-P*0.28f,Y,X+P*0.28f,Y);
    break;
   default: // Ashen Clans: three pronged forge fire.
    Stroke(X,Y-P,X-P*0.65f,Y+P);
    Stroke(X,Y-P,X+P*0.65f,Y+P);
    Stroke(X-P*0.65f,Y+P,X+P*0.65f,Y+P);
    Stroke(X-P*0.35f,Y-P*0.35f,X,Y+P*0.35f);
    Stroke(X+P*0.35f,Y-P*0.35f,X,Y+P*0.35f);
    break;
  }
 };
 // High-contrast rail marks the focused choice independently of text color.
 const auto DrawSelectionRail = [this](float L,float T,float H) {
  if (!FMath::IsFinite(L)||!FMath::IsFinite(T)||!FMath::IsFinite(H)
      || H <= 0.f) return;
  DrawRect(FLinearColor(1.f,0.80f,0.37f,0.98f),L,T,4.f,H);
 };
 // A single cursor query serves every native Canvas menu. The hover
 // indicator is visual only and uses the EXISTING click geometry.
 float PanelPointerX=-1.f,PanelPointerY=-1.f;
 const APlayerController* PanelPC=Cast<APlayerController>(Hero->GetController());
 const bool bPanelPointerVisible=PanelPC && PanelPC->bShowMouseCursor
   && PanelPC->GetMousePosition(PanelPointerX,PanelPointerY)
   && FMath::IsFinite(PanelPointerX) && FMath::IsFinite(PanelPointerY);
 const auto DrawHoverFrame = [this,bPanelPointerVisible,PanelPointerX,PanelPointerY](
     float X,float Y,float W,float H) {
  if (!bPanelPointerVisible || !FMath::IsFinite(X) || !FMath::IsFinite(Y)
      || !FMath::IsFinite(W) || !FMath::IsFinite(H)
      || W <= 0.f || H <= 0.f || PanelPointerX < X || PanelPointerX > X+W
      || PanelPointerY < Y || PanelPointerY > Y+H) return;
  // Constant four rects, no animation, no pointer capture, no input binding.
  const FLinearColor Focus(0.97f,0.80f,0.43f,0.98f);
  DrawRect(Focus,X,Y,W,2.f);
  DrawRect(Focus,X,Y+H-2.f,W,2.f);
  DrawRect(Focus,X,Y,2.f,H);
  DrawRect(Focus,X+W-2.f,Y,2.f,H);
 };
 if (Hero->bTalentWebOpen) {
  // Original 141-node classless tree. Shared, deterministic layout is used
  // by mouse and keyboard/controller input; nothing pauses the live world.
  const auto P=MoonfallTalentWeb::ForViewport(Canvas->SizeX,Canvas->SizeY,
      Hero->TalentWebFirstTier);
  if (!P.Valid) {
   DrawText(TEXT("TALENTS | Enlarge window | K / ESC: close"),
       FLinearColor(0.90f,0.83f,0.70f),12.f,14.f,nullptr,0.8f);
   DrawLiveMenuHitCue();
   return;
  }
  const float L=float(P.Panel.X),T=float(P.Panel.Y),W=float(P.Panel.W),H=float(P.Panel.H);
  const int Branch=FMath::Clamp(Hero->TalentWebBranch,0,2);
  const int Points=Hero->GetAvailableTalentPoints();
  const bool FullRanks=true; // fixed-size rank ledger validated by save loader
  DrawRect(FLinearColor(0.035f,0.032f,0.065f,0.97f),L,T,W,H);
  DrawRect(FLinearColor(0.53f,0.34f,0.74f,1.f),L,T,W,5.f);
  DrawModalChrome(L,T,W,H,FLinearColor(0.57f,0.37f,0.73f,0.98f));
  if (W>=420.f) DrawMoonfallSigil(L+W-65.f,T+25.f,10.f,FLinearColor(0.87f,0.69f,0.98f),3);
  DrawText(TEXT("MOONFALL | CONSTELLATION WEB"),
      FLinearColor(0.95f,0.86f,0.99f),L+20.f,T+14.f,nullptr,
      Canvas->SizeX<850.f ? 0.92f : 1.20f);
  DrawText(FString::Printf(TEXT("LEVEL %d  |  TALENT POINTS: %d"),
      Hero->Level,Points),FLinearColor(0.88f,0.85f,0.71f),
      L+20.f,T+87.f,nullptr,0.82f);
  const auto Close=MoonfallTalentWeb::CloseRect(P);
  DrawRect(FLinearColor(0.36f,0.17f,0.23f,0.95f),
      float(Close.X),float(Close.Y),float(Close.W),float(Close.H));
  DrawHoverFrame(float(Close.X),float(Close.Y),float(Close.W),float(Close.H));
  DrawText(TEXT("X"),FLinearColor::White,float(Close.X+9),float(Close.Y+5),nullptr,0.84f);
  static const TCHAR* BranchNames[3]={TEXT("MIGHT"),TEXT("AGILITY"),TEXT("ARCANA")};
  static const FLinearColor BranchColors[3]={
    FLinearColor(0.83f,0.38f,0.38f),FLinearColor(0.38f,0.74f,0.60f),
    FLinearColor(0.63f,0.44f,0.91f)};
  for(int B=0;B<3;++B) {
   const auto Tab=MoonfallTalentWeb::BranchTab(P,B);
   DrawRect(B==Branch?BranchColors[B]:FLinearColor(0.13f,0.12f,0.19f,0.94f),
      float(Tab.X),float(Tab.Y),float(Tab.W),float(Tab.H));
   if (B==Branch) DrawSelectionRail(float(Tab.X),float(Tab.Y),float(Tab.H));
   if (Tab.W>=115) DrawMoonfallSigil(float(Tab.X+Tab.W-13),
     float(Tab.Y+Tab.H/2),7.f,BranchColors[B],B);
   DrawHoverFrame(float(Tab.X),float(Tab.Y),float(Tab.W),float(Tab.H));
   DrawText(FString(BranchNames[B]),B==Branch?FLinearColor::Black:FLinearColor::White,
       float(Tab.X+13.f),float(Tab.Y+6.f),nullptr,0.86f);
  }
  for(int Col=0;Col<P.VisibleTiers;++Col) {
   DrawText(FString::Printf(TEXT("TIER %d"),P.FirstTier+Col),
       FLinearColor(0.65f,0.63f,0.76f),
       float(L+22.0+Col*(P.TileWidth+12.0)),T+108.f,nullptr,0.67f);
  }
  // Draw dependency connections beneath the nodes. Off-screen predecessors
  // remain locked by the canonical prerequisite rule; no phantom edges.
  for(int I=0;I<MoonfallTalents::NodeCount;++I) {
   if(MoonfallTalentWeb::BranchOf(I)!=Branch)continue;
   const auto Dest=MoonfallTalentWeb::NodeRect(P,I,Branch);
   if(Dest.W<=0)continue;
   const auto& N=MoonfallTalents::Nodes[I];
   const int Parents[2]={N.RequiresA,N.RequiresB};
   for(const int Parent:Parents) {
    const auto Source=MoonfallTalentWeb::NodeRect(P,Parent,Branch);
    if(Source.W<=0)continue;
    DrawLine(float(Source.X+Source.W),float(Source.Y+Source.H/2),
        float(Dest.X),float(Dest.Y+Dest.H/2),
        FLinearColor(0.53f,0.43f,0.68f,0.72f),1.4f);
   }
  }
  for(int I=0;I<MoonfallTalents::NodeCount;++I) {
   if(MoonfallTalentWeb::BranchOf(I)!=Branch)continue;
   const auto NodeRect=MoonfallTalentWeb::NodeRect(P,I,Branch);
   if(NodeRect.W<=0)continue;
   const auto& Node=MoonfallTalents::Nodes[I];
   const int Rank=FullRanks?Hero->GetTalentRank(I):0;
   const bool LevelOK=Hero->Level>=Node.MinLevel;
   const bool ParentsOK=(Node.RequiresA<0 || (FullRanks && Hero->GetTalentRank(Node.RequiresA)>0))
     && (Node.RequiresB<0 || (FullRanks && Hero->GetTalentRank(Node.RequiresB)>0));
   const bool CanBuy=LevelOK && ParentsOK && Rank<Node.MaxRank && Points>0;
   const bool Selected=Hero->TalentWebSelected==I;
   const FLinearColor Fill=Selected?FLinearColor(0.42f,0.31f,0.64f,0.97f)
     : Rank>0?FLinearColor(0.17f,0.32f,0.27f,0.97f)
     : CanBuy?FLinearColor(0.19f,0.19f,0.31f,0.96f)
     : FLinearColor(0.095f,0.095f,0.125f,0.95f);
   DrawRect(Fill,float(NodeRect.X),float(NodeRect.Y),
      float(NodeRect.W),float(NodeRect.H));
   DrawRect(Selected?FLinearColor(0.96f,0.79f,0.40f):BranchColors[Branch],
      float(NodeRect.X),float(NodeRect.Y),4.f,float(NodeRect.H));
   // Rank stamp is drawn only for invested or focused talents.
   if ((Selected || Rank>0) && NodeRect.W>=110 && NodeRect.H>=32)
    DrawMoonfallSigil(float(NodeRect.X+NodeRect.W-13),
      float(NodeRect.Y+13),6.f,BranchColors[Branch],Branch);
   const FString Name=FString(UTF8_TO_TCHAR(Node.Title)).Left(P.TileWidth<148?16:22);
   DrawText(Name,Selected?FLinearColor::White:FLinearColor(0.87f,0.86f,0.89f),
      float(NodeRect.X+9),float(NodeRect.Y+5),nullptr,
      P.TileWidth<148?0.65f:0.77f);
   DrawText(FString::Printf(TEXT("%d/%d | LV %d"),Rank,Node.MaxRank,Node.MinLevel),
      FLinearColor(0.72f,0.71f,0.79f),float(NodeRect.X+9),
      float(NodeRect.Y+NodeRect.H-20),nullptr,0.65f);
  }
  const int Chosen=FMath::Clamp(Hero->TalentWebSelected,0,MoonfallTalents::NodeCount-1);
  const auto& Selected=MoonfallTalents::Nodes[Chosen];
  const int CurrentRank=FullRanks?Hero->GetTalentRank(Chosen):0;
  // Always describe the selected effect before spending points. The stat
  // names come from the canonical 141-node catalog, never weapon artwork.
  const TCHAR* EffectName=TEXT("HEALTH");
  switch (Selected.AffectedStat) {
   case MoonfallTalents::Stat::Health: EffectName=TEXT("HEALTH"); break;
   case MoonfallTalents::Stat::Mana: EffectName=TEXT("MANA"); break;
   case MoonfallTalents::Stat::Stamina: EffectName=TEXT("STAMINA"); break;
   case MoonfallTalents::Stat::MeleeDamage: EffectName=TEXT("MELEE DAMAGE"); break;
   case MoonfallTalents::Stat::RangedDamage: EffectName=TEXT("RANGED DAMAGE"); break;
   case MoonfallTalents::Stat::MagicDamage: EffectName=TEXT("MAGIC DAMAGE"); break;
   default: break;
  }
  const double DisplayPerRank=Selected.AmountPerRank*(Selected.IsPercent?100.0:1.0);
  const FString Detail=Selected.AffectedStat==MoonfallTalents::Stat::MeleeDamage
     ? FString::Printf(TEXT("%s  |  MELEE DAMAGE +%.1f%%/RANK  |  RANK %d/%d"),
         UTF8_TO_TCHAR(Selected.Title),DisplayPerRank,CurrentRank,Selected.MaxRank)
     : FString::Printf(TEXT("%s  |  %s +%.1f%s/RANK  |  RANK %d/%d  |  LV %d"),
         UTF8_TO_TCHAR(Selected.Title),EffectName,DisplayPerRank,
         Selected.IsPercent?TEXT("%"):TEXT(""),CurrentRank,Selected.MaxRank,
         Selected.MinLevel);
  DrawText(Detail,FLinearColor(0.91f,0.83f,0.63f),
     L+20.f,T+H-117.f,nullptr,0.79f);
  DrawText(TEXT("Page keys: row  Home or End: tier  D-pad: move  Wheel: pan"),
     FLinearColor(0.75f,0.74f,0.84f),L+20.f,T+H-91.f,nullptr,
     W<760.f?0.54f:0.69f);
  DrawText(TEXT("K/ESC: close   Enter/A: spend   Backspace/LT twice: reset"),
     FLinearColor(0.75f,0.74f,0.84f),L+20.f,T+H-69.f,nullptr,
     W<760.f?0.54f:0.69f);
  const auto Spend=MoonfallTalentWeb::SpendRect(P);
  const auto Respec=MoonfallTalentWeb::RespecRect(P);
  DrawRect(FLinearColor(0.16f,0.36f,0.27f,0.98f),
      float(Spend.X),float(Spend.Y),float(Spend.W),float(Spend.H));
  DrawHoverFrame(float(Spend.X),float(Spend.Y),float(Spend.W),float(Spend.H));
  DrawText(TEXT("SPEND POINT"),FLinearColor::White,
      float(Spend.X+15),float(Spend.Y+6),nullptr,0.75f);
  const bool Arm=Hero->bTalentRespecArmed && Hero->GetWorld()
       && Hero->GetWorld()->GetTimeSeconds()<=Hero->TalentRespecArmUntil;
  DrawRect(Arm?FLinearColor(0.56f,0.26f,0.18f,0.96f)
      :FLinearColor(0.28f,0.18f,0.26f,0.96f),
      float(Respec.X),float(Respec.Y),float(Respec.W),float(Respec.H));
  DrawHoverFrame(float(Respec.X),float(Respec.Y),float(Respec.W),float(Respec.H));
  DrawText(Arm?TEXT("CONFIRM RESET"):TEXT("RESET ALL"),
      FLinearColor::White,float(Respec.X+13),float(Respec.Y+6),nullptr,0.75f);
  DrawLiveMenuHitCue();
  return;
 }
 // Native Canvas resident menu. Matching click bounds are in HeroPawn.
 if (const AMoonfallWorldNPC* Resident = Hero->FocusedResident.Get()) {
  const float InteractionRange = IsValid(Resident) && FMath::IsFinite(Resident->InteractRange)
      ? FMath::Clamp(Resident->InteractRange, 50.f, 400.f) : 0.f;
  if (Hero->Health > 0.f && IsValid(Resident) && InteractionRange > 0.f &&
      !Hero->GetActorLocation().ContainsNaN() &&
      !Resident->GetActorLocation().ContainsNaN() &&
      FVector::DistSquared(Hero->GetActorLocation(), Resident->GetActorLocation()) <=
          FMath::Square(InteractionRange)) {
   if (!Hero->HasResidentContextPointerSurface(Canvas->SizeX,Canvas->SizeY)) {
    DrawText(TEXT("NPC | Enlarge window | ESC / RMB: close"),
      FLinearColor(1.f,0.87f,0.70f),12.f,14.f,nullptr,0.72f);
    DrawLiveMenuHitCue();
    return;
   }
   const float W = FMath::Min(490.f, FMath::Max(260.f, Canvas->SizeX - 24.f));
   const float H = Hero->GetResidentContextPanelHeight(Canvas->SizeY);
   const float L = (Canvas->SizeX - W) * 0.5f, T = (Canvas->SizeY - H) * 0.5f;
   DrawRect(FLinearColor(0.025f, 0.028f, 0.07f, 0.96f), L, T, W, H);
   DrawRect(FLinearColor(0.42f, 0.28f, 0.68f, 0.90f), L, T, W, 5.f);
   DrawModalChrome(L,T,W,H,FLinearColor(0.55f,0.38f,0.70f,0.98f));
   if (W>=420.f) DrawMoonfallSigil(L+W-65.f,T+25.f,10.f,FLinearColor(0.85f,0.73f,0.98f),0);
   // Standard close X in every interactive Moonfall panel.
   const float NpcCloseX = L + W - 42.f;
   DrawRect(FLinearColor(0.37f,0.16f,0.22f,0.96f), NpcCloseX, T + 12.f, 28.f, 28.f);
   DrawHoverFrame(NpcCloseX,T+12.f,28.f,28.f);
   DrawText(TEXT("X"), FLinearColor::White, NpcCloseX + 9.f, T + 16.f, nullptr, 0.9f);
   // One unique heraldic marker for social interaction, never a replacement
   // of the NPC portrait or the authored Z/Nyra character art.
   const int32 ResidentFactionArt =
     Resident->FactionId == FName(TEXT("Wanderers")) ? 0
     : Resident->FactionId == FName(TEXT("VerdantCircle")) ? 1
     : Resident->FactionId == FName(TEXT("StoneHearth")) ? 2
     : Resident->FactionId == FName(TEXT("MoonCoven")) ? 3
     : Resident->FactionId == FName(TEXT("AshenClans")) ? 4 : INDEX_NONE;
   if (W>=390.f) {
    if (ResidentFactionArt!=INDEX_NONE)
     DrawFactionCrest(L+W-67.f,T+75.f,10.f,ResidentFactionArt,
       FLinearColor(0.81f,0.64f,0.94f));
    else DrawMoonfallSigil(L+W-67.f,T+75.f,10.f,
       FLinearColor(0.81f,0.64f,0.94f),2);
   }
   DrawText(Resident->DisplayName.ToString(), FLinearColor::White,
     L + 18.f, T + 16.f, nullptr, Canvas->SizeX < 900.f ? 0.95f : 1.25f);
   DrawText(FString::Printf(TEXT("%s  |  %s  |  Trust %d"),
     *Resident->GetRaceName(), *Resident->GetSocialDisposition(Hero),
     Resident->GetBondWithHero(Hero)), FLinearColor(0.70f,0.83f,0.98f),
     L + 18.f, T + 53.f, nullptr, 0.90f);
   // Story-only, designer-placed residents must not advertise an unfinishable
   // quest. Firstlight and Crossroads known IDs keep their saveable contracts.
   const bool bNoContract = Resident->bAmbientVisitor ||
       !Hero->SupportsResidentContract(Resident->ResidentId);
   const int32 Quest = Resident->bAmbientVisitor ? 2
       : bNoContract ? 2 : Hero->GetResidentQuestState(Resident->ResidentId);
   const int32 ContractCost=bNoContract?0:Resident->GetContractSalvageCost();
   const auto QuestReward=MoonfallQuestXP::Preview(ContractCost,Hero->Level,
       Hero->Experience,Hero->ExperienceToNextLevel);
   const bool bQuestRewardVisible=!bNoContract && Quest<2 && !Hero->bResidentCompanionMenu;
   const FString QuestAction = bNoContract ? TEXT("No contract")
     : Quest == 0 ? TEXT("Accept contract")
     : Quest == 1 ? (Hero->Salvage<ContractCost ? TEXT("Need salvage") : TEXT("Turn in salvage")) : TEXT("Completed");
   FString Names[9] = { TEXT("Talk"),
     Quest == 1 && Hero->bResidentTurnInPending && Hero->GetWorld()
       && Hero->GetWorld()->GetTimeSeconds() <= Hero->ResidentTurnInConfirmUntil
       ? FString(TEXT("Confirm turn-in")) : QuestAction,
     TEXT("Close"), TEXT("Companion"), TEXT("Talents") };
   if(Hero->bResidentCompanionMenu){
    const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<AMoonfallUnrealGameMode>():nullptr;
    const bool bRecruited=Mode&&Mode->GetLocalCompanion()==Resident;
    Names[0]=!bRecruited?TEXT("Recruit (60 trust)"):
      Mode->LocalCompanionStance==EMoonfallCompanionStance::Follow?TEXT("Follow | Switch to Hold"):
      Mode->LocalCompanionStance==EMoonfallCompanionStance::Hold?TEXT("Hold | Switch to Passive"):TEXT("Passive | Switch to Follow");
    Names[1]=bRecruited?TEXT("Dismiss companion"):TEXT("Report deeds");
    Names[5]=TEXT("Report deeds");
    Names[6]=Resident->bCompanionDowned?TEXT("Revive companion"):TEXT("Move ahead");
    Names[7]=TEXT("Trade equipment");
    if(bRecruited){
     const TCHAR* Ability=Mode->LocalCompanionRole==EMoonfallCompanionRole::Protector?TEXT("Brace"):
       Mode->LocalCompanionRole==EMoonfallCompanionRole::Striker?TEXT("Precision Burst"):TEXT("Second Wind");
     const float Remaining=Mode->GetCompanionAbilityRemaining();
     Names[8]=Resident->GetBondWithHero(Hero)<80?FString::Printf(TEXT("%s | 80 trust"),Ability):
       Remaining>0.f?FString::Printf(TEXT("%s | %.0fs"),Ability,Remaining):FString::Printf(TEXT("%s | Activate"),Ability);
    }
    Names[2]=TEXT("Back");
    Names[4]=TEXT("Companion talents");
    Names[3]=!bRecruited?TEXT("Specialization"):
      Mode->LocalCompanionRole==EMoonfallCompanionRole::Balanced?TEXT("Balanced | Switch to Protector"):
      Mode->LocalCompanionRole==EMoonfallCompanionRole::Protector?TEXT("Protector | Switch to Striker"):TEXT("Striker | Switch to Balanced");
    if(Hero->bResidentCompanionGear){
     static const TCHAR* Slots[]={TEXT("Weapon"),TEXT("Armor"),TEXT("Accessory")};
     for(int32 Slot=0;Slot<3;++Slot){
      const int Tier=Hero->GetCompanionGearTier(Resident->ResidentId,Slot);
      Names[Slot]=FString::Printf(TEXT("Give best spare %s | Tier %d"),Slots[Slot],Tier);
      Names[Slot+3]=FString::Printf(TEXT("Return %s"),Slots[Slot]);
     }
     Names[6]=TEXT("Back");
    }
    if(Hero->bResidentCompanionTalents){
     const int Rank=MoonfallWorldPolicy::CompanionRank(Resident->GetBondWithHero(Hero));
     const int Active=bRecruited?MoonfallWorldPolicy::EffectiveCompanionRole(Rank,static_cast<int>(Mode->LocalCompanionRole)):-1;
     static const TCHAR* Paths[]={TEXT("Balanced"),TEXT("Protector"),TEXT("Striker")};
     static const TCHAR* Perks[]={TEXT("Fieldcraft"),TEXT("Shield Expert"),TEXT("Precision")};
     for(int32 Path=0;Path<3;++Path){
      const bool bUnlocked=bRecruited&&Rank>=(Path==0?1:2);
      Names[Path]=FString::Printf(TEXT("%s%s | %s"),Active==Path?TEXT("* "):TEXT(""),Paths[Path],
       !bUnlocked?(Path==0?TEXT("60 trust"):TEXT("80 trust")):Rank==3?Perks[Path]:TEXT("Perk at 100 trust"));
     }
     Names[3]=TEXT("Back");
     Names[4]=TEXT("Train active path");
     if(Hero->bResidentCompanionTraining){
      static const TCHAR* Nodes[9]={TEXT("Far Watch +0.1m cover"),TEXT("First Aid +1 HP cap"),TEXT("Trail Master +0.1m ranges"),
       TEXT("Rapid Cover -1s"),TEXT("Iron Ward +1% cover"),TEXT("Bulwark -1s / +1 cap"),
       TEXT("Long Reach +0.1m"),TEXT("Weak Point +1 posture"),TEXT("Quick Strike -0.5s")};
      const int Role=bRecruited?static_cast<int>(Mode->LocalCompanionRole):0;
      const uint32 Mask=bRecruited?Mode->GetLocalCompanionTraining():0;
      const int Branch=Role>=0&&Role<=2?Role:0;
      for(int32 Index=0;Index<3;++Index){
       const int Node=Branch*3+Index;
       const int Trained=MoonfallWorldPolicy::CompanionNodeRank(Mask,Node);
       const bool bCanTrain=bRecruited&&MoonfallWorldPolicy::CanTrainCompanion(Mask,Rank,Branch,Node);
       Names[Index]=FString::Printf(TEXT("%s | %d/2 %s"),Nodes[Node],Trained,
         Trained==2?TEXT("MAX"):bCanTrain?TEXT("+"):TEXT("Locked"));
      }
      Names[3]=TEXT("Refund all training");Names[4]=TEXT("Back");
     }
    }
   }
   // Mouse hover and gamepad selection share row geometry.
   float CursorX = -1.f, CursorY = -1.f;
   bool bCursorInMenu = false;
   if (const APlayerController* PC = Cast<APlayerController>(Hero->GetController()))
    bCursorInMenu = PC->bShowMouseCursor && PC->GetMousePosition(CursorX, CursorY);
   const float Pitch=Hero->GetResidentContextRowPitch(H);
   for (int32 Row = 0; Row < Hero->GetResidentContextRowCount(); ++Row) {
    const float Y = T + 122.f + Row * Pitch;
    const bool bHover = bCursorInMenu && CursorX >= L + 14.f &&
      CursorX <= L + W - 14.f && CursorY >= Y && CursorY <= Y + Pitch-10.f;
    DrawRect((bHover || Row == Hero->ResidentContextSelection)
        ? FLinearColor(0.24f,0.22f,0.43f,0.98f)
        : FLinearColor(0.10f,0.10f,0.18f,0.94f), L + 14.f, Y, W - 28.f, Pitch-10.f);
    if (Row == Hero->ResidentContextSelection)
     DrawSelectionRail(L+14.f,Y,Pitch-10.f);
    DrawHoverFrame(L+14.f,Y,W-28.f,Pitch-10.f);
    DrawText(Names[Row], !Hero->bResidentCompanionMenu && Row == 1 && (Resident->bAmbientVisitor || Quest == 2)
        ? FLinearColor(0.64f,0.66f,0.72f) : FLinearColor::White,
        L + 25.f, Y + 4.f, nullptr, Pitch<40.f?0.75f:1.f);
   }
   const FString ContractStatus = bNoContract ? TEXT("None")
     : Quest == 0 ? TEXT("Available")
     : Quest == 1 ? TEXT("Active") : TEXT("Completed");
   const int32 CompanionRank=MoonfallWorldPolicy::CompanionRank(Resident->GetBondWithHero(Hero));
   const FString ContextStatus=Hero->bResidentCompanionMenu
     ? FString::Printf(TEXT("Bond rank %d / 3 | %s"),CompanionRank,
         CompanionRank==3?TEXT("Veteran"):CompanionRank==2?TEXT("Proven"):CompanionRank==1?TEXT("Trusted"):TEXT("Not eligible"))
     : FString::Printf(TEXT("Contract: %s  |  Faction %s"),*ContractStatus,*Resident->FactionId.ToString());
   FString DisplayContext=Hero->bResidentCompanionTalents?TEXT("COMPANION TALENTS | Select a path"):ContextStatus;
   if(Hero->bResidentCompanionMenu&&!Hero->bResidentCompanionTalents){
    const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<AMoonfallUnrealGameMode>():nullptr;
    if(Mode&&Mode->GetLocalCompanion()==Resident)
     DisplayContext+=FString::Printf(TEXT(" | HP %.0f/100"),Resident->CompanionHealth);
   }
   if(Hero->bResidentCompanionGear)DisplayContext=TEXT("COMPANION EQUIPMENT | Give or return");
   if(Hero->bResidentCompanionTraining){
    const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<AMoonfallUnrealGameMode>():nullptr;
    const int Spent=Mode&&Mode->GetLocalCompanion()==Resident?MoonfallWorldPolicy::CompanionPointsSpent(Mode->GetLocalCompanionTraining()):0;
    const int Budget=MoonfallWorldPolicy::CompanionPointBudget(CompanionRank);
    DisplayContext=FString::Printf(TEXT("TRAINING | %d / %d points spent"),Spent,Budget);
   }
   if(Hero->bResidentCompanionMenu){
    const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<AMoonfallUnrealGameMode>():nullptr;
    if(!Hero->bResidentCompanionTalents&&Mode&&Mode->GetLocalCompanion()==Resident&&CompanionRank==3){
     using MoonfallWorldPolicy::CompanionPerk;
     switch(MoonfallWorldPolicy::VeteranPerk(CompanionRank,static_cast<int>(Mode->LocalCompanionRole))){
      case CompanionPerk::Fieldcraft: DisplayContext+=TEXT(" | Fieldcraft");break;
      case CompanionPerk::ShieldExpert: DisplayContext+=TEXT(" | Shield Expert");break;
      case CompanionPerk::Precision: DisplayContext+=TEXT(" | Precision");break;
      default:break;
     }
    }
   }
   DrawText(DisplayContext,
     FLinearColor(0.83f,0.81f,0.94f), L + 18.f, T + 77.f, nullptr, 0.76f);
   if(H>=337.f+53.f*(Hero->GetResidentContextRowCount()-3)) DrawText(Resident->bAmbientVisitor
       ? TEXT("Road encounter  |  Talk to hear their story")
       : bNoContract ? TEXT("Talk to hear this resident's story")
       : FString::Printf(TEXT("Salvage: %d / %d"),
           Hero->Salvage, Resident->GetContractSalvageCost()),
       FLinearColor(0.76f,0.87f,0.68f), L + 18.f, T + H - 58.f, nullptr, 0.86f);
   if (Hero->GetWorld() && Hero->GetWorld()->GetTimeSeconds() < Hero->ResidentFeedbackUntil)
    DrawText(Hero->ResidentFeedbackMessage, FLinearColor(0.96f,0.78f,0.41f),
        L + 18.f, T + 99.f, nullptr, W<390.f?0.60f:0.72f);
   else if (bQuestRewardVisible) {
    FString RewardLabel=FString::Printf(TEXT("%d salvage | +%d XP"),ContractCost,QuestReward.RewardXP);
    if (Hero->Level==100) RewardLabel=FString::Printf(TEXT("%d salvage | MAX LEVEL"),ContractCost);
    else if (QuestReward.LevelsGained>0) RewardLabel+=FString::Printf(TEXT(" | Lv %d"),QuestReward.FinalLevel);
    if (W>=390.f) RewardLabel+=FString::Printf(TEXT(" | %s"),UTF8_TO_TCHAR(MoonfallQuestXP::RankName(ContractCost)));
    DrawText(RewardLabel, FLinearColor(0.85f,0.78f,0.98f),L+18.f,T+99.f,nullptr,W<390.f?0.65f:0.76f);
   }
   if(H>=337.f+53.f*(Hero->GetResidentContextRowCount()-3)) DrawText(TEXT("Click choice  |  D-pad up/down + A  |  Esc / RMB: close"),
       FLinearColor(0.80f,0.79f,0.92f), L + 18.f, T + H - 31.f, nullptr,
       Canvas->SizeX < 850.f ? 0.65f : 0.78f);
   DrawLiveMenuHitCue();
   return;
  }
 }
 if (Hero->bAtlasOpen) {
  // Graceful fallback on tiny windows instead of negative panel dimensions.
  if (Canvas->SizeX < 300.f || Canvas->SizeY < 240.f) {
   DrawText(TEXT("ATLAS | Enlarge window | J / ESC: close"),
       FLinearColor(0.91f, 0.84f, 0.68f), 12.f, 14.f, nullptr, 0.75f);
   DrawLiveMenuHitCue();
   return;
  }
  // Wide illustrated Atlas inspired by ink-wash/hand-drawn cartography.
  // Artwork is procedural and schematic, never a claim of surveyed terrain.
  static const TCHAR* AtlasRegions[6] = {TEXT("Ashen Wilds"), TEXT("Moonveil Forest"), TEXT("Hollow Depths"), TEXT("Iron Foundry"), TEXT("Frostbound Reach"), TEXT("Eclipse Wastes")};
  // Original Moonfall place names: one stable title for each of 30 dungeons.
  // Titles identify discovered destinations only; never reveal unknown markers.
  static const TCHAR* AtlasDungeonTitles[30] = {
   TEXT("Emberfall Crypt"), TEXT("Cinderwatch Bastion"), TEXT("Ashen Reliquary"),
   TEXT("Pyreborn Hollow"), TEXT("Sable Furnace"),
   TEXT("Briarshade Grotto"), TEXT("Moonpetal Ruins"), TEXT("Weepingroot Sanctum"),
   TEXT("Veilwood Depths"), TEXT("Thornbound Chapel"),
   TEXT("Blackwater Ossuary"), TEXT("Echoing Chasm"), TEXT("Buried Choir"),
   TEXT("Gloamstone Vault"), TEXT("Abyssal Watch"),
   TEXT("Rustglass Foundry"), TEXT("Brassfang Works"), TEXT("Broken Crucible"),
   TEXT("Furnaceheart Keep"), TEXT("Ironwake Vault"),
   TEXT("Shiverpeak Cairn"), TEXT("Winterglass Tomb"), TEXT("Frostmourn Hollows"),
   TEXT("Glacierwatch Ruins"), TEXT("Coldstar Bastion"),
   TEXT("Umbral Scar"), TEXT("Nightfall Reliquary"), TEXT("Duskspire Ruins"),
   TEXT("Black Sun Sanctuary"), TEXT("Eclipse Throne")
  };
  static_assert(UE_ARRAY_COUNT(AtlasDungeonTitles) == 30, "Each Moonfall dungeon must have a name");
  static const FLinearColor AtlasZoneInk[6] = {
   FLinearColor(0.43f, 0.34f, 0.28f), FLinearColor(0.28f, 0.38f, 0.32f),
   FLinearColor(0.35f, 0.33f, 0.43f), FLinearColor(0.48f, 0.33f, 0.25f),
   FLinearColor(0.35f, 0.43f, 0.47f), FLinearColor(0.39f, 0.34f, 0.39f)
  };
  const float Width = FMath::Min(900.f, Canvas->SizeX - 24.f);
  const float Height = FMath::Min(650.f, Canvas->SizeY - 24.f);
  const float Left = (Canvas->SizeX - Width) * 0.5f;
  const float Top = (Canvas->SizeY - Height) * 0.5f;
  DrawRect(FLinearColor(0.045f, 0.043f, 0.049f, 0.98f), Left, Top, Width, Height);
  DrawRect(FLinearColor(0.45f, 0.38f, 0.28f, 0.96f), Left + 8.f, Top + 8.f, Width - 16.f, 1.f);
  DrawModalChrome(Left,Top,Width,Height,FLinearColor(0.55f,0.46f,0.30f,0.98f));
  if (Width>=480.f) DrawMoonfallSigil(Left+Width-67.f,Top+24.f,11.f,FLinearColor(0.92f,0.75f,0.42f),1);
  const float Scale = FMath::Clamp(Width / 900.f, 0.48f, 1.f);
  DrawText(TEXT("MOONFALL | CARTOGRAPHER'S ATLAS"), FLinearColor(0.91f, 0.84f, 0.68f), Left + 18.f, Top + 13.f, nullptr, Scale * 1.32f);
  // The clickable X is the only Atlas header button (no ME button).
  const float AtlasCloseX = Left + Width - 42.f;
  DrawRect(FLinearColor(0.37f, 0.16f, 0.22f, 0.96f), AtlasCloseX, Top + 12.f, 28.f, 28.f);
  DrawHoverFrame(AtlasCloseX,Top+12.f,28.f,28.f);
  DrawText(TEXT("X"), FLinearColor::White, AtlasCloseX + 9.f, Top + 16.f, nullptr, 0.9f);
  DrawText(Hero->bAtlasChronicleFocus
      ? TEXT("J/ESC close | Up/Down: region | Left/Right: chapter")
      : TEXT("J/ESC close | Arrows: region / pan | Wheel: zoom"),
    FLinearColor(0.82f, 0.77f, 0.67f), Left + 18.f, Top + 49.f,
    nullptr, Scale * 0.75f);
  DrawText(Hero->bAtlasChronicleFocus
      ? TEXT("Tab / R3: map | Mouse: chapter tabs | Wheel: zoom")
      : TEXT("Mouse: select a region or pan the chart | Tab / R3: read"),
    FLinearColor(0.71f, 0.69f, 0.62f), Left + 18.f, Top + 70.f,
    nullptr, Scale * 0.72f);
  const float AtlasSidebarWidth = FMath::Clamp(Width * 0.27f, 132.f, 238.f);
  const float AtlasRowGap = FMath::Clamp((Height - 145.f) / 6.f, 24.f, 78.f);
  const bool bCompactAtlasRows = AtlasRowGap < 55.f;
  const float AtlasSideTextScale = FMath::Clamp(AtlasSidebarWidth / 230.f, 0.50f, 1.f);
  const int32 Selected = FMath::Clamp(Hero->AtlasSelectedRegion, 0, 5);
  DrawRect(FLinearColor(0.09f, 0.08f, 0.08f, 0.95f),
      Left + 10.f, Top + 102.f, AtlasSidebarWidth - 16.f, Height - 132.f);
  for (int32 Zone = 0; Zone < 6; ++Zone) {
   const float Y = Top + 110.f + Zone * AtlasRowGap;
   const float RowHeight = FMath::Max(20.f, AtlasRowGap - 4.f);
   if (Zone == Selected) {
    DrawRect(FLinearColor(0.24f, 0.21f, 0.18f, 0.95f),
        Left + 13.f, Y - 4.f, AtlasSidebarWidth - 22.f, RowHeight);
    DrawRect(AtlasZoneInk[Zone], Left + 13.f, Y - 4.f, 4.f, RowHeight);
   }
   DrawHoverFrame(Left+13.f,Y-4.f,AtlasSidebarWidth-22.f,RowHeight);
   // Six different illustrated biome seals, one per authored Atlas region.
   // They identify region names already visible, never hidden geography.
   if (AtlasSidebarWidth>=210.f && RowHeight>=33.f)
    DrawRegionEmblem(Left+AtlasSidebarWidth-25.f,Y+RowHeight*0.5f-3.f,
      9.f,Zone,AtlasZoneInk[Zone]);
   int32 Discovered = 0, Defeated = 0, Revealed = 0;
   for (int32 Dungeon = 0; Dungeon < 5; ++Dungeon) {
    if (Hero->HasDiscoveredDungeon(Zone, Dungeon)) ++Discovered;
    if (Hero->HasDefeatedDungeonBoss(Zone, Dungeon)) ++Defeated;
   }
   for (int32 FogRow = 0; FogRow < 7; ++FogRow)
    for (int32 FogColumn = 0; FogColumn < 8; ++FogColumn)
     if (Hero->HasExploredAtlasCell(Zone, FogColumn, FogRow)) ++Revealed;
   DrawText(FString(AtlasRegions[Zone]), Zone == Selected ? FLinearColor(1.f, 0.88f, 0.70f)
       : FLinearColor(0.75f, 0.73f, 0.66f),
       Left + 23.f, Y, nullptr, Scale * AtlasSideTextScale * 0.91f);
   DrawText(FString::Printf(TEXT("DUNGEONS %d/5 | BOSSES %d/5"), Discovered, Defeated),
       FLinearColor(0.72f, 0.79f, 0.76f), Left + 23.f, Y + 19.f,
       nullptr, Scale * AtlasSideTextScale * 0.73f);
   if (!bCompactAtlasRows) {
    DrawText(FString::Printf(TEXT("EXPLORED %d%%"), FMath::RoundToInt(100.f * Revealed / 56.f)),
        FLinearColor(0.72f, 0.66f, 0.55f), Left + 23.f, Y + 38.f,
        nullptr, Scale * AtlasSideTextScale * 0.75f);
   }
  }
  // Read-only worldwide Chronicle progress on the Atlas sidebar footer.
  // Keep the compact label within the sidebar on smaller window sizes.
  const uint32 ChronicleDiscovered = static_cast<uint32>(Hero->DiscoveredDungeonMask);
  const uint32 ChronicleDefeated = static_cast<uint32>(Hero->DefeatedDungeonBossMask);
  const auto WorldChronicle = MoonfallChronicle::SummarizeWorld(
      ChronicleDiscovered, ChronicleDefeated);
  if (WorldChronicle.Valid) {
   const FString WorldLabel = AtlasSidebarWidth < 175.f
      ? FString::Printf(TEXT("%d/42 | %d/6 ACTS"),
          WorldChronicle.ChaptersRevealed, WorldChronicle.ActsCompleted)
      : FString::Printf(TEXT("STORY %d/42 | ACTS %d/6"),
          WorldChronicle.ChaptersRevealed, WorldChronicle.ActsCompleted);
   DrawText(WorldLabel, FLinearColor(0.91f, 0.80f, 0.60f),
       Left + 14.f, Top + Height - 24.f,
       nullptr, Scale * AtlasSideTextScale * 0.79f);
  }
  // Large, readable map viewport. Match the pointer hit tests in HeroPawn.
  const float MapLeft = Left + AtlasSidebarWidth + 19.f;
  const float MapWidth = FMath::Max(40.f, Width - AtlasSidebarWidth - 36.f);
  const float MapTop = Top + 105.f;
  const float MapHeight = FMath::Max(74.f, Height - 141.f);
  DrawRect(FLinearColor(0.09f, 0.087f, 0.087f, 1.f),
      MapLeft - 3.f, MapTop - 3.f, MapWidth + 6.f, MapHeight + 6.f);
  DrawRect(FLinearColor(0.054f, 0.053f, 0.055f, 1.f), MapLeft, MapTop, MapWidth, MapHeight);
  const float MapCenterX = (static_cast<float>(Selected * 5 + 2) - 14.5f) * 1350.f;
  const float SafeAtlasZoom = FMath::Clamp(FMath::IsFinite(Hero->AtlasZoom) ? Hero->AtlasZoom : 1.f, 0.8f, 1.8f);
  const float SafeAtlasPanY = FMath::Clamp(FMath::IsFinite(Hero->AtlasPanY) ? Hero->AtlasPanY : -2500.f, -12000.f, 12000.f);
  const float VisibleHalfSpan = 21000.f / SafeAtlasZoom;
  const auto ProjectX = [&](float WorldX) {
   return MapLeft + MapWidth * (0.5f + (WorldX - MapCenterX) / (2.f * VisibleHalfSpan));
  };
  const float VisibleHalfY = 6500.f / SafeAtlasZoom;
  const auto ProjectY = [&](float WorldY) {
   return MapTop + MapHeight * (0.5f - (WorldY - SafeAtlasPanY) / (2.f * VisibleHalfY));
  };
  // V12 geographic fog: only cells the hero physically entered gain
  // subdued terrain shading. Do not reveal empty world coordinates or roads.
  // Bound both iteration (336 cells) and draw area to this Atlas viewport.
  const float FogCellW = 6750.f / 8.f;
  const float FogCellH = 24000.f / 7.f;
  for (int32 Zone = 0; Zone < 6; ++Zone) {
   const float ZoneMinX = -20250.f + Zone * 6750.f;
   for (int32 Row = 0; Row < 7; ++Row) {
    const float CellMinY = -12000.f + Row * FogCellH;
    for (int32 Column = 0; Column < 8; ++Column) {
     if (!Hero->HasExploredAtlasCell(Zone, Column, Row)) continue;
     const float CellMinX = ZoneMinX + Column * FogCellW;
     const float CellLeft = FMath::Max(MapLeft, ProjectX(CellMinX));
     const float CellRight = FMath::Min(MapLeft + MapWidth, ProjectX(CellMinX + FogCellW));
     const float CellTop = FMath::Max(MapTop, ProjectY(CellMinY + FogCellH));
     const float CellBottom = FMath::Min(MapTop + MapHeight, ProjectY(CellMinY));
     if (CellRight > CellLeft + 0.5f && CellBottom > CellTop + 0.5f) {
      // Ink-wash pigments suggest biome character without inventing topography.
      const float InkVariation = 0.84f + 0.04f * float((Column * 3 + Row * 5) % 5);
      DrawRect(AtlasZoneInk[Zone] * InkVariation, CellLeft, CellTop,
          CellRight - CellLeft, CellBottom - CellTop);
      // Subtle engraved contours only in revealed cells; bounded to 3 lines.
      if (CellRight - CellLeft > 14.f && CellBottom - CellTop > 15.f) {
       const FLinearColor Etch = AtlasZoneInk[Zone] * 0.64f;
       const float EtchInset = FMath::Min(7.f, (CellRight - CellLeft) * 0.12f);
       for (int32 EtchLine = 1; EtchLine <= 3; ++EtchLine) {
        const float EtchY = CellTop + (CellBottom - CellTop) * float(EtchLine) / 4.f;
        const float EtchOffset = float((Column + Row + EtchLine) % 3) * 2.f;
        DrawRect(Etch, CellLeft + EtchInset + EtchOffset, EtchY,
            FMath::Max(0.f, CellRight - CellLeft - EtchInset * 2.f - EtchOffset), 1.f);
       }
      }
     }
    }
   }
  }
  // A static compass rose belongs to the map paper, not the world data.
  // Drawing it reveals no unexplored dungeons, roads, shrines or fog cells.
  if (MapWidth>=260.f && MapHeight>=170.f) {
   const float CompassX=MapLeft+MapWidth-29.f;
   const float CompassY=MapTop+35.f;
   DrawMoonfallSigil(CompassX,CompassY,13.f,
     FLinearColor(0.86f,0.76f,0.51f),1);
   DrawText(TEXT("N"),FLinearColor(0.92f,0.83f,0.59f),
     CompassX-4.f,CompassY-27.f,nullptr,0.63f);
  }
  // Named region caption is public knowledge; geographic details stay fogged.
  DrawText(FString(AtlasRegions[Selected]), FLinearColor(0.90f, 0.81f, 0.66f),
      MapLeft + 10.f, MapTop + 10.f, nullptr, Scale * 0.82f);
  // Do not reveal the complete road through unexplored territory.
  // Show only road segments around previously discovered dungeons.
  const float RoadY = ProjectY(-2500.f);
  bool bNamedRoadRevealed = false;
  if (RoadY >= MapTop && RoadY <= MapTop + MapHeight - 4.f) {
   for (int32 Slot = 0; Slot < 30; ++Slot) {
    const int32 Zone = Slot / 5;
    const int32 Dungeon = Slot % 5;
    if (!Hero->HasDiscoveredDungeon(Zone, Dungeon) && !Hero->HasDefeatedDungeonBoss(Zone, Dungeon)) continue;
    const float WorldX = (static_cast<float>(Slot) - 14.5f) * 1350.f;
    const float RoadStart = FMath::Max(MapLeft, ProjectX(WorldX - 600.f));
    const float RoadEnd = FMath::Min(MapLeft + MapWidth, ProjectX(WorldX + 600.f));
    if (RoadEnd > RoadStart) {
     bNamedRoadRevealed = true;
     DrawRect(FLinearColor(0.22f, 0.25f, 0.31f), RoadStart, RoadY, RoadEnd - RoadStart, 4.f);
    }
   }
  }
  // Road title appears only after at least one discovered segment is visible.
  if (bNamedRoadRevealed && MapHeight >= 95.f)
   DrawText(TEXT("DUNGEON ROAD"), FLinearColor(0.76f, 0.72f, 0.64f),
       MapLeft + 10.f, FMath::Clamp(RoadY - 14.f, MapTop + 24.f, MapTop + MapHeight - 20.f),
       nullptr, Scale * 0.59f);
  // One pointer lookup per Atlas frame. Tooltips do not perform world traces
  // or expose any undiscovered coordinates.
  float AtlasPointerX = 0.f, AtlasPointerY = 0.f;
  bool bAtlasPointerInsideMap = false;
  if (APlayerController* AtlasPC = Cast<APlayerController>(Hero->GetController())) {
   if (AtlasPC->bShowMouseCursor && AtlasPC->GetMousePosition(AtlasPointerX, AtlasPointerY)
       && FMath::IsFinite(AtlasPointerX) && FMath::IsFinite(AtlasPointerY))
    bAtlasPointerInsideMap = AtlasPointerX >= MapLeft && AtlasPointerX <= MapLeft + MapWidth
        && AtlasPointerY >= MapTop && AtlasPointerY <= MapTop + MapHeight;
  }
  int32 HoveredDungeonSlot = INDEX_NONE;
  bool bHoveredShrine = false;
  bool bHoveredActiveRespawn = false;
  FString HoveredShrineTitle;
  // Map wayfinding: mark only the nearest known, unfinished dungeon in the
  // selected region. Distances are straight-line estimates, NOT traversable routes.
  const FVector HeroPosition = Hero->GetActorLocation();
  const bool bValidHeroXY = FMath::IsFinite(HeroPosition.X) && FMath::IsFinite(HeroPosition.Y);
  int32 NearestKnownDungeonSlot = INDEX_NONE;
  float NearestKnownDungeonDistanceSq = 1.0e20f;
  if (bValidHeroXY) {
   for (int32 Dungeon = 0; Dungeon < 5; ++Dungeon) {
    if (!Hero->HasDiscoveredDungeon(Selected, Dungeon)
        || Hero->HasDefeatedDungeonBoss(Selected, Dungeon)) continue;
    const int32 Slot = Selected * 5 + Dungeon;
    const float WorldX = (static_cast<float>(Slot) - 14.5f) * 1350.f;
    const float DistanceSq = FMath::Square(HeroPosition.X - WorldX)
        + FMath::Square(HeroPosition.Y + 2500.f);
    if (FMath::IsFinite(DistanceSq) && DistanceSq < NearestKnownDungeonDistanceSq) {
     NearestKnownDungeonSlot = Slot;
     NearestKnownDungeonDistanceSq = DistanceSq;
    }
   }
  }
  for (int32 Slot = 0; Slot < 30; ++Slot) {
   const float WorldX = (static_cast<float>(Slot) - 14.5f) * 1350.f;
   const float MarkerX = ProjectX(WorldX);
   if (MarkerX < MapLeft + 5.f || MarkerX > MapLeft + MapWidth - 5.f) continue;
   const int32 Zone = Slot / 5;
   const int32 Dungeon = Slot % 5;
   const bool bFound = Hero->HasDiscoveredDungeon(Zone, Dungeon);
   const bool bCleared = Hero->HasDefeatedDungeonBoss(Zone, Dungeon);
   const float MarkerY = ProjectY(-2500.f);
   if (MarkerY < MapTop + 5.f || MarkerY > MapTop + MapHeight - 5.f) continue;
   // Undiscovered dungeon locations remain hidden instead of exposing coordinates.
   if (!bFound && !bCleared) continue;
   if (bAtlasPointerInsideMap && FMath::Abs(AtlasPointerX - MarkerX) <= 10.f
       && FMath::Abs(AtlasPointerY - MarkerY) <= 10.f)
    HoveredDungeonSlot = Slot;
   if (Slot == NearestKnownDungeonSlot)
    DrawRect(FLinearColor(0.96f, 0.72f, 0.33f), MarkerX - 7.f, MarkerY - 7.f, 14.f, 14.f);
   DrawRect(bCleared ? FLinearColor(0.24f, 0.65f, 0.46f)
     : FLinearColor(0.33f, 0.58f, 0.86f),
     MarkerX - 4.f, MarkerY - 4.f, 8.f, 8.f);
  }
  // World-space shrine icons are cached when J opens the Atlas.
  // Only discovered shrine actors appear. The active respawn is violet,
  // while other permanently discovered shrines remain cyan.
  int32 AtlasShrineIndex = 0;
  for (const FVector& ShrinePos : Hero->AtlasDiscoveredShrineLocations) {
   const FString ShrineTitle = Hero->AtlasDiscoveredShrineNames.IsValidIndex(AtlasShrineIndex)
       ? Hero->AtlasDiscoveredShrineNames[AtlasShrineIndex] : FString(TEXT("Discovered Shrine"));
   ++AtlasShrineIndex;
   if (!FMath::IsFinite(ShrinePos.X) || !FMath::IsFinite(ShrinePos.Y)) continue;
   const float ShrineX = ProjectX(ShrinePos.X);
   const float ShrineY = ProjectY(ShrinePos.Y);
   if (ShrineX < MapLeft + 6.f || ShrineX > MapLeft + MapWidth - 6.f
       || ShrineY < MapTop + 6.f || ShrineY > MapTop + MapHeight - 6.f) continue;
   const bool bActiveRespawn = Hero->bAtlasHasActiveRespawnShrineLocation
       && FVector::DistSquared2D(ShrinePos, Hero->AtlasActiveRespawnShrineLocation) <= 1.f;
   if (bAtlasPointerInsideMap && FMath::Abs(AtlasPointerX - ShrineX) <= 11.f
       && FMath::Abs(AtlasPointerY - ShrineY) <= 11.f) {
    bHoveredShrine = true;
    bHoveredActiveRespawn = bActiveRespawn;
    HoveredShrineTitle = ShrineTitle;
   }
   DrawRect(bActiveRespawn ? FLinearColor(0.70f, 0.49f, 0.95f) : FLinearColor(0.32f, 0.91f, 0.91f),
       ShrineX - 5.f, ShrineY - 5.f, 10.f, 10.f);
   DrawRect(FLinearColor(0.025f, 0.035f, 0.085f), ShrineX - 2.f, ShrineY - 2.f, 4.f, 4.f);
  }
  if (bValidHeroXY) {
   const float HeroX = ProjectX(HeroPosition.X);
   const float HeroY = ProjectY(HeroPosition.Y);
   // Never pin a phantom "YOU" label to an edge when the hero is off-screen.
   if (HeroX >= MapLeft + 4.f && HeroX <= MapLeft + MapWidth - 4.f
       && HeroY >= MapTop + 4.f && HeroY <= MapTop + MapHeight - 4.f) {
    DrawRect(FLinearColor(1.f, 0.87f, 0.30f), HeroX - 4.f, HeroY - 4.f, 8.f, 8.f);
    if (MapWidth >= 150.f && MapHeight >= 95.f)
     DrawText(TEXT("YOU"), FLinearColor(1.f, 0.90f, 0.48f),
         FMath::Clamp(HeroX + 10.f, MapLeft + 5.f, MapLeft + MapWidth - 28.f),
         FMath::Clamp(HeroY - 12.f, MapTop + 28.f, MapTop + MapHeight - 17.f),
         nullptr, Scale * 0.67f);
   }
  }
  // Full six-act, seven-chapter-per-act Chronicle. The same geometry is
  // shared with ResidentContextMouseClick: tabs NEVER pan the map behind.
  // All 42 chapter titles and prose are gated by saved dungeon discovery.
  const MoonfallChronicle::FProgress Chronicle = MoonfallChronicle::Summarize(
    Selected, ChronicleDiscovered, ChronicleDefeated);
  const MoonfallChronicleUI::FCard ChronicleCard = MoonfallChronicleUI::Layout(
    MapLeft, MapTop, MapWidth, MapHeight, Hero->bAtlasChronicleFocus);
  const int32 ChapterIndex = Hero->AtlasChronicleChapter >= 0
    && Hero->AtlasChronicleChapter < MoonfallChronicle::BeatsPerAct
    ? Hero->AtlasChronicleChapter : Chronicle.FeaturedBeat;
  const MoonfallChronicle::FBeat* ChronicleBeat = Chronicle.Valid
    ? MoonfallChronicle::RevealedBeat(Selected, ChapterIndex,
        ChronicleDiscovered, ChronicleDefeated)
    : nullptr;
  const bool bChroniclePointer = bAtlasPointerInsideMap
    && MoonfallChronicleUI::HitCard(ChronicleCard, AtlasPointerX, AtlasPointerY);
  if (Chronicle.Valid && ChronicleCard.Visible) {
   const float CL = ChronicleCard.Left, CT = ChronicleCard.Top;
   const float CW = ChronicleCard.Width, CH = ChronicleCard.Height;
   DrawRect(FLinearColor(0.038f, 0.041f, 0.067f, 0.97f), CL, CT, CW, CH);
   DrawRect(FLinearColor(0.73f, 0.56f, 0.32f, 0.96f), CL, CT, 3.f, CH);
   const FString ActLabel = FString::Printf(TEXT("ACT %d/6  |  %s"),
     Selected + 1, UTF8_TO_TCHAR(MoonfallChronicle::ActNames[Selected]));
   DrawText(ActLabel.Left(MapWidth < 420.f ? 24 : 44),
     FLinearColor(0.96f, 0.84f, 0.62f),
     CL + 10.f, CT + 5.f, nullptr, Scale * 0.74f);
   // Reading focus can also be toggled by Tab/controller R3. Mouse tabs
   // directly select any of seven chapters, including a redacted locked one.
   DrawRect(Hero->bAtlasChronicleFocus ? FLinearColor(0.33f, 0.27f, 0.17f)
       : FLinearColor(0.16f, 0.19f, 0.27f),
     ChronicleCard.ToggleLeft, ChronicleCard.ToggleTop,
     ChronicleCard.ToggleWidth, ChronicleCard.ToggleHeight);
   DrawText(Hero->bAtlasChronicleFocus ? TEXT("MAP") : TEXT("READ"),
     FLinearColor::White, ChronicleCard.ToggleLeft + 8.f,
     ChronicleCard.ToggleTop + 2.f, nullptr, Scale * 0.66f);
   for (int32 Chapter = 0; Chapter < MoonfallChronicle::BeatsPerAct; ++Chapter) {
    const auto ChapterState = MoonfallChronicle::BeatState(
      Selected, Chapter, ChronicleDiscovered, ChronicleDefeated);
    const bool bSelected = Chapter == ChapterIndex;
    const FLinearColor ChapterInk =
      ChapterState == MoonfallChronicle::EBeatState::Hidden
        ? FLinearColor(0.29f, 0.29f, 0.32f)
        : ChapterState == MoonfallChronicle::EBeatState::Cleared
          ? FLinearColor(0.23f, 0.58f, 0.45f)
          : FLinearColor(0.67f, 0.48f, 0.24f);
    const float TabX = ChronicleCard.TabLeft + ChronicleCard.TabWidth * Chapter;
    DrawRect(bSelected ? ChapterInk : ChapterInk * 0.48f,
      TabX + 1.f, ChronicleCard.TabTop,
      ChronicleCard.TabWidth - 2.f, ChronicleCard.TabHeight);
    if (bSelected)
     DrawRect(FLinearColor(0.98f, 0.90f, 0.70f),
       TabX + 1.f, ChronicleCard.TabTop + ChronicleCard.TabHeight - 2.f,
       ChronicleCard.TabWidth - 2.f, 2.f);
    DrawText(FString::FromInt(Chapter + 1),
      ChapterState == MoonfallChronicle::EBeatState::Hidden
        ? FLinearColor(0.77f, 0.76f, 0.75f) : FLinearColor::White,
      TabX + (ChronicleCard.TabWidth - 11.f) * 0.5f,
      ChronicleCard.TabTop + 2.f, nullptr, Scale * 0.75f);
   }
   const auto SelectedState = MoonfallChronicle::BeatState(
     Selected, ChapterIndex, ChronicleDiscovered, ChronicleDefeated);
   const TCHAR* ChapterStatus =
     SelectedState == MoonfallChronicle::EBeatState::Hidden ? TEXT("LOCKED")
       : SelectedState == MoonfallChronicle::EBeatState::Cleared ? TEXT("CLEARED")
       : TEXT("DISCOVERED");
   const FString ChapterLabel = ChronicleBeat
     ? FString::Printf(TEXT("CHAPTER %d/7  |  %s"),
          ChapterIndex + 1, UTF8_TO_TCHAR(ChronicleBeat->Title))
     : FString::Printf(TEXT("CHAPTER %d/7  |  UNDISCOVERED"),
          ChapterIndex + 1);
   DrawText(ChapterLabel.Left(MapWidth < 400.f ? 38 : 59),
     FLinearColor::White, CL + 10.f, CT + 51.f, nullptr, Scale * 0.77f);
   // Never touch Acts[][] for unrevealed chapters. The text must not leak
   // via titles, labels, pointer tooltips or automatic next-page selection.
   const FString Lore = ChronicleBeat
     ? FString(UTF8_TO_TCHAR(ChronicleBeat->Lore))
     : ChapterIndex == 6
       ? FString(TEXT("Defeat all five regional dungeon bosses to reveal this chapter."))
       : FString(TEXT("Discover the corresponding dungeon entrance to reveal this chapter."));
   const int32 LoreLineLimit = MapWidth >= 480.f ? 78 : 48;
   const int32 LoreLines = ChronicleCard.LoreLines;
   int32 LoreCursor = 0;
   for (int32 Line = 0; Line < LoreLines && LoreCursor < Lore.Len(); ++Line) {
    int32 End = FMath::Min(LoreCursor + LoreLineLimit, Lore.Len());
    if (End < Lore.Len()) {
     int32 WordEnd = End;
     while (WordEnd > LoreCursor + LoreLineLimit / 2
         && Lore[WordEnd] != TCHAR(' ')) --WordEnd;
     if (WordEnd > LoreCursor + LoreLineLimit / 2) End = WordEnd;
    }
    FString Segment = Lore.Mid(LoreCursor, End - LoreCursor);
    if (Line == LoreLines - 1 && End < Lore.Len())
     Segment = Segment.Left(FMath::Max(0, Segment.Len() - 3)) + TEXT("...");
    DrawText(Segment, FLinearColor(0.79f, 0.84f, 0.93f),
       CL + 10.f, CT + 72.f + Line * 17.f, nullptr, Scale * 0.68f);
    LoreCursor = End;
    while (LoreCursor < Lore.Len() && Lore[LoreCursor] == TCHAR(' '))
     ++LoreCursor;
   }
   DrawText(FString::Printf(TEXT("%d/7 REVEALED  |  %d/5 BOSSES  |  %s"),
       Chronicle.Revealed, Chronicle.BossesDefeated, ChapterStatus),
     FLinearColor(0.66f, 0.86f, 0.78f),
     CL + 10.f, CT + CH - 19.f, nullptr, Scale * 0.67f);
  }
  // Hover cards are purely informational and keep the mouse optional.
  // Only known markers qualify, and cards remain inside the chart bounds.
  if (bAtlasPointerInsideMap && !bChroniclePointer
      && MapWidth >= 150.f && MapHeight >= 95.f
      && (bHoveredShrine || HoveredDungeonSlot != INDEX_NONE)) {
   const float TooltipW = FMath::Min(260.f, MapWidth - 12.f);
   // Clip very long authored titles to the information card width.
   const int32 TooltipNameChars = TooltipW < 200.f ? 20 : 29;
   const float TooltipH = 53.f;
   const float TooltipX = FMath::Clamp(AtlasPointerX + 12.f,
       MapLeft + 4.f, MapLeft + MapWidth - TooltipW - 4.f);
   const float TooltipY = FMath::Clamp(AtlasPointerY - TooltipH - 12.f,
       MapTop + 4.f, MapTop + MapHeight - TooltipH - 4.f);
   DrawRect(FLinearColor(0.045f, 0.044f, 0.052f, 0.98f),
       TooltipX, TooltipY, TooltipW, TooltipH);
   DrawRect(FLinearColor(0.58f, 0.48f, 0.33f),
       TooltipX, TooltipY, TooltipW, 2.f);
   if (bHoveredShrine) {
    DrawText(HoveredShrineTitle.Left(TooltipNameChars),
        FLinearColor(0.95f, 0.82f, 0.65f), TooltipX + 7.f, TooltipY + 6.f,
        nullptr, Scale * 0.77f);
    DrawText(bHoveredActiveRespawn ? TEXT("Active respawn shrine") : TEXT("Discovered Shrine"),
        FLinearColor(0.82f, 0.80f, 0.75f), TooltipX + 7.f, TooltipY + 22.f,
        nullptr, Scale * 0.68f);
    DrawText(TEXT("Permanent discovery"), FLinearColor(0.76f, 0.76f, 0.72f),
        TooltipX + 7.f, TooltipY + 37.f, nullptr, Scale * 0.60f);
   } else {
    const bool bDungeonCleared = Hero->HasDefeatedDungeonBoss(
        HoveredDungeonSlot / 5, HoveredDungeonSlot % 5);
    DrawText(FString(AtlasDungeonTitles[HoveredDungeonSlot]).Left(TooltipNameChars),
        FLinearColor(0.95f, 0.82f, 0.65f), TooltipX + 7.f, TooltipY + 6.f,
        nullptr, Scale * 0.77f);
    DrawText(FString::Printf(TEXT("%s | Dungeon %02d"),
        AtlasRegions[HoveredDungeonSlot / 5], HoveredDungeonSlot + 1).Left(42),
        FLinearColor(0.82f, 0.80f, 0.75f), TooltipX + 7.f, TooltipY + 22.f,
        nullptr, Scale * 0.65f);
    DrawText(bDungeonCleared ? TEXT("Boss defeated") : TEXT("Discovered | boss not defeated"),
        FLinearColor(0.76f, 0.76f, 0.72f), TooltipX + 7.f, TooltipY + 37.f,
        nullptr, Scale * 0.60f);
   }
  }
  // Fixed compass: Unreal's +Y world axis points up on this XY projection.
  DrawText(TEXT("N"), FLinearColor(0.91f, 0.78f, 0.52f),
      MapLeft + MapWidth - 22.f, MapTop + 8.f, nullptr, Scale * 0.88f);
  DrawRect(FLinearColor(0.91f, 0.78f, 0.52f),
      MapLeft + MapWidth - 19.f, MapTop + 27.f, 2.f, 12.f);
  // Readable in every region, including ones with no discovered entrances.
  DrawText(MapWidth >= 430.f ? TEXT("Gold: hero / goal | cyan: shrine | violet: respawn")
      : TEXT("Gold: hero / goal | cyan: shrine"),
      FLinearColor(0.82f, 0.77f, 0.67f), MapLeft, MapTop + MapHeight + 5.f,
      nullptr, Scale * 0.68f);
  if (NearestKnownDungeonSlot != INDEX_NONE) {
   const int32 Meters = FMath::Clamp(
       FMath::RoundToInt(FMath::Sqrt(NearestKnownDungeonDistanceSq) * 0.01f), 0, 999999);
   const FString GoalName = FString(AtlasDungeonTitles[NearestKnownDungeonSlot])
       .Left(MapWidth >= 500.f ? 28 : 17);
   const FString GoalText = MapWidth >= 315.f
       ? FString::Printf(TEXT("%s | D%02d | %d m straight line"),
           *GoalName, NearestKnownDungeonSlot + 1, Meters)
       : FString::Printf(TEXT("D%02d | %d m direct"),
           NearestKnownDungeonSlot + 1, Meters);
   DrawText(GoalText, FLinearColor(0.97f, 0.81f, 0.57f),
       MapLeft, MapTop + MapHeight + 20.f, nullptr, Scale * 0.66f);
  } else {
   DrawText(TEXT("No discovered, uncleared dungeon in selected region"),
       FLinearColor(0.70f, 0.70f, 0.66f), MapLeft, MapTop + MapHeight + 20.f,
       nullptr, Scale * 0.66f);
  }
  DrawLiveMenuHitCue();
  return;
 }
 if (Hero->bOptionsOpen) {
  if (Canvas->SizeX < 264.f || Canvas->SizeY < 304.f) {
   DrawText(TEXT("OPTIONS | Enlarge window | ESC to close"),
     FLinearColor(1.f,0.87f,0.70f),12.f,14.f,nullptr,0.75f);
   DrawLiveMenuHitCue();
   return;
  }
  const float MenuWidth = FMath::Min(560.f, FMath::Max(240.f, Canvas->SizeX - 24.f));
  const float MenuHeight = FMath::Min(550.f, FMath::Max(280.f, Canvas->SizeY - 24.f));
  const float MenuLeft = Canvas->SizeX * 0.5f - MenuWidth * 0.5f;
  const float MenuTop = Canvas->SizeY * 0.5f - MenuHeight * 0.5f;
  const float MenuRowGap = (MenuHeight - 130.f) / 8.f;
  const float RowHeight = FMath::Min(38.f, MenuRowGap - 2.f);
  const float FirstRowY = MenuTop + 63.f;
  const bool bCompactOptions = MenuWidth < 420.f;
  DrawRect(FLinearColor(0.04f,0.03f,0.11f,0.94f), MenuLeft, MenuTop, MenuWidth, MenuHeight);
  DrawModalChrome(MenuLeft,MenuTop,MenuWidth,MenuHeight,FLinearColor(0.35f,0.52f,0.70f,0.98f));
  if (MenuWidth>=480.f) DrawMoonfallSigil(MenuLeft+MenuWidth-66.f,MenuTop+25.f,10.f,FLinearColor(0.52f,0.77f,0.95f),2);
  const float OptionsCloseX = MenuLeft + MenuWidth - 42.f;
  DrawRect(FLinearColor(0.37f,0.16f,0.22f,0.96f), OptionsCloseX, MenuTop + 12.f, 28.f, 28.f);
  DrawHoverFrame(OptionsCloseX,MenuTop+12.f,28.f,28.f);
  DrawText(TEXT("X"), FLinearColor::White, OptionsCloseX + 9.f, MenuTop + 16.f, nullptr, 0.9f);
  DrawText(bCompactOptions ? TEXT("MOONFALL | OPTIONS") : TEXT("MOONFALL | GRAPHICS & ACCESSIBILITY"),
      FLinearColor::White, MenuLeft + 18.f, MenuTop + 17.f, nullptr, bCompactOptions ? 0.83f : 1.05f);
  UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
  if (Settings) {
   float ScalePct = 100.f, ScaleNormalized = 0.f, ScaleMin = 50.f, ScaleMax = 100.f;
   Settings->GetResolutionScaleInformationEx(ScalePct, ScaleNormalized, ScaleMin, ScaleMax);
   const FIntPoint Res = Settings->GetScreenResolution();
   // Short, readable values leave room for real mouse +/- targets on every row.
   const FString ResLabel = Res == FIntPoint(1280,720) ? TEXT("720p")
     : Res == FIntPoint(1920,1080) ? TEXT("1080p")
     : Res == FIntPoint(2560,1440) ? TEXT("1440p")
     : Res == FIntPoint(3840,2160) ? TEXT("4K UHD")
     : FString::Printf(TEXT("%dx%d"), Res.X, Res.Y);
   const int32 Quality = FMath::Clamp(Settings->GetOverallScalabilityLevel(), 0, 3);
   const FString QualityName[] = {TEXT("Low"), TEXT("Med"), TEXT("High"), TEXT("Epic")};
   const FString FrameStatus = Hero->FrameGenerationPreference == 0 ? TEXT("Off")
     : Hero->FrameGenerationPreference == 1 ? TEXT("Auto") : TEXT("On");
   const FString Names[] = {TEXT("Resolution"), TEXT("Render scale"), TEXT("Quality"),
     TEXT("Frame gen"), TEXT("Reduced motion"), TEXT("Combat text"), TEXT("Boss alerts"),
     TEXT("Display mode")};
   const FString CompactNames[] = {TEXT("Res"), TEXT("Scale"), TEXT("Quality"),
     TEXT("Frame"), TEXT("Motion"), TEXT("Text"), TEXT("Alerts"), TEXT("Mode")};
   const FString Values[] = {ResLabel, FString::Printf(TEXT("%.0f%%"), ScalePct),
     QualityName[Quality], FrameStatus, Hero->bReduceMotion ? TEXT("On") : TEXT("Off"),
     FString::Printf(TEXT("%.0f%%"), Hero->CombatTextScale * 100.f),
     FString::Printf(TEXT("%.0f%%"), Hero->BossWarningScale * 100.f),
     Settings->GetFullscreenMode() == EWindowMode::Windowed ? TEXT("Windowed") : Settings->GetFullscreenMode() == EWindowMode::WindowedFullscreen ? TEXT("Borderless") : TEXT("Fullscreen")};
   const float MinusX = MenuLeft + MenuWidth - 145.f;
   const float PlusX = MenuLeft + MenuWidth - 37.f;
   // Options mouse hover is visual only; clicks still choose and adjust explicitly.
   float OptionsCursorX = -1.f, OptionsCursorY = -1.f;
   bool bOptionsCursorVisible = false;
   if (const APlayerController* PC = Cast<APlayerController>(Hero->GetController()))
    bOptionsCursorVisible = PC->bShowMouseCursor
      && PC->GetMousePosition(OptionsCursorX, OptionsCursorY);
   for (int32 i = 0; i < 8; ++i) {
    const float Y = FirstRowY + i * MenuRowGap;
    const bool bSelected = Hero->OptionsSelection == i;
    const bool bHoveredRow = bOptionsCursorVisible
      && OptionsCursorX >= MenuLeft + 10.f && OptionsCursorX <= MenuLeft + MenuWidth - 10.f
      && OptionsCursorY >= Y && OptionsCursorY <= Y + RowHeight;
    DrawRect(bSelected ? FLinearColor(0.22f,0.18f,0.36f,0.98f)
                       : bHoveredRow ? FLinearColor(0.15f,0.18f,0.29f,0.96f)
                       : FLinearColor(0.09f,0.08f,0.16f,0.94f),
       MenuLeft + 10.f, Y, MenuWidth - 20.f, RowHeight);
    if (bSelected) DrawSelectionRail(MenuLeft+10.f,Y,RowHeight);
    DrawHoverFrame(MenuLeft+10.f,Y,MenuWidth-20.f,RowHeight);
    if (!bCompactOptions && MenuWidth>=480.f && RowHeight>=24.f)
     DrawMoonfallSigil(MenuLeft+25.f,Y+RowHeight*0.5f,7.f,
       FLinearColor(0.54f,0.73f,0.91f),i);
    DrawText(bCompactOptions ? CompactNames[i] : Names[i], bSelected ? FLinearColor(0.96f,0.86f,0.56f) : FLinearColor::White,
       MenuLeft + (bCompactOptions?17.f:38.f), Y + 3.f, nullptr, bCompactOptions ? 0.65f : 0.85f);
    const float ButtonHeight = FMath::Min(26.f, RowHeight);
    const bool bMinusHover = bOptionsCursorVisible && OptionsCursorX >= MinusX
      && OptionsCursorX <= MinusX + 25.f && OptionsCursorY >= Y
      && OptionsCursorY <= Y + ButtonHeight;
    const bool bPlusHover = bOptionsCursorVisible && OptionsCursorX >= PlusX
      && OptionsCursorX <= PlusX + 25.f && OptionsCursorY >= Y
      && OptionsCursorY <= Y + ButtonHeight;
    const FLinearColor IdleButton(0.20f,0.25f,0.37f,0.98f);
    const FLinearColor HoverButton(0.35f,0.45f,0.60f,0.98f);
    DrawRect(bMinusHover ? HoverButton : IdleButton, MinusX, Y, 25.f, ButtonHeight);
    DrawRect(bPlusHover ? HoverButton : IdleButton, PlusX, Y, 25.f, ButtonHeight);
    DrawHoverFrame(MinusX,Y,25.f,ButtonHeight);
    DrawHoverFrame(PlusX,Y,25.f,ButtonHeight);
    DrawText(TEXT("-"), FLinearColor::White, MinusX + 9.f, Y + 2.f, nullptr, 0.95f);
    DrawText(TEXT("+"), FLinearColor::White, PlusX + 7.f, Y + 2.f, nullptr, 0.95f);
    DrawText(Values[i], FLinearColor(0.75f,0.90f,1.f), MinusX + 31.f, Y + 3.f,
      nullptr, bCompactOptions ? 0.57f : 0.72f);
   }
  }
  // Show concise, setting-specific help without covering the rows on small screens.
  const FString SettingHelp[] = {
    TEXT("Choose an available screen resolution"),
    TEXT("Adjust image sharpness and performance"),
    TEXT("Higher quality may reduce frame rate"),
    TEXT("Frame generation is a preference only; no runtime integration."),
    TEXT("Reduce impact shake and camera motion"),
    TEXT("Scale damage numbers in combat"),
    TEXT("Scale the visibility of boss warnings"),
    TEXT("Choose Windowed, Borderless or Fullscreen")
  };
  const FString CompactHelp[] = {
    TEXT("Screen resolution"), TEXT("Render scaling"), TEXT("Graphics preset"),
    TEXT("FG preference only (inactive)"), TEXT("Less camera motion"),
    TEXT("Damage text size"), TEXT("Boss alert size"), TEXT("Window mode")
  };
  const int32 HelpIndex = FMath::Clamp(Hero->OptionsSelection, 0, 7);
  FString HelpText = bCompactOptions ? CompactHelp[HelpIndex] : SettingHelp[HelpIndex];
  // Clarify 4K UHD's 3840x2160 output size and how render scale affects cost.
  // This is guidance, never a hidden automatic graphics-setting change.
  if (Settings && Settings->GetScreenResolution() == FIntPoint(3840,2160)) {
   if (HelpIndex == 0)
    HelpText = bCompactOptions ? TEXT("4K UHD: 3840x2160") : TEXT("4K UHD: 3840 x 2160 output; use render scale for performance");
   else if (HelpIndex == 1)
    HelpText = bCompactOptions ? TEXT("4K render scale: quality vs FPS")
      : TEXT("4K render scale: 100% is sharpest; reduce for performance");
  }
  DrawText(HelpText, FLinearColor(0.74f,0.81f,0.91f), MenuLeft + 15.f,
     MenuTop + MenuHeight - 28.f, nullptr, bCompactOptions ? 0.60f : 0.72f);
  DrawLiveMenuHitCue();
  return;
 }
 if (Hero->bInventoryOpen) {
  if (Canvas->SizeX < 304.f || Canvas->SizeY < 304.f) {
   DrawText(Hero->bCharacterStatsOpen
     ? TEXT("CHARACTER | Enlarge window | ESC: back")
     : TEXT("INVENTORY | Enlarge window | B / ESC: close"),
     FLinearColor(1.f,0.87f,0.70f),12.f,14.f,nullptr,0.72f);
   DrawLiveMenuHitCue();
   return;
  }
  // Canvas-only continuous inventory: no extra actors, widgets, or hidden controls.
  const float PanelWidth = FMath::Min(720.f, FMath::Max(280.f, FMath::Min(Canvas->SizeX - 24.f, Canvas->SizeX * 0.48f)));
  const float PanelHeight = FMath::Min(860.f, FMath::Max(280.f, Canvas->SizeY - 24.f));
  // Dock the inventory and character pane at the right, keeping the world visible to the left.
  const float Left = Canvas->SizeX - PanelWidth - 12.f;
  const float Top = (Canvas->SizeY - PanelHeight) * 0.5f;
  DrawRect(FLinearColor(0.04f, 0.035f, 0.10f, 0.96f), Left, Top, PanelWidth, PanelHeight);
  DrawModalChrome(Left,Top,PanelWidth,PanelHeight,FLinearColor(0.43f,0.52f,0.66f,0.98f));
  if (PanelWidth>=360.f) DrawMoonfallSigil(Left+PanelWidth-23.f,Top+PanelHeight-23.f,
    10.f,FLinearColor(0.84f,0.72f,0.47f),2);
  // The character and full-screen grimoire each paint their own title.
  // Do not draw an underlying inventory label over the active page heading.
  if (!Hero->bCharacterStatsOpen && !Hero->bSpellBookOpen)
   DrawText(PanelWidth < 550.f ? TEXT("INVENTORY") : TEXT("MOONFALL  |  INVENTORY"),
    FLinearColor::White, Left + 18.f, Top + 16.f, nullptr, PanelWidth < 550.f ? 0.93f : 1.3f);
  // Shared close button is visible on inventory and character views.
  const float CloseSortWidth = PanelWidth < 550.f ? 105.f : 172.f;
  const float CloseSortX = Left + PanelWidth - CloseSortWidth - 14.f;
  const float CloseButtonX = Hero->bCharacterStatsOpen ? Left + PanelWidth - 42.f : CloseSortX - 36.f;
  DrawRect(FLinearColor(0.37f,0.16f,0.22f,0.96f), CloseButtonX, Top + 14.f, 28.f, 28.f);
  DrawHoverFrame(CloseButtonX,Top+14.f,28.f,28.f);
  DrawText(TEXT("X"), FLinearColor::White, CloseButtonX + 9.f, Top + 18.f, nullptr, 0.9f);
  if (Hero->bCharacterStatsOpen) {
   // Character sheet is a read-only page. Inventory operations remain locked
   // while it is open; controller and keyboard switch pages with the same input.
   const TCHAR* CharacterTitle = Hero->bCharacterRepPageOpen
     ? (PanelWidth < 550.f ? TEXT("REPUTATION") : TEXT("MOONFALL  |  REPUTATION"))
     : Hero->bCharacterRacePageOpen
       ? (PanelWidth < 550.f ? TEXT("ANCESTRY") : TEXT("MOONFALL  |  ANCESTRY"))
       : (PanelWidth < 550.f ? TEXT("CHARACTER") : TEXT("MOONFALL  |  CHARACTER"));
   DrawText(CharacterTitle, FLinearColor::White,
     Left + 18.f, Top + 17.f, nullptr, PanelWidth < 550.f ? 0.80f : 1.05f);
   DrawText(FString::Printf(TEXT("Level %d  |  XP %d / %d  |  Salvage %d"),
      Hero->Level, Hero->Experience, Hero->ExperienceToNextLevel, Hero->Salvage),
      FLinearColor(0.78f,0.83f,1.f), Left + 18.f, Top + 57.f, nullptr, 0.95f);
   // Three clickable tabs: combined Stats & Gear, Ancestry, Reputation.
   const float PageTabWidth = (PanelWidth - 60.f) / 3.f;
   const float PageTabLeft = Left + 18.f;
   const float PageTabTop = Top + 77.f;
   const int32 ActivePage = Hero->bCharacterRepPageOpen ? 2
      : (Hero->bCharacterRacePageOpen ? 1 : 0);
   const TCHAR* TabLabels[] = {TEXT("STATS & GEAR"), TEXT("ANCESTRY & PASSIVES"), TEXT("REPUTATION")};
   const TCHAR* CompactTabLabels[] = {TEXT("STATS+GEAR"), TEXT("RACE"), TEXT("REP")};
   for (int32 Tab = 0; Tab < 3; ++Tab) {
    const float TabX = PageTabLeft + Tab * (PageTabWidth + 8.f);
    const bool bSelected = Tab == ActivePage;
    DrawRect(bSelected
      ? FLinearColor(0.21f,0.38f,0.39f,0.98f)
      : FLinearColor(0.12f,0.17f,0.24f,0.95f),
      TabX,PageTabTop,PageTabWidth,32.f);
    DrawRect(bSelected ? FLinearColor(0.89f,0.70f,0.36f,1.f)
      : FLinearColor(0.27f,0.32f,0.40f,0.9f),
      TabX,PageTabTop+29.f,PageTabWidth,3.f);
    DrawHoverFrame(TabX,PageTabTop,PageTabWidth,32.f);
    DrawText(PanelWidth < 480.f ? CompactTabLabels[Tab] : TabLabels[Tab],
      bSelected ? FLinearColor(1.f,0.89f,0.64f) : FLinearColor(0.85f,0.89f,0.95f),
      TabX+6.f,PageTabTop+7.f,nullptr,
      PanelWidth<480.f ? 0.61f : (Tab==1 ? 0.51f : 0.70f));
   }
   const float StartY = Top + 123.f;
   if (Hero->bCharacterRacePageOpen) {
    // This is the saved 12-race gameplay ancestry, not the 30-entry
    // world-race browsing catalog. Never imply an unchosen race has bonuses.
    const bool Chosen = Hero->bRaceChosen;
    const float ReadyIn = Chosen ? Hero->GetRacePowerCooldownRemaining() : 0.f;
    const float ActiveFor = Chosen ? Hero->GetRacePowerDurationRemaining() : 0.f;
    const FString RacePowerStatus = !Chosen ? TEXT("LOCKED")
      : ActiveFor>0.f ? FString::Printf(TEXT("ACTIVE  %.1fs"),ActiveFor)
      : ReadyIn>0.f ? FString::Printf(TEXT("COOLDOWN  %.1fs"),ReadyIn)
      : TEXT("READY");
    const FString RaceLines[] = {
      TEXT("SAVED HERO ANCESTRY"),
      FString::Printf(TEXT("%s  |  %s"), *Hero->GetRaceName(),
        Chosen ? TEXT("CHOSEN - PERMANENT") : TEXT("NOT YET CHOSEN")),
      TEXT("PASSIVE ATTRIBUTE BONUSES"),
      FString::Printf(TEXT("Might +%d  |  Insight +%d  |  Resolve +%d"),
        Hero->GetRaceMightBonus(),Hero->GetRaceInsightBonus(),
        Hero->GetRaceResolveBonus()),
      FString::Printf(TEXT("Stamina regen +%.0f%%  |  Mana regen +%.0f%%"),
        static_cast<double>(Hero->GetRaceStaminaRegenBonus())*100.0,
        static_cast<double>(Hero->GetRaceManaRegenBonus())*100.0),
      TEXT("PASSIVE PROFILE"),
      Hero->GetRacePassiveSummary(),
      TEXT("RACIAL POWER"),
      FString::Printf(TEXT("%s  |  %s"),
        Chosen ? *Hero->GetRacePowerName() : TEXT("LOCKED"),
        *RacePowerStatus),
      Hero->GetRacePowerDescription(),
      TEXT("POWER RECHARGE"),
      Chosen ? TEXT("45-second cooldown after activation.")
        : TEXT("No racial power available until a race is chosen."),
      TEXT("CHARACTER CREATION"),
      TEXT("Ancestry choice is saved and cannot be changed here."),
      TEXT("Select STATS & GEAR to view your equipment and attributes")
    };
    const float RaceRowHeight = FMath::Clamp(
      (PanelHeight - 178.f) / UE_ARRAY_COUNT(RaceLines), 9.f, 32.f);
    for (int32 Row=0; Row<UE_ARRAY_COUNT(RaceLines); ++Row) {
      const bool bSection = Row==0 || Row==2 || Row==5 || Row==7 || Row==10 || Row==12;
      const float RowY = StartY+Row*RaceRowHeight;
      // Small windows must not paint over the footer or escape the panel.
      if (RowY+RaceRowHeight > Top+PanelHeight-65.f) break;
      DrawRect(bSection ? FLinearColor(0.19f,0.15f,0.10f,0.96f)
        : FLinearColor(0.08f,0.11f,0.15f,0.76f),
        Left+15.f,RowY-2.f,PanelWidth-30.f,FMath::Max(7.f,RaceRowHeight-1.f));
      if (bSection)
       DrawRect(FLinearColor(0.70f,0.51f,0.22f,0.95f),
         Left+15.f,RowY-2.f,3.f,FMath::Max(7.f,RaceRowHeight-1.f));
      DrawText(RaceLines[Row],
        bSection ? FLinearColor(1.f,0.79f,0.42f) : FLinearColor(0.92f,0.94f,0.97f),
        Left+23.f,RowY,nullptr,
        PanelHeight<360.f ? 0.52f : (PanelWidth<420.f?0.63f:0.81f));
    }
   } else if (Hero->bCharacterRepPageOpen) {
    // Five canonical factions, from existing V4+ save data. This view never
    // modifies reputation or world state, and does no actor/world scanning.
    const FName FactionIds[] = {
      FName(TEXT("Wanderers")),FName(TEXT("VerdantCircle")),
      FName(TEXT("StoneHearth")),FName(TEXT("MoonCoven")),
      FName(TEXT("AshenClans"))
    };
    const TCHAR* FactionNames[] = {
      TEXT("Wanderers"),TEXT("Verdant Circle"),TEXT("Stone Hearth"),
      TEXT("Moon Coven"),TEXT("Ashen Clans")
    };
    const float DetailsReserve = PanelHeight>=620.f ? 158.f : 0.f;
    const float RowsBottom = Top+PanelHeight-65.f-DetailsReserve;
    const float ReputationRowHeight = FMath::Clamp(
      (RowsBottom-StartY)/5.f,18.f,74.f);
    const int32 SelectedFaction = FMath::Clamp(Hero->CharacterRepSelection,0,4);
    int32 FriendlyCount=0,HostileCount=0,TrustedCount=0;
    for (int32 FactionIndex=0;FactionIndex<5;++FactionIndex) {
      const int32 Score=Hero->GetFactionReputation(FactionIds[FactionIndex]);
      if (Score<=-50) ++HostileCount;
      if (Score>=15) ++FriendlyCount;
      if (Score>=50) ++TrustedCount;
      const float Y=StartY+FactionIndex*ReputationRowHeight;
      if (Y+ReputationRowHeight>RowsBottom) break;
      // Exact NPC dialogue and resident quest-gating thresholds.
      const TCHAR* Standing=Score<=-50 ? TEXT("HOSTILE")
        : Score<=-15 ? TEXT("WARY")
        : Score>=50 ? TEXT("TRUSTED")
        : Score>=15 ? TEXT("FRIENDLY") : TEXT("NEUTRAL");
      const FLinearColor StandingColor=Score<=-50
        ? FLinearColor(0.99f,0.40f,0.39f)
        : Score<=-15 ? FLinearColor(0.98f,0.70f,0.38f)
        : Score>=50 ? FLinearColor(0.99f,0.85f,0.42f)
        : Score>=15 ? FLinearColor(0.45f,0.92f,0.59f)
        : FLinearColor(0.76f,0.82f,0.90f);
      const bool bSelected=FactionIndex==SelectedFaction;
      DrawRect(bSelected ? FLinearColor(0.17f,0.23f,0.34f,0.98f)
        : (FactionIndex%2==0 ? FLinearColor(0.08f,0.11f,0.16f,0.95f)
        : FLinearColor(0.11f,0.13f,0.19f,0.95f)),
        Left+15.f,Y,PanelWidth-30.f,ReputationRowHeight-3.f);
      DrawRect(bSelected ? FLinearColor(0.96f,0.82f,0.46f) : StandingColor,
        Left+15.f,Y,bSelected?5.f:3.f,ReputationRowHeight-3.f);
      if (ReputationRowHeight>=29.f && PanelWidth>=410.f)
       DrawFactionCrest(Left+PanelWidth-30.f,Y+13.f,9.f,FactionIndex,StandingColor);
      DrawHoverFrame(Left+15.f,Y,PanelWidth-30.f,ReputationRowHeight-3.f);
      DrawText(FString::Printf(TEXT("%s  |  %s  |  %+d"),
        FactionNames[FactionIndex],Standing,Score),StandingColor,
        Left+25.f,Y+4.f,nullptr,PanelWidth<440.f?0.58f:0.82f);
      // Next standing thresholds are inclusive and match NPC behavior.
      if (ReputationRowHeight>=36.f) {
        const int32 Next=Score < -49 ? -49
          : Score < -14 ? -14 : Score < 15 ? 15
          : Score < 50 ? 50 : 100;
        const TCHAR* NextRank=Score < -49 ? TEXT("WARY")
          : Score < -14 ? TEXT("NEUTRAL") : Score < 15 ? TEXT("FRIENDLY")
          : Score < 50 ? TEXT("TRUSTED") : TEXT("MAX");
        DrawText(Score>=100 ? TEXT("MAXIMUM STANDING")
          : FString::Printf(TEXT("%d TO %s"),Next-Score,NextRank),
          FLinearColor(0.76f,0.81f,0.88f),
          Left+25.f,Y+22.f,nullptr,PanelWidth<440.f?0.51f:0.68f);
        // -100..+100 with zero clearly marked; Neutral is not an empty bar.
        const float Fraction=FMath::Clamp((Score+100.f)/200.f,0.f,1.f);
        const float BarX=Left+25.f, BarWidth=PanelWidth-50.f;
        const float BarY=Y+ReputationRowHeight-12.f;
        DrawRect(FLinearColor(0.16f,0.19f,0.25f,1.f),
          BarX,BarY,BarWidth,6.f);
        DrawRect(StandingColor,BarX,BarY,BarWidth*Fraction,6.f);
        DrawRect(FLinearColor(0.80f,0.82f,0.85f,0.86f),
          BarX+BarWidth*0.5f,BarY-2.f,2.f,10.f);
      }
    }
    // Context panel only when sufficiently tall, never hiding the footer.
    if (PanelHeight>=620.f) {
      const int32 Score=Hero->GetFactionReputation(FactionIds[SelectedFaction]);
      const int32 RankMin=Score<=-50 ? -100 : Score<=-15 ? -49
        : Score<15 ? -14 : Score<50 ? 15 : 50;
      const int32 RankMax=Score<=-50 ? -50 : Score<=-15 ? -15
        : Score<15 ? 14 : Score<50 ? 49 : 100;
      const float RankProgress=FMath::Clamp(
        float(Score-RankMin)/float(FMath::Max(1,RankMax-RankMin)),0.f,1.f);
      const bool bQuestBlocked=Score<=-50;
      const bool bNoSavedChange=!Hero->FactionReputation.Contains(FactionIds[SelectedFaction]);
      const float DetailY=StartY+5.f*ReputationRowHeight+11.f;
      const float DetailHeight=FMath::Max(0.f,Top+PanelHeight-66.f-DetailY);
      DrawRect(FLinearColor(0.07f,0.10f,0.15f,0.97f),
        Left+15.f,DetailY,PanelWidth-30.f,DetailHeight);
      DrawRect(FLinearColor(0.83f,0.66f,0.32f,0.96f),
        Left+15.f,DetailY,PanelWidth-30.f,3.f);
      DrawText(FString::Printf(TEXT("%s  |  FACTION DETAILS"),
        FactionNames[SelectedFaction]),FLinearColor(1.f,0.85f,0.54f),
        Left+25.f,DetailY+9.f,nullptr,PanelWidth<440.f?0.61f:0.88f);
      DrawText(bQuestBlocked ? TEXT("RESIDENT QUESTS: BLOCKED (HOSTILE)")
        : TEXT("RESIDENT QUESTS: ALLOWED IF AVAILABLE"),
        bQuestBlocked ? FLinearColor(0.98f,0.52f,0.45f)
          : FLinearColor(0.51f,0.93f,0.65f),
        Left+25.f,DetailY+35.f,nullptr,PanelWidth<440.f?0.54f:0.72f);
      DrawText(FString::Printf(TEXT("CURRENT RANK: %d TO %d  |  SCORE %+d"),
        RankMin,RankMax,Score),FLinearColor(0.82f,0.86f,0.94f),
        Left+25.f,DetailY+58.f,nullptr,PanelWidth<440.f?0.51f:0.71f);
      DrawRect(FLinearColor(0.17f,0.20f,0.26f,0.98f),
        Left+25.f,DetailY+78.f,PanelWidth-50.f,8.f);
      DrawRect(FLinearColor(0.79f,0.62f,0.35f,0.98f),
        Left+25.f,DetailY+78.f,(PanelWidth-50.f)*RankProgress,8.f);
      if (DetailHeight>=126.f)
        DrawText(bNoSavedChange ? TEXT("No saved change yet: Neutral baseline.")
          : TEXT("Standing persists with your world save."),
          FLinearColor(0.76f,0.82f,0.89f),
          Left+25.f,DetailY+98.f,nullptr,PanelWidth<440.f?0.52f:0.68f);
      if (DetailHeight>=155.f)
        DrawText(FString::Printf(TEXT("RELATIONS: %d FRIENDLY+  |  %d TRUSTED  |  %d HOSTILE"),
          FriendlyCount,TrustedCount,HostileCount),FLinearColor(0.82f,0.86f,0.94f),
          Left+25.f,DetailY+122.f,nullptr,PanelWidth<440.f?0.49f:0.70f);
    }
   } else {
   const FString CharacterLines[] = {
     FString::Printf(TEXT("Equipment power: %d  |  W %d / A %d / T %d"),
       Hero->GetEquipmentPower(), Hero->GetItemPower(Hero->GetEquipmentSlotCode(0)),
       Hero->GetItemPower(Hero->GetEquipmentSlotCode(1)),
       Hero->GetItemPower(Hero->GetEquipmentSlotCode(2))),
     TEXT("PRIMARY ATTRIBUTES"),
     FString::Printf(TEXT("Might: %d    |    +%.1f%% hostile spell power"),
       Hero->GetMight(), FMath::Min(Hero->GetMight() * 0.1f, 10.f)),
     FString::Printf(TEXT("Insight: %d    |    +%.1f%% mana regeneration"),
       Hero->GetInsight(), FMath::Min(Hero->GetInsight() * 0.2f, 20.f)),
     FString::Printf(TEXT("Resolve: %d    |    +%.1f%% stamina regeneration"),
       Hero->GetResolve(), FMath::Min(Hero->GetResolve() * 0.3f, 30.f)),
     TEXT("SECONDARY STATS"),
     FString::Printf(TEXT("Critical Strike: %.1f%%  |  Haste: %.1f%%"),
       Hero->GetCriticalChance() * 100.f, Hero->GetHasteBonus() * 100.f),
     FString::Printf(TEXT("Mastery: %.1f%%  |  Versatility: %.1f%%"),
       Hero->GetMasteryBonus() * 100.f, Hero->GetVersatilityBonus() * 100.f),
     TEXT("LEGENDARY EQUIPMENT PERKS"),
     FString::Printf(TEXT("Weapon: %s  |  critical spell damage x1.65"),
       Hero->HasLegendaryPerk(0) ? TEXT("ACTIVE") : TEXT("LOCKED")),
     FString::Printf(TEXT("Armor: %s  |  +1.5%% protection (34%% cap)"),
       Hero->HasLegendaryPerk(1) ? TEXT("ACTIVE") : TEXT("LOCKED")),
     FString::Printf(TEXT("Accessory: %s  |  +2.5%% haste (20%% cap)"),
       Hero->HasLegendaryPerk(2) ? TEXT("ACTIVE") : TEXT("LOCKED")),
     TEXT("EQUIPMENT SET BONUSES"),
     FString::Printf(TEXT("Force %d/3: 2pc +3%% crit; 3pc +4%% mastery"),
       Hero->GetSetPieceCount(1)),
     FString::Printf(TEXT("Ward %d/3: 2pc +3%% vers; 3pc +2.5%% protection"),
       Hero->GetSetPieceCount(2)),
     FString::Printf(TEXT("Focus %d/3: 2pc +4%% haste; 3pc +6%% mana regen"),
       Hero->GetSetPieceCount(3)),
     TEXT("DUNGEON CAMPAIGN"),
     FString::Printf(TEXT("Bosses %d / 30  |  Zones cleared %d / 6  |  Entries %d / 30"),
       Hero->GetTotalDefeatedDungeonBosses(),
       Hero->GetClearedDungeonZoneCount(), Hero->GetTotalDiscoveredDungeons())
   };
   // Stats and owned equipped gear share a single WoW-style character page.
   // Reserve visible room for the three real slots at every supported height.
   const bool bCompactStats = PanelHeight < 570.f;
   const float StatsBottom = Top + PanelHeight - (bCompactStats ? 130.f : 144.f);
   const float StatsRowHeight = FMath::Clamp(
     (StatsBottom-StartY)/(bCompactStats ? 8.f : 18.f),9.f,24.f);
   int32 ShownStatsRows = 0;
   const float StatPanelLeft = Left + 15.f;
   const float StatPanelWidth = PanelWidth - 30.f;
   for (int32 Row = 0; Row < UE_ARRAY_COUNT(CharacterLines); ++Row) {
    const bool bSection = Row == 1 || Row == 5 || Row == 8 || Row == 12 || Row == 16;
    if (bCompactStats && !(Row==0 || Row==2 || Row==3 || Row==4
      || Row==6 || Row==7 || Row==13 || Row==17)) continue;
    const float RowY = StartY + ShownStatsRows * StatsRowHeight;
    if (RowY+StatsRowHeight > StatsBottom) break;
    ++ShownStatsRows;
    DrawRect(bSection
       ? FLinearColor(0.19f,0.15f,0.10f,0.96f)
       : (Row % 2 == 0
         ? FLinearColor(0.09f,0.11f,0.16f,0.82f)
         : FLinearColor(0.07f,0.09f,0.13f,0.72f)),
       StatPanelLeft,RowY-2.f,StatPanelWidth,FMath::Max(7.f,StatsRowHeight-1.f));
    if (bSection)
     DrawRect(FLinearColor(0.70f,0.51f,0.22f,0.95f),
        StatPanelLeft,RowY-2.f,3.f,FMath::Max(7.f,StatsRowHeight-1.f));
    if (bSection && PanelWidth>=470.f && StatsRowHeight>=18.f)
     DrawMoonfallSigil(Left+PanelWidth-28.f,RowY+6.f,6.f,
       FLinearColor(0.89f,0.71f,0.43f),Row/4);
    DrawText(CharacterLines[Row],
      bSection ? FLinearColor(1.f,0.79f,0.42f) : FLinearColor(0.92f,0.94f,0.97f),
      Left + 23.f, RowY, nullptr,
      PanelHeight < 360.f ? 0.55f : (Canvas->SizeX < 900.f ? 0.75f : 0.90f));
   }
   // Actual Weapon, Armor and Accessory records, validated by combat getters.
   // Read-only: all equipping and item comparison remains in Inventory.
   const float GearTop=StartY+ShownStatsRows*StatsRowHeight+6.f;
   const float GearBottom=Top+PanelHeight-65.f;
   if (GearBottom-GearTop >= 17.f) {
    DrawText(TEXT("EQUIPPED GEAR"),FLinearColor(1.f,0.79f,0.42f),
      Left+23.f,GearTop,nullptr,PanelWidth<440.f?0.57f:0.78f);
    const float CardsTop=GearTop+16.f;
    const float CardsAvailable=GearBottom-CardsTop;
    const int32 Codes[] = {Hero->GetEquipmentSlotCode(0),
      Hero->GetEquipmentSlotCode(1),Hero->GetEquipmentSlotCode(2)};
    const TCHAR* SlotNames[] = {TEXT("WEAPON"),TEXT("ARMOR"),TEXT("ACCESSORY")};
    const TCHAR* RarityNames[] = {TEXT("COMMON"),TEXT("UNCOMMON"),TEXT("RARE"),
      TEXT("EPIC"),TEXT("EMERALD"),TEXT("LEGENDARY")};
    const TCHAR* TraitNames[] = {TEXT("NONE"),TEXT("FORCE"),TEXT("WARD"),TEXT("FOCUS")};
    const FLinearColor RarityColors[] = {
      FLinearColor(0.85f,0.85f,0.85f),FLinearColor(0.36f,0.72f,1.f),
      FLinearColor(1.f,0.83f,0.33f),FLinearColor(0.76f,0.45f,0.98f),
      FLinearColor(0.31f,0.91f,0.49f),FLinearColor(1.f,0.58f,0.23f)
    };
    if (CardsAvailable >= 34.f) {
     const float ResourceReserve=PanelHeight>=760.f ? 60.f : 0.f;
     const float CardHeight=FMath::Clamp(
       (CardsAvailable-ResourceReserve-6.f)/3.f,10.f,46.f);
     for (int32 Slot=0;Slot<3;++Slot) {
      const int32 Tier=Hero->GetValidatedEquippedTier(Slot);
      const bool bEquipped=Tier>0;
      const int32 Rarity=bEquipped ? (Codes[Slot]/1000)%10 : 0;
      const int32 Trait=bEquipped ? Codes[Slot]/100000 : 0;
      const bool bValid=bEquipped && Rarity>=0 && Rarity<6
        && Trait>=0 && Trait<4;
      const float Y=CardsTop+Slot*(CardHeight+3.f);
      if (Y+CardHeight>GearBottom-ResourceReserve+1.f) break;
      DrawRect(FLinearColor(0.08f,0.11f,0.16f,0.95f),
        Left+15.f,Y,PanelWidth-30.f,CardHeight);
      DrawRect(bValid ? RarityColors[Rarity] : FLinearColor(0.36f,0.39f,0.45f),
        Left+15.f,Y,4.f,CardHeight);
      const FString Label=bValid
        ? FString::Printf(TEXT("%s  T%d %s  %s  |  POWER %d"),
          SlotNames[Slot],Tier,RarityNames[Rarity],TraitNames[Trait],
          Hero->GetItemPower(Codes[Slot]))
        : FString::Printf(TEXT("%s  |  EMPTY"),SlotNames[Slot]);
      if (CardHeight>=25.f && PanelWidth>=390.f)
       DrawGearGlyph(Left+PanelWidth-29.f,Y+CardHeight*0.5f,
         10.f,Slot,bValid ? RarityColors[Rarity] : FLinearColor(0.38f,0.41f,0.49f));
      DrawText(Label,bValid ? RarityColors[Rarity]
        : FLinearColor(0.75f,0.78f,0.83f),
        Left+24.f,Y+1.f,nullptr,PanelWidth<440.f?0.48f:0.69f);
      if (CardHeight>=32.f)
       DrawText(bValid && Hero->HasLegendaryPerk(Slot)
         ? TEXT("LEGENDARY PERK ACTIVE")
         : TEXT("Equip upgrades in Inventory"),
         FLinearColor(0.78f,0.86f,0.92f),Left+24.f,Y+22.f,
         nullptr,PanelWidth<440.f?0.47f:0.60f);
     }
     if (PanelHeight>=760.f) {
      // Retain the live HP/mana/stamina view from the former gear page.
      const float ResourceTop=CardsTop+3.f*(CardHeight+3.f)+2.f;
      const float Current[] = {Hero->Health,Hero->Mana,Hero->Stamina};
      const float Max[] = {Hero->MaxHealth,Hero->MaxMana,Hero->MaxStamina};
      const TCHAR* Names[] = {TEXT("HP"),TEXT("MP"),TEXT("STAMINA")};
      const FLinearColor Colors[] = {
        FLinearColor(0.78f,0.22f,0.26f),FLinearColor(0.27f,0.53f,0.88f),
        FLinearColor(0.68f,0.52f,0.21f)
      };
      for (int32 R=0;R<3;++R) {
       const float SafeMax=FMath::IsFinite(Max[R])?FMath::Max(0.f,Max[R]):0.f;
       const float Safe=FMath::IsFinite(Current[R])
         ? FMath::Clamp(Current[R],0.f,SafeMax):0.f;
       const float Fraction=SafeMax>0.f?Safe/SafeMax:0.f;
       const float Y=ResourceTop+R*18.f;
       if (Y+14.f>GearBottom) break;
       DrawText(FString::Printf(TEXT("%s %.0f/%.0f"),Names[R],Safe,SafeMax),
         FLinearColor(0.86f,0.90f,0.95f),Left+24.f,Y,nullptr,0.55f);
       DrawRect(FLinearColor(0.13f,0.17f,0.22f,0.98f),
         Left+160.f,Y+3.f,PanelWidth-190.f,7.f);
       DrawRect(Colors[R],Left+160.f,Y+3.f,
         (PanelWidth-190.f)*Fraction,7.f);
      }
     }
    } else {
     DrawText(FString::Printf(TEXT("W %d  |  A %d  |  T %d"),
       Hero->GetValidatedEquippedTier(0),Hero->GetValidatedEquippedTier(1),
       Hero->GetValidatedEquippedTier(2)),FLinearColor(0.83f,0.90f,0.97f),
       Left+23.f,GearTop+12.f,nullptr,0.62f);
    }
   }

   }
   DrawText(TEXT("C / D-pad Left/Right: stats & gear / ancestry / rep"),FLinearColor(0.69f,0.89f,0.75f),
      Left+18.f,Top+PanelHeight-53.f,nullptr,PanelWidth<460.f?0.62f:0.8f);
   DrawText(TEXT("A/K: talents   J/R3: atlas   Esc/Pad B: back   Select: close"),
     FLinearColor(0.81f,0.83f,0.97f), Left + 18.f, Top + PanelHeight - 29.f,
     nullptr, 0.8f);
   DrawLiveMenuHitCue();
   return;
  }
  // The closed book's tab lives in the bag. The open book expands into a
  // centered, readable grimoire. Geometry is shared with mouse hit testing.
  const auto Book = MoonfallSpellBook::Layout(Canvas->SizeX, Canvas->SizeY, Hero->bSpellBookOpen);
  if (Hero->bSpellBookOpen) {
   // Mask the underlying bag; clicks cannot leak through the book modal.
   DrawRect(FLinearColor(0.009f,0.010f,0.025f,0.94f),
       0.f,0.f,Canvas->SizeX,Canvas->SizeY);
   DrawRect(FLinearColor(0.035f,0.031f,0.078f,0.99f),
       Book.Left,Book.Top,Book.Width,Book.Height);
   DrawRect(FLinearColor(0.43f,0.28f,0.52f,0.92f),
       Book.Left,Book.Top,Book.Width,5.f);
   DrawModalChrome(Book.Left,Book.Top,Book.Width,Book.Height,
     FLinearColor(0.58f,0.40f,0.68f,0.98f));
   if (Book.Width>=450.f) DrawMoonfallSigil(Book.Left+Book.Width-26.f,
     Book.Top+65.f,11.f,FLinearColor(0.78f,0.60f,0.99f),3);
   DrawRect(FLinearColor(0.37f,0.16f,0.22f,0.96f),Book.CloseX,Book.CloseY,Book.CloseSize,Book.CloseSize);
   DrawHoverFrame(Book.CloseX,Book.CloseY,Book.CloseSize,Book.CloseSize);
   DrawText(TEXT("X"),FLinearColor::White,Book.CloseX+9.f,Book.CloseY+4.f,nullptr,0.9f);
   DrawText(Book.Width<500.f ? TEXT("THE GRIMOIRE") : TEXT("MOONFALL  |  THE GRIMOIRE"),
       FLinearColor(0.96f,0.88f,0.76f),Book.Left+18.f,Book.Top+13.f,
       nullptr,Book.Width<500.f ? 0.88f : 1.36f);
   DrawText(FString::Printf(TEXT("SIX KNOWN SPELLS   |   MANA %.0f / %.0f"),
       FMath::IsFinite(Hero->Mana) ? Hero->Mana : 0.f,
       FMath::IsFinite(Hero->MaxMana) ? Hero->MaxMana : 0.f),
       FLinearColor(0.72f,0.82f,0.97f), Book.Left+18.f,Book.Top+47.f,
       nullptr,Book.Width<500.f ? 0.62f : 0.85f);
  }
  DrawRect(Hero->bSpellBookOpen ? FLinearColor(0.29f,0.39f,0.33f,0.96f)
      : FLinearColor(0.24f,0.20f,0.37f,0.95f),
      Book.TabX, Book.TabY, Book.TabWidth, Book.TabHeight);
  DrawHoverFrame(Book.TabX,Book.TabY,Book.TabWidth,Book.TabHeight);
  DrawText(Hero->bSpellBookOpen ? TEXT("BACK TO BAG") : TEXT("SPELL BOOK"),
      FLinearColor::White,Book.TabX+9.f,Book.TabY+5.f,
      nullptr,Book.Width<500.f ? 0.67f : 0.79f);
  if (Hero->bSpellBookOpen) {
   const int32 SelectedSpell = FMath::Clamp(Hero->SpellBookSelection,
       0,MoonfallSpellBook::SpellCount-1);
   for (int32 School=0; School<MoonfallSpellBook::SpellCount; ++School) {
    const auto& Entry = MoonfallSpellBook::Spells[School];
    const float Y = Book.RowY + School*Book.RowGap;
    const bool bSelected = SelectedSpell == School;
    const bool bLeft = Hero->LeftSpell == School;
    const bool bRight = Hero->bRightHandSpellReadied && Hero->RightSpell == School;
    DrawRect(bSelected ? FLinearColor(0.31f,0.30f,0.48f,0.99f)
        : FLinearColor(0.10f,0.12f,0.21f,0.96f),
        Book.RowX,Y,Book.RowWidth,Book.RowHeight);
    DrawRect(bSelected ? FLinearColor(0.78f,0.62f,0.37f,0.97f)
        : FLinearColor(0.27f,0.25f,0.43f,0.92f),
        Book.RowX,Y,4.f,Book.RowHeight);
    if (bSelected) DrawSelectionRail(Book.RowX,Y,Book.RowHeight);
    DrawHoverFrame(Book.RowX,Y,Book.RowWidth,Book.RowHeight);
    // Genuine original art for each of the six canonical schools.
    // Compact rows keep their original text geometry and readable labels.
    const bool bDrawSchoolIcon=Book.RowHeight>=25.f && Book.RowWidth>=350.f;
    if (bDrawSchoolIcon) DrawSpellGlyph(Book.RowX+19.f,
      Y+FMath::Min(13.f,Book.RowHeight*0.5f),9.f,School);
    const FString Name(ANSI_TO_TCHAR(Entry.Name));
    DrawText(FString::Printf(TEXT("%d  %s"),School+1,*Name),
        bSelected ? FLinearColor::White : FLinearColor(0.86f,0.88f,0.96f),
        Book.RowX+(bDrawSchoolIcon?37.f:12.f),Y+2.f,nullptr,
        Book.Width<500.f ? 0.63f : 0.94f);
    // Use rows as full selectable surfaces, never as cast buttons.
    if (Book.Width >= 700.f) {
     DrawText(FString(ANSI_TO_TCHAR(Entry.School)),
       FLinearColor(0.78f,0.77f,0.95f),
       Book.RowX+Book.RowWidth*0.42f,Y+3.f,nullptr,0.78f);
     DrawText(FString::Printf(TEXT("%.0f MANA  |  %.2fs  %s%s"),
       Entry.ManaCost,Entry.Cooldown,
       bLeft ? TEXT("[L]") : TEXT(""),
       bRight ? TEXT("[R]") : TEXT("")),
       FLinearColor(0.69f,0.87f,0.92f),
       Book.RowX+Book.RowWidth-241.f,Y+3.f,nullptr,0.72f);
     if (Book.RowHeight >= 35.f) {
      const FString Role = Entry.bHealing ? TEXT("RESTORE") : TEXT("DAMAGE");
      DrawText(Role,FLinearColor(0.67f,0.85f,0.65f),
         Book.RowX+14.f,Y+26.f,nullptr,0.67f);
     }
    } else if (Book.Width >= 440.f) {
     DrawText(FString::Printf(TEXT("%.0f MP %s%s"),Entry.ManaCost,
       bLeft ? TEXT("L") : TEXT(""),bRight ? TEXT("R") : TEXT("")),
       FLinearColor(0.69f,0.87f,0.92f),
       Book.RowX+Book.RowWidth-110.f,Y+3.f,nullptr,0.62f);
    } else if (bLeft || bRight) {
     DrawText(bLeft && bRight ? TEXT("L/R") : bLeft ? TEXT("L") : TEXT("R"),
       FLinearColor(0.68f,0.95f,0.73f),
       Book.RowX+Book.RowWidth-30.f,Y+2.f,nullptr,0.59f);
    }
   }
   // Read-only state uses the same mana and per-hand haste/cooldown formula
   // as TryCast. Hostile spells still require target acquisition at cast time.
   const auto& Selected = MoonfallSpellBook::Spells[SelectedSpell];
   const auto LeftStatus = Hero->GetSpellBookCastState(SelectedSpell,true);
   const auto RightStatus = Hero->GetSpellBookCastState(SelectedSpell,false);
   const auto StatusText = [](const MoonfallSpellBook::FCastState& State, bool bHealing)->FString {
    switch (State.State) {
     case MoonfallSpellBook::EReadiness::Ready: return bHealing ? TEXT("READY TO HEAL") : TEXT("READY TO AIM");
     case MoonfallSpellBook::EReadiness::Cooldown:
      return FString::Printf(TEXT("COOLDOWN %.1fs"),State.CooldownRemaining);
     case MoonfallSpellBook::EReadiness::LowMana:
      return FString::Printf(TEXT("NEED %.0f MANA"),State.ManaShortfall);
     case MoonfallSpellBook::EReadiness::FullHealth: return TEXT("FULL HEALTH");
     default: return TEXT("UNAVAILABLE");
    }
   };
   const auto StatusColor = [](MoonfallSpellBook::EReadiness State)->FLinearColor {
    return State==MoonfallSpellBook::EReadiness::Ready
      ? FLinearColor(0.52f,0.94f,0.66f)
      : FLinearColor(0.95f,0.70f,0.42f);
   };
   if (Book.DetailsY+33.f < Book.EquipY) {
    DrawText(FString::Printf(TEXT("%s  |  %.0f MANA  |  %.2fs BASE COOLDOWN"),
      ANSI_TO_TCHAR(Selected.School),Selected.ManaCost,Selected.Cooldown),
      FLinearColor(0.94f,0.82f,0.54f),Book.RowX,Book.DetailsY,nullptr,
      Book.Width<500.f ? 0.57f : 0.88f);
    const FString Detail(ANSI_TO_TCHAR(Selected.Description));
    DrawText(Book.Width<450.f ? Detail.Left(39) : Detail,
       FLinearColor(0.86f,0.87f,0.94f),
       Book.RowX,Book.DetailsY+23.f,nullptr,
       Book.Width<500.f ? 0.60f : 0.80f);
   }
   if (Book.DetailsY+80.f < Book.EquipY) {
    DrawText(FString::Printf(TEXT("LEFT: %s%s"),*StatusText(LeftStatus,Selected.bHealing),
      Hero->LeftSpell==SelectedSpell ? TEXT("  [EQUIPPED]") : TEXT("")),
      StatusColor(LeftStatus.State),Book.RowX,Book.DetailsY+49.f,nullptr,
      Book.Width<500.f ? 0.59f : 0.78f);
    DrawText(FString::Printf(TEXT("RIGHT: %s%s"),*StatusText(RightStatus,Selected.bHealing),
      Hero->bRightHandSpellReadied && Hero->RightSpell==SelectedSpell
       ? TEXT("  [EQUIPPED]") : TEXT("")),
      StatusColor(RightStatus.State),Book.RowX,Book.DetailsY+72.f,nullptr,
      Book.Width<500.f ? 0.59f : 0.78f);
   }
   DrawRect(FLinearColor(0.22f,0.32f,0.46f,0.98f),
      Book.EquipLeftX,Book.EquipY,Book.EquipWidth,Book.EquipHeight);
   DrawHoverFrame(Book.EquipLeftX,Book.EquipY,Book.EquipWidth,Book.EquipHeight);
   DrawRect(FLinearColor(0.24f,0.43f,0.34f,0.98f),
      Book.EquipRightX,Book.EquipY,Book.EquipWidth,Book.EquipHeight);
   DrawHoverFrame(Book.EquipRightX,Book.EquipY,Book.EquipWidth,Book.EquipHeight);
   DrawText(TEXT("EQUIP LEFT"),FLinearColor::White,
      Book.EquipLeftX+7.f,Book.EquipY+5.f,nullptr,
      Book.Width<500.f ? 0.62f : 0.85f);
   DrawText(TEXT("EQUIP RIGHT"),FLinearColor::White,
      Book.EquipRightX+7.f,Book.EquipY+5.f,nullptr,
      Book.Width<500.f ? 0.62f : 0.85f);
   if (Book.Width >= 440.f)
    DrawText(TEXT("P/Y: BACK | J/R3: ATLAS | D-PAD: SELECT / EQUIP"),
       FLinearColor(0.64f,0.76f,0.88f),Book.Left+18.f,
       Book.Top+Book.Height-14.f,nullptr,Book.Width<650.f ? 0.49f : 0.66f);
   DrawLiveMenuHitCue();
   return;
  }
  // Identical button rectangle to HitTestInventorySortButton in HeroPawn.
  const float SortWidth = PanelWidth < 550.f ? 105.f : 172.f;
  const float SortX = Left + PanelWidth - SortWidth - 14.f;
  const float SortY = Top + 14.f;
  DrawRect(FLinearColor(0.18f,0.28f,0.42f,0.96f), SortX, SortY, SortWidth, 28.f);
  DrawHoverFrame(SortX,SortY,SortWidth,28.f);
  DrawText(PanelWidth < 550.f
       ? (Hero->InventorySortMode == 0 ? TEXT("SORT: NEW") :
          Hero->InventorySortMode == 1 ? TEXT("SORT: QLTY") : TEXT("SORT: SLOT"))
       : (Hero->InventorySortMode == 0 ? TEXT("SORT: ORIGINAL") :
          Hero->InventorySortMode == 1 ? TEXT("SORT: QUALITY") : TEXT("SORT: SLOT")),
     FLinearColor::White, SortX + 7.f, SortY + 5.f, nullptr, PanelWidth < 550.f ? 0.68f : 0.78f);
  // Match HitTestInventoryEquipButton exactly: wide panels only.
  // Row clicks remain selection-only; this is a separate explicit action.
  if (PanelWidth >= 550.f && Hero->InventoryItemCodes.IsValidIndex(Hero->InventorySelection)) {
   const float EquipX = Left + PanelWidth - 186.f;
   const float EquipY = Top + 87.f;
   DrawRect(FLinearColor(0.18f,0.34f,0.28f,0.96f), EquipX, EquipY, 172.f, 30.f);
   DrawHoverFrame(EquipX,EquipY,172.f,30.f);
   DrawText(TEXT("EQUIP SELECTED"), FLinearColor::White,
     EquipX + 9.f, EquipY + 6.f, nullptr, 0.78f);
  }
  // Skyrim-style equipped-hand favorites inside the regular inventory.
  // Same 7 rectangles as HitTestRightHandFavorite; no gameplay toggle key.
  const float StripX = Left + 18.f;
  const float StripY = Top + 119.f;
  const float SlotWidth = (PanelWidth - 36.f) / 7.f;
  const int32 ActiveFavorite = Hero->bRightHandSpellReadied
      ? FMath::Clamp(Hero->RightSpell + 1, 1, 6) : 0;
  for (int32 Slot = 0; Slot < 7; ++Slot) {
   const bool bEquipped = ActiveFavorite == Slot;
   const float X = StripX + Slot * SlotWidth;
   DrawRect(bEquipped ? FLinearColor(0.28f, 0.55f, 0.46f, 0.96f)
                     : FLinearColor(0.18f, 0.20f, 0.30f, 0.93f),
       X, StripY, SlotWidth - 3.f, 22.f);
   if (bEquipped) DrawSelectionRail(X,StripY,22.f);
   DrawHoverFrame(X,StripY,SlotWidth-3.f,22.f);
   // A weapon name, not "W": W is reserved exclusively for walking forward.
   const FString Label = Slot == 0 ? TEXT("SWORD")
       : FString::Printf(TEXT("S%d"), Slot);
   DrawText(Label, bEquipped ? FLinearColor::White : FLinearColor(0.77f, 0.83f, 0.98f),
       X + (Slot == 0 ? 3.f : 7.f), StripY + 3.f, nullptr,
       Slot == 0 ? 0.55f : 0.68f);
  }
  const int32 Count = Hero->InventoryItemCodes.Num();
  // Reserve height for a real equipment-stat line and three controls rows.
  const int32 PerPage = Hero->GetInventoryVisibleRows();
  const int32 Selected = FMath::Clamp(Hero->InventorySelection, 0, FMath::Max(0, Count - 1));
  const TArray<int32> DisplayOrder = Hero->GetInventoryDisplayOrder();
  const int32 HoveredIndex = Hero->HitTestInventoryRow();
  // Wheel moves only the visible window; keyboard/controller selection remains independent.
  const int32 PageStart = FMath::Clamp(Hero->InventoryViewStart, 0, FMath::Max(0, Count - PerPage));
  const int32 PageEnd = FMath::Min(Count, PageStart + PerPage);
  DrawText(FString::Printf(TEXT("%d items  |  %d salvage  |  visible %d-%d / %d"),
    Count, Hero->Salvage, Count == 0 ? 0 : PageStart + 1,
    PageEnd, Count), FLinearColor(0.75f,0.72f,0.91f),
    Left + 18.f, Top + 49.f, nullptr, 0.9f);
  if (Count == 0)
   DrawText(TEXT("No equipment yet. Defeat enemies to find gear."), FLinearColor::White,
     Left + 18.f, Top + 100.f, nullptr, 0.92f);
  const float RowSpacing = FMath::Max(21.f, (PanelHeight - 390.f) / PerPage);
  for (int32 DisplayPosition = PageStart; DisplayPosition < PageEnd; ++DisplayPosition) {
   const int32 Index = DisplayOrder[DisplayPosition];
   const int32 Code = Hero->InventoryItemCodes[Index];
   const int32 Affix = Code / 100000;
   const int32 Slot = (Code / 10000) % 10;
   const int32 Rarity = (Code / 1000) % 10;
   const int32 Tier = Code % 1000;
   const TCHAR* Slots[] = {TEXT("Weapon"), TEXT("Armor"), TEXT("Accessory")};
   const TCHAR* Rarities[] = {TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"), TEXT("Epic"), TEXT("Emerald"), TEXT("Legendary")};
   const TCHAR* Traits[] = {TEXT("None"), TEXT("Force"), TEXT("Ward"), TEXT("Focus")};
   const bool bValid = Code > 0 && Slot <= 2 && Affix <= 3 && Rarity <= 5 && Tier >= 1 && Tier <= 25
     && (Rarity != 0 || Affix == 0)
     && Code == Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier;
   const bool bEquipped = bValid && Code == Hero->GetEquipmentSlotCode(Slot);
   const float Y = Top + 145.f + (DisplayPosition - PageStart) * RowSpacing;
   if (Index == Selected) DrawRect(FLinearColor(0.25f,0.18f,0.43f,0.90f),
     Left + 12.f, Y - 3.f, PanelWidth - 24.f, RowSpacing - 3.f);
   else if (Index == HoveredIndex) DrawRect(FLinearColor(0.17f,0.32f,0.43f,0.80f),
     Left + 12.f, Y - 3.f, PanelWidth - 24.f, RowSpacing - 3.f);
   if (Index == Selected)
    DrawSelectionRail(Left+12.f,Y-3.f,FMath::Max(3.f,RowSpacing-3.f));
   DrawHoverFrame(Left+12.f,Y-3.f,PanelWidth-24.f,RowSpacing-3.f);
   const FString Row = bValid ? FString::Printf(TEXT("%02d  %s  T%d  %s  %s%s"),
      DisplayPosition + 1, Slots[Slot], Tier, Rarities[Rarity], Traits[Affix], bEquipped ? TEXT("  [EQUIPPED]") : TEXT(""))
    : FString::Printf(TEXT("%02d  Invalid item"), DisplayPosition + 1);
   // Item names retain a clear text label plus a distinct quality color.
   const FLinearColor RarityColors[] = {FLinearColor(0.90f,0.90f,0.90f),
     FLinearColor(0.25f,0.69f,1.f), FLinearColor(0.96f,0.79f,0.25f),
     FLinearColor(0.72f,0.35f,0.96f), FLinearColor(0.23f,0.84f,0.42f),
     FLinearColor(1.f,0.52f,0.12f)};
   if (bValid && PanelWidth>=440.f && RowSpacing>=23.f)
    DrawGearGlyph(Left+PanelWidth-29.f,Y+FMath::Min(11.f,RowSpacing*0.40f),
      9.f,Slot,RarityColors[Rarity]);
   DrawText(Row, Index == Selected ? FLinearColor::Yellow
     : (bValid ? RarityColors[Rarity] : FLinearColor::White),
     Left + 20.f, Y, nullptr, Canvas->SizeX < 900.f ? 0.75f : 0.9f);
  }
  // Proportional scroll indicator. Clicking its track jumps the visible
  // bag window without selecting, equipping or modifying saved gear.
  if (Count > PerPage) {
   const float TrackY = Top + 145.f;
   const float TrackHeight = RowSpacing * PerPage;
   const float TrackX = Left + PanelWidth - 11.f;
   const float ThumbHeight = FMath::Max(14.f, TrackHeight * PerPage / Count);
   const int32 MaxStart = FMath::Max(1, Count - PerPage);
   const float ThumbY = TrackY + (TrackHeight - ThumbHeight) * PageStart / MaxStart;
   DrawRect(FLinearColor(0.17f,0.16f,0.25f,0.95f), TrackX, TrackY, 4.f, TrackHeight);
   DrawRect(FLinearColor(0.57f,0.81f,0.93f,1.f), TrackX, ThumbY, 6.f, ThumbHeight);
  }
  // Match the combat decoder: a code only contributes while owned, correctly
  // slotted and canonically encoded. HUD previews must never imply phantom stats.
  const auto ReadOwnedGear = [Hero](int32 Code, int32 ExpectedSlot,
                                   int32& Trait, int32& Rarity, int32& Tier) {
   Trait = 0; Rarity = 0; Tier = 0;
   if (Code <= 0 || !Hero->InventoryItemCodes.Contains(Code)) return;
   const int32 EncodedTrait = Code / 100000;
   const int32 Slot = (Code / 10000) % 10;
   const int32 EncodedRarity = (Code / 1000) % 10;
   const int32 EncodedTier = Code % 1000;
   if (EncodedTrait < 0 || EncodedTrait > 3 || Slot != ExpectedSlot
       || EncodedRarity < 0 || EncodedRarity > 5
       || EncodedTier < 1 || EncodedTier > 25
       || (EncodedRarity == 0 && EncodedTrait != 0)
       || Code != EncodedTrait * 100000 + Slot * 10000 + EncodedRarity * 1000 + EncodedTier)
    return;
   Trait = EncodedTrait; Rarity = EncodedRarity; Tier = EncodedTier;
  };
  int32 WeaponTrait = 0, WeaponRarity = 0, WeaponTier = 0;
  int32 ArmorTrait = 0, ArmorRarity = 0, ArmorTier = 0;
  int32 AccessoryTrait = 0, AccessoryRarity = 0, AccessoryTier = 0;
  ReadOwnedGear(Hero->EquippedWeaponCode, 0, WeaponTrait, WeaponRarity, WeaponTier);
  ReadOwnedGear(Hero->EquippedArmorCode, 1, ArmorTrait, ArmorRarity, ArmorTier);
  ReadOwnedGear(Hero->EquippedAccessoryCode, 2, AccessoryTrait, AccessoryRarity, AccessoryTier);
  const float WeaponBonusPct = 100.f * (FMath::Clamp(0.018f * WeaponTier, 0.f, 0.35f)
    + (WeaponTrait == 1 && WeaponRarity > 0 ? 0.06f : 0.f) + WeaponRarity * 0.015f);
  const float ArmorMitigationPct = 100.f * FMath::Min(0.34f, FMath::Min(0.28f, ArmorTier * 0.01f)
    + (ArmorTrait == 2 && ArmorRarity > 0 ? 0.06f : 0.f) + ArmorRarity * 0.005f
    + Hero->GetVersatilityBonus() * 0.5f
    + (Hero->GetSetPieceCount(2) >= 3 ? 0.025f : 0.f)
    + (Hero->HasLegendaryPerk(1) ? 0.015f : 0.f));
  const float ManaRegenBonusPct = 100.f * (FMath::Clamp(AccessoryTier * 0.012f, 0.f, 0.25f)
    + (AccessoryTrait == 3 && AccessoryRarity > 0 ? 0.08f : 0.f) + AccessoryRarity * 0.02f
    + (Hero->GetSetPieceCount(3) >= 3 ? 0.06f : 0.f));
  DrawText(FString::Printf(TEXT("Equipped: +%.1f%% spell damage  |  %.1f%% protection  |  +%.1f%% mana regen"),
    WeaponBonusPct, ArmorMitigationPct, ManaRegenBonusPct),
    FLinearColor(0.76f,0.90f,0.99f), Left + 18.f, Top + 75.f,
    nullptr, Canvas->SizeX < 900.f ? 0.62f : 0.74f);
  // Secondary stats and equipped-set thresholds always reflect active gear.
  DrawText(FString::Printf(TEXT("Crit %.1f%%   Haste %.1f%%   Mastery %.1f%%   Versatility %.1f%%"),
    Hero->GetCriticalChance() * 100.f, Hero->GetHasteBonus() * 100.f,
    Hero->GetMasteryBonus() * 100.f, Hero->GetVersatilityBonus() * 100.f),
    FLinearColor(0.81f,0.89f,1.f), Left + 18.f, Top + 94.f,
    nullptr, Canvas->SizeX < 900.f ? 0.63f : 0.76f);
  const int32 ForcePieces = Hero->GetSetPieceCount(1);
  const int32 WardPieces = Hero->GetSetPieceCount(2);
  const int32 FocusPieces = Hero->GetSetPieceCount(3);
  DrawText(FString::Printf(TEXT("Sets: Force %d/3  Ward %d/3  Focus %d/3    2pc + 3pc unlock bonuses"),
    ForcePieces, WardPieces, FocusPieces), FLinearColor(0.59f,0.95f,0.77f),
    Left + 18.f, Top + 113.f, nullptr, Canvas->SizeX < 900.f ? 0.61f : 0.72f);
  DrawText(TEXT("Mouse wheel: scroll   Left click: select   Right click: equip"),
     FLinearColor::White, Left + 18.f, Top + PanelHeight - 51.f, nullptr, 0.75f);
  DrawText(TEXT("Drag scrollbar: browse   Delete / X: salvage"),
     FLinearColor(0.80f,0.85f,0.97f), Left + 18.f, Top + PanelHeight - 68.f,
     nullptr, PanelWidth < 420.f ? 0.60f : 0.72f);
  // Display cost for the actual selected item, rather than a formula
  // players must calculate while navigating the inventory.
  if (Hero->InventoryItemCodes.IsValidIndex(Selected)) {
   const int32 SelectedCode = Hero->InventoryItemCodes[Selected];
   const int32 SelectedTier = SelectedCode % 1000;
   const int32 SelectedRarity = (SelectedCode / 1000) % 10;
   const int32 SelectedSlot = (SelectedCode / 10000) % 10;
   const int32 SelectedAffix = SelectedCode / 100000;
   const bool bForgeable = SelectedCode > 0 && SelectedTier >= 1 && SelectedTier < 25
     && SelectedSlot >= 0 && SelectedSlot <= 2
     && SelectedRarity >= 0 && SelectedRarity <= 5
     && SelectedAffix >= 0 && SelectedAffix <= 3
     && (SelectedRarity != 0 || SelectedAffix == 0)
     && SelectedCode == SelectedAffix * 100000 + SelectedSlot * 10000
       + SelectedRarity * 1000 + SelectedTier;
   const bool bAscendable = SelectedCode > 0 && SelectedTier >= 1 && SelectedTier <= 25
     && SelectedSlot >= 0 && SelectedSlot <= 2 && SelectedRarity >= 0 && SelectedRarity < 5
     && SelectedAffix >= 0 && SelectedAffix <= 3
     && (SelectedRarity != 0 || SelectedAffix == 0)
     && SelectedCode == SelectedAffix * 100000 + SelectedSlot * 10000
       + SelectedRarity * 1000 + SelectedTier;
   const bool bReforgeable = SelectedCode > 0 && SelectedTier >= 1 && SelectedTier <= 25
     && SelectedSlot >= 0 && SelectedSlot <= 2 && SelectedRarity >= 2 && SelectedRarity <= 5
     && SelectedAffix >= 0 && SelectedAffix <= 3
     && SelectedCode == SelectedAffix * 100000 + SelectedSlot * 10000
       + SelectedRarity * 1000 + SelectedTier;
   // A dedicated, read-only selected-item summary above crafting and comparison.
   // Keep this area hidden on very short viewports to avoid covering bag rows.
   if (PanelHeight >= 500.f) {
    const TCHAR* DetailSlots[] = {TEXT("Weapon"), TEXT("Armor"), TEXT("Accessory")};
    const TCHAR* DetailRarities[] = {TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"),
      TEXT("Epic"), TEXT("Emerald"), TEXT("Legendary")};
    const TCHAR* DetailTraits[] = {TEXT("None"), TEXT("Force"), TEXT("Ward"), TEXT("Focus")};
    const bool bValidDetail = SelectedCode > 0 && SelectedSlot >= 0 && SelectedSlot <= 2
      && SelectedRarity >= 0 && SelectedRarity <= 5
      && SelectedAffix >= 0 && SelectedAffix <= 3 && SelectedTier >= 1 && SelectedTier <= 25
      && (SelectedRarity != 0 || SelectedAffix == 0)
      && SelectedCode == SelectedAffix * 100000 + SelectedSlot * 10000
        + SelectedRarity * 1000 + SelectedTier;
    const FLinearColor DetailColors[] = {FLinearColor(0.90f,0.90f,0.90f),
      FLinearColor(0.25f,0.69f,1.f), FLinearColor(0.96f,0.79f,0.25f),
      FLinearColor(0.72f,0.35f,0.96f), FLinearColor(0.23f,0.84f,0.42f),
      FLinearColor(1.f,0.52f,0.12f)};
    const FString ItemName = bValidDetail
      ? FString::Printf(TEXT("SELECTED  |  %s  |  %s  |  Tier %d  |  %s"),
          DetailSlots[SelectedSlot], DetailRarities[SelectedRarity], SelectedTier,
          DetailTraits[SelectedAffix])
      : TEXT("SELECTED  |  Invalid equipment data");
    DrawText(ItemName,
      bValidDetail ? DetailColors[SelectedRarity] : FLinearColor(1.f,0.48f,0.46f),
      Left + 18.f, Top + PanelHeight - 230.f, nullptr, PanelWidth < 550.f ? 0.61f : 0.78f);
    const int32 SetPieces = bValidDetail && SelectedAffix > 0
      ? Hero->GetSetPieceCount(SelectedAffix) : 0;
    const TCHAR* SetEffects[] = {TEXT("No set trait"), TEXT("2pc: +3% crit  |  3pc: +4% mastery"),
      TEXT("2pc: +3% versatility  |  3pc: +2.5% protection"),
      TEXT("2pc: +4% haste  |  3pc: +6% mana regen")};
    const FString SetLine = bValidDetail && SelectedAffix > 0
      ? FString::Printf(TEXT("SET PREVIEW: %s %d/3 -> %d/3  |  %s"),
          DetailTraits[SelectedAffix], SetPieces,
          Hero->GetProjectedSetPieceCount(SelectedAffix, SelectedCode),
          SetEffects[SelectedAffix])
      : TEXT("EQUIPPED SET: no set bonus from this item");
    DrawText(SetLine, FLinearColor(0.59f,0.95f,0.77f),
      Left + 18.f, Top + PanelHeight - 207.f, nullptr, PanelWidth < 550.f ? 0.56f : 0.68f);
   }
   // Read-only five-tier projection from the same forge cost schedule.
   // Display in the existing tall-panel detail gap; never spend salvage here.
   if (PanelHeight >= 500.f) {
    const MoonfallForgeForecast::FPlan Plan =
      MoonfallForgeForecast::Preview(SelectedCode, Hero->Salvage, 5);
    const FString Forecast = !Plan.Valid ? TEXT("FORGE 5: INVALID GEAR")
      : Plan.NextTierCost == 0 ? TEXT("FORGE 5: MAX TIER")
      : Plan.Upgrades == 0
        ? FString::Printf(TEXT("FORGE 5: NEED %d SALVAGE FOR NEXT TIER"), Plan.NextTierCost)
        : FString::Printf(TEXT("FORGE 5: +%d TIERS TO T%d  |  %d SALVAGE"),
            Plan.Upgrades, Plan.FinalTier, Plan.TotalCost);
    DrawText(Forecast,
      Plan.Valid && Plan.Upgrades > 0
        ? FLinearColor(0.58f,0.91f,0.83f) : FLinearColor(0.93f,0.72f,0.50f),
      Left + 18.f, Top + PanelHeight - 182.f, nullptr,
      PanelWidth < 550.f ? 0.57f : 0.72f);
   }
   const int32 ReforgeCost = 12 + SelectedTier * 2 + SelectedRarity * 8;
   DrawText(bReforgeable
     ? FString::Printf(TEXT("Reforge trait: %d salvage  [%s]"),
         ReforgeCost, Hero->Salvage >= ReforgeCost ? TEXT("READY") : TEXT("NEED MORE"))
     : TEXT("Reforge trait: requires Rare or better"),
     bReforgeable && Hero->Salvage >= ReforgeCost
       ? FLinearColor(0.55f,0.92f,0.75f) : FLinearColor(0.96f,0.66f,0.46f),
     Left + 18.f, Top + PanelHeight - 156.f, nullptr, 0.70f);
   const int32 AscendCost = 20 + SelectedTier * 3 + SelectedRarity * 15;
   DrawText(bAscendable
     ? FString::Printf(TEXT("Ascend rarity: %d salvage  [%s]"),
         AscendCost, Hero->Salvage >= AscendCost ? TEXT("READY") : TEXT("NEED MORE"))
     : TEXT("Ascend rarity: LEGENDARY / invalid"),
     bAscendable && Hero->Salvage >= AscendCost
       ? FLinearColor(0.55f,0.92f,0.75f) : FLinearColor(0.96f,0.66f,0.46f),
     Left + 18.f, Top + PanelHeight - 135.f, nullptr, 0.74f);
   // Compare the highlighted item against the currently equipped item in
   // the same slot. This is informational only and never changes gear.
   const int32 EquippedCode = Hero->GetEquipmentSlotCode(SelectedSlot);
   const int32 EquippedTier = EquippedCode > 0 ? EquippedCode % 1000 : 0;
   const int32 EquippedRarity = EquippedCode > 0 ? (EquippedCode / 1000) % 10 : 0;
   const int32 EquippedAffix = EquippedCode > 0 ? EquippedCode / 100000 : 0;
   const int32 TierDelta = SelectedTier - EquippedTier;
   const int32 RarityDelta = SelectedRarity - EquippedRarity;
   const bool bValidSelection = MoonfallGearAdvisor::Valid(SelectedCode, SelectedSlot);
   const bool bAlreadyEquipped = bValidSelection && SelectedCode == EquippedCode;
   const bool bIsUpgrade = bValidSelection && !bAlreadyEquipped && (EquippedCode <= 0 || TierDelta > 0
     || (TierDelta == 0 && RarityDelta > 0));
   const bool bIsLower = bValidSelection && EquippedCode > 0 && (TierDelta < 0
     || (TierDelta == 0 && RarityDelta < 0));
   const TCHAR* CompareWord = !bValidSelection ? TEXT("INVALID GEAR")
     : bAlreadyEquipped ? TEXT("EQUIPPED") : EquippedCode <= 0 ? TEXT("EMPTY SLOT")
     : bIsUpgrade ? TEXT("UPGRADE") : bIsLower ? TEXT("LOWER") : TEXT("SIDEGRADE");
   const TCHAR* CompareRarities[] = {TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"),
     TEXT("Epic"), TEXT("Emerald"), TEXT("Legendary")};
   const TCHAR* CompareTraits[] = {TEXT("None"), TEXT("Force"), TEXT("Ward"), TEXT("Focus")};
   const FString CompareLine = !bValidSelection ? FString(TEXT("Compare: INVALID GEAR"))
     : EquippedCode <= 0
     ? FString::Printf(TEXT("Compare: %s  |  equipped slot is empty"), CompareWord)
     : FString::Printf(TEXT("Compare: %s  |  T%d / %s / %s  vs  T%d / %s / %s"),
       CompareWord, SelectedTier, CompareRarities[FMath::Clamp(SelectedRarity, 0, 5)],
       CompareTraits[FMath::Clamp(SelectedAffix, 0, 3)],
       EquippedTier, CompareRarities[FMath::Clamp(EquippedRarity, 0, 5)],
       CompareTraits[FMath::Clamp(EquippedAffix, 0, 3)]);
   const FString PowerCompareLine = !bValidSelection ? CompareLine : CompareLine +
     FString::Printf(TEXT("  |  %+d gear power"), Hero->GetItemPowerDelta(SelectedCode));
   DrawText(PowerCompareLine,
     bIsUpgrade ? FLinearColor(0.42f,0.96f,0.62f)
       : bIsLower ? FLinearColor(0.96f,0.62f,0.42f)
       : FLinearColor(0.78f,0.86f,1.f),
     Left + 18.f, Top + PanelHeight - 110.f, nullptr, PanelWidth < 550.f ? 0.57f : 0.68f);
   const int32 NextCost = 4 + SelectedTier * 2 + SelectedRarity * 3;
   const FString ForgeStatus = !bForgeable ? TEXT("Crafting: MAX TIER or invalid")
     : FString::Printf(TEXT("Next tier: %d salvage  [%s]"),
       NextCost, Hero->Salvage >= NextCost ? TEXT("READY") : TEXT("NEED MORE"));
   DrawText(ForgeStatus, bForgeable && Hero->Salvage >= NextCost
     ? FLinearColor(0.50f,0.95f,0.62f) : FLinearColor(0.98f,0.67f,0.45f),
     Left + 18.f, Top + PanelHeight - 89.f, nullptr, 0.82f);
  }
  // Cache the read-only selected-row score. Ownership validation searches the
  // bag, so avoid repeating it at HUD frame rate on 9,999-item inventories.
  // Changing selection, equipped gear, count or selected item invalidates it.
  // Same-count changes in other slots refresh within 0.25 seconds.
  const float AdvisorNow = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
  const int32 AdvisorCode = Count > 0 ? Hero->InventoryItemCodes[Selected] : 0;
  const bool bRefreshAdvisor = !bAdvisorScoreCached || CachedAdvisorHero.Get() != Hero
    || CachedAdvisorCount != Count || CachedAdvisorSelected != Selected
    || CachedAdvisorSelectedCode != AdvisorCode
    || CachedAdvisorWeapon != Hero->EquippedWeaponCode
    || CachedAdvisorArmor != Hero->EquippedArmorCode
    || CachedAdvisorAccessory != Hero->EquippedAccessoryCode
    || AdvisorNow < AdvisorScoreUpdatedAt
    || AdvisorNow - AdvisorScoreUpdatedAt >= 0.25f;
  if (bRefreshAdvisor) {
   CachedAdvisorScore = Count > 0 ? Hero->GetEquipmentUpgradeScore(Selected) : 0;
   CachedAdvisorHero = Hero;
   CachedAdvisorCount = Count;
   CachedAdvisorSelected = Selected;
   CachedAdvisorSelectedCode = AdvisorCode;
   CachedAdvisorWeapon = Hero->EquippedWeaponCode;
   CachedAdvisorArmor = Hero->EquippedArmorCode;
   CachedAdvisorAccessory = Hero->EquippedAccessoryCode;
   AdvisorScoreUpdatedAt = AdvisorNow;
   bAdvisorScoreCached = true;
  }
  const int32 SelectedUpgradeScore = CachedAdvisorScore;
  const bool bCompactAdvisor = PanelWidth < 550.f;
  const FString AdvisorLine = Hero->Health == 0.f
    ? TEXT("DEFEATED: equip gear, close bag to respawn")
    : Hero->IsRecentlyInCombatForAmbientAI()
    ? TEXT("IN COMBAT: weapon swaps only")
    : Count == 0
    ? (bCompactAdvisor ? TEXT("GEAR: EMPTY BAG") : TEXT("GEAR ADVISOR: find equipment to compare"))
    : SelectedUpgradeScore > 0
      ? (bCompactAdvisor ? FString::Printf(TEXT("GEAR: UPGRADE (+%d)"), SelectedUpgradeScore)
         : FString::Printf(TEXT("GEAR ADVISOR: selected gear is an upgrade (+%d)"), SelectedUpgradeScore))
      : (bCompactAdvisor ? TEXT("GEAR: NO UPGRADE") : TEXT("GEAR ADVISOR: selected gear is not an upgrade"));
  DrawText(AdvisorLine,
    SelectedUpgradeScore > 0 ? FLinearColor(0.50f,0.95f,0.62f)
      : FLinearColor(0.74f,0.82f,0.94f),
    Left + 18.f, Top + PanelHeight - 50.f, nullptr,
    PanelWidth < 550.f ? 0.56f : 0.74f);
  if (Hero->Health == 0.f)
   DrawText(TEXT("LT: equip best  |  Crafting and salvage unavailable"), FLinearColor(0.78f,0.90f,0.95f), Left + 18.f, Top + PanelHeight - 70.f, nullptr, 0.74f);
  else DrawText(TEXT("LT: equip best   U / RT: +1 tier   H / RB: forge 5   R / B: ascend   G / LB: reforge"),
     FLinearColor(0.78f,0.90f,0.95f), Left + 18.f, Top + PanelHeight - 70.f, nullptr, 0.74f);
  DrawText(TEXT("C/A: stats   P/Y: spells   J/R3: atlas   B/I/Select/Esc: close"), FLinearColor::White,
     Left + 18.f, Top + PanelHeight - 28.f, nullptr, 0.8f);
  DrawLiveMenuHitCue();
  return;
 }
 // Reuse a single world scan for boss/posture HUD and nearby combat text.
 // This avoids iterating the entire enemy population twice every rendered frame.
 const AMoonfallEnemy* Boss = nullptr;
 const AMoonfallEnemy* PostureTarget = nullptr;
 int32 NearbySwordTell = -1;
 float SwordTellDistanceSq = FMath::Square(650.f);
 float BossDistanceSq = TNumericLimits<float>::Max();
 float PostureDistanceSq = FMath::Square(1200.f);
 TArray<const AMoonfallEnemy*, TInlineAllocator<64>> NearbyDamageEnemies;
 const FVector HeroLocation = Hero->GetActorLocation();
 const bool bHeroLocationValid = !HeroLocation.ContainsNaN();
 const float WorldTime = GetWorld()->GetTimeSeconds();
 for (TActorIterator<AMoonfallEnemy> It(GetWorld()); It; ++It) {
  const AMoonfallEnemy* E = *It;
  if (!IsValid(E) || !bHeroLocationValid) continue;
  const FVector EnemyLocation = E->GetActorLocation();
  if (EnemyLocation.ContainsNaN()) continue;
  const float DistSq = FVector::DistSquared(EnemyLocation, HeroLocation);
  if (!FMath::IsFinite(DistSq)) continue;
  // Keep defeated enemies eligible for final-hit feedback, but only retain
  // actors that have a recent visible damage entry. Most idle world enemies
  // never enter the per-frame damage-label projection pass.
  if (DistSq <= FMath::Square(6000.f)) {
   bool bHasRecentDamage = false;
   for (int32 Slot = 0; Slot < AMoonfallEnemy::DamageHistorySize; ++Slot) {
    const AMoonfallEnemy::FRecentDamageHit& Hit = E->DamageHistory[Slot];
    const float Age = WorldTime - Hit.Time;
    if (Age >= 0.f && Age <= 0.8f && FMath::IsFinite(Hit.Amount) && Hit.Amount > 0.f) {
     bHasRecentDamage = true;
     break;
    }
   }
   if (bHasRecentDamage) NearbyDamageEnemies.Add(E);
  }
  if (E->Health <= 0.f) continue;
  if (DistSq < SwordTellDistanceSq) {
   const int32 Tell = E->GetDirectionalWindup(WorldTime, HeroLocation);
   if (MoonfallOmniattack::ValidAngle(Tell)) {
    NearbySwordTell = Tell;
    SwordTellDistanceSq = DistSq;
   }
  }
  if (E->bBoss) {
   if (DistSq < BossDistanceSq) { Boss = E; BossDistanceSq = DistSq; }
  } else if (DistSq < PostureDistanceSq && (E->Posture > 0.f
      || E->PostureBrokenUntil > WorldTime)) {
   PostureTarget = E;
   PostureDistanceSq = DistSq;
  }
 }
 // Fixed high-contrast text, no flashing or extra world scan. Boss alerts take priority.
 if (MoonfallOmniattack::ValidAngle(NearbySwordTell)
     && !(Boss && BossDistanceSq < FMath::Square(3000.f))) {
  const float TellWidth = FMath::Min(240.f, Canvas->SizeX * 0.5f);
  const float TellX = (Canvas->SizeX - TellWidth) * 0.5f;
  const float TellY = Canvas->SizeY * 0.23f;
  DrawRect(FLinearColor(0.055f,0.035f,0.065f,0.92f), TellX,TellY,TellWidth,40.f);
  DrawText(FString::Printf(TEXT("%s STRIKE | GUARD"), UTF8_TO_TCHAR(MoonfallOmniattack::GuardName(NearbySwordTell))),
     FLinearColor(0.95f,0.86f,0.68f),TellX+9.f,TellY+10.f,nullptr,0.84f);
 }
 if (Boss && BossDistanceSq < FMath::Square(3000.f)) {
  const float Width = FMath::Min(480.f, Canvas->SizeX * 0.68f);
  const float X = (Canvas->SizeX - Width) * 0.5f;
  const float Y = 72.f;
  const float MaxHP = FMath::Max(Boss->Health, Boss->BossMaxHealth);
  const float Percent = FMath::Clamp(Boss->Health / FMath::Max(1.f, MaxHP), 0.f, 1.f);
  const bool bRegionalBoss = Boss->bBoss && Boss->DungeonBossRank >= 1
    && Boss->DungeonBossRank <= 5 && !Boss->DungeonBossFamily.IsNone();
  const FString BossTitle = bRegionalBoss && !Boss->DungeonBossTitle.IsEmpty()
    ? Boss->DungeonBossTitle.ToString() : FString(TEXT("BOSS"));
  const float AlertScale = FMath::Clamp(Hero->BossWarningScale, 1.f, 2.f);
  DrawRect(FLinearColor(0.07f,0.04f,0.10f,0.94f),
    X-6.f,Y-40.f,Width+12.f,120.f);
  DrawRect(FLinearColor(0.19f,0.10f,0.16f),X,Y,Width,16.f);
  DrawRect(Boss->BossPhase >= 3 ? FLinearColor(0.86f,0.18f,0.67f) : FLinearColor(0.84f,0.15f,0.22f),X,Y,Width*Percent,16.f);
  DrawText(FString::Printf(TEXT("%s  |  PHASE %d / 3  |  %.0f / %.0f HP"),
    *BossTitle,Boss->BossPhase,FMath::Max(0.f,Boss->Health),MaxHP),
    FLinearColor::White,X,Y-29.f,nullptr,Canvas->SizeX < 850.f ? 0.73f : 0.95f);
  if (bRegionalBoss) {
   DrawText(FString::Printf(TEXT("REGIONAL BOSS  |  RANK %d / 5"),Boss->DungeonBossRank),
     FLinearColor(0.83f,0.87f,1.f),X,Y-48.f,nullptr,0.76f);
  }
  const bool bBossPostureBroken = Boss->PostureBrokenUntil > GetWorld()->GetTimeSeconds();
  const float BossPostureRatio = FMath::Clamp(Boss->Posture / FMath::Max(1.f, Boss->MaxPosture), 0.f, 1.f);
  DrawRect(FLinearColor(0.15f,0.14f,0.24f),X,Y+24.f,Width,7.f);
  DrawRect(bBossPostureBroken ? FLinearColor(1.f,0.77f,0.30f) : FLinearColor(0.84f,0.59f,1.f),
    X,Y+24.f,Width*(bBossPostureBroken?1.f:BossPostureRatio),7.f);
  if (Width>=320.f) DrawMoonfallSigil(X+Width-13.f,Y+40.f,
    9.f,FLinearColor(0.95f,0.44f,0.59f),0);
  DrawText(bBossPostureBroken ? TEXT("POSTURE SHATTERED") : TEXT("POSTURE"),
    bBossPostureBroken ? FLinearColor(1.f,0.84f,0.48f) : FLinearColor(0.85f,0.79f,1.f),
    X,Y+35.f,nullptr,0.78f);
  const float WarningRemaining = Boss->BossWarningEndTime - GetWorld()->GetTimeSeconds();
  if (FMath::IsFinite(WarningRemaining) && WarningRemaining > 0.f) {
   // Draw a concrete countdown, not just a vague warning. It uses the
   // authoritative attack clock and remains readable in reduced-motion mode.
   const float Seconds = FMath::Clamp(WarningRemaining, 0.f, 10.f);
   const FString MoveName = bRegionalBoss && !Boss->DungeonBossAttackLabel.IsEmpty()
     ? Boss->DungeonBossAttackLabel.ToString() : FString(TEXT("INCOMING ATTACK"));
   // Phantom Blade-inspired readable parry/dodge vocabulary, paired with
   // Wukong-style anticipation. Color is redundant with the explicit text.
   // Red is truly dodge-only in the damage path. Blue stays parryable.
   const bool bRedDodge = bRegionalBoss && Boss->bDungeonBossAttackDodgeOnly;
   const bool bBlueParry = bRegionalBoss && !bRedDodge &&
     (MoveName == TEXT("MOON CLEAVE") || MoveName == TEXT("SPINNING SWEEP"));
   const FLinearColor CueColor = bRedDodge ? FLinearColor(1.f,0.22f,0.26f)
     : bBlueParry ? FLinearColor(0.26f,0.72f,1.f)
     : FLinearColor(1.f,0.81f,0.35f);
   const TCHAR* ActionCue = bRedDodge ? TEXT("RED - DODGE ONLY")
     : bBlueParry ? TEXT("BLUE - PARRY / DODGE")
     : TEXT("GUARD / PARRY / DODGE");
   DrawRect(CueColor,X-6.f,Y+51.f,5.f,22.f);
   DrawText(FString::Printf(TEXT("%s  |  %.1fs  |  %s"),
      *MoveName, Seconds, ActionCue),CueColor,
      X+6.f,Y+55.f,nullptr,FMath::Min(1.35f,0.72f * AlertScale));
   // A fixed, non-flashing impact cue remains readable with Reduce Motion.
   if (Seconds <= 0.3f)
    DrawText(TEXT("IMPACT SOON"),CueColor,X,Y+89.f,nullptr,0.78f);
   if (bRegionalBoss && FMath::IsFinite(Boss->DungeonBossAttackWarningDuration)
       && Boss->DungeonBossAttackWarningDuration > 0.f) {
    const float WarningProgress = FMath::Clamp(
      Seconds / Boss->DungeonBossAttackWarningDuration, 0.f, 1.f);
    DrawRect(FLinearColor(0.25f,0.13f,0.09f),X,Y+73.f,Width,4.f);
    DrawRect(CueColor,
      X,Y+73.f,Width*WarningProgress,4.f);
   }
  }
  if (bRegionalBoss && Canvas->SizeY > 600.f)
   DrawText(FString::Printf(TEXT("DUNGEONS CLEARED  %d / 30  |  ZONES  %d / 6"),
     Hero->GetTotalDefeatedDungeonBosses(),Hero->GetClearedDungeonZoneCount()),
     FLinearColor(0.75f,0.92f,0.83f),X,Y+112.f,nullptr,0.76f);
 }
 // Show only the nearest engaged non-boss posture target to avoid clutter.
 if (PostureTarget && GetOwningPlayerController()) {
  FVector2D Screen;
  if (GetOwningPlayerController()->ProjectWorldLocationToScreen(
      PostureTarget->GetActorLocation() + FVector(0.f,0.f,175.f),Screen)
      && Screen.X >= 0.f && Screen.X + 80.f < Canvas->SizeX
      && Screen.Y >= 0.f && Screen.Y + 18.f < Canvas->SizeY) {
   const bool bShattered = PostureTarget->PostureBrokenUntil > GetWorld()->GetTimeSeconds();
   const float Ratio = FMath::Clamp(PostureTarget->Posture /
       FMath::Max(1.f, PostureTarget->MaxPosture),0.f,1.f);
   DrawRect(FLinearColor(0.12f,0.11f,0.19f),Screen.X,Screen.Y,80.f,5.f);
   DrawRect(bShattered ? FLinearColor(1.f,0.76f,0.24f) : FLinearColor(0.78f,0.47f,0.98f),
     Screen.X,Screen.Y,80.f*(bShattered?1.f:Ratio),5.f);
   if (bShattered) DrawText(TEXT("SHATTERED"),FLinearColor(1.f,0.83f,0.48f),
     Screen.X,Screen.Y+7.f,nullptr,0.75f);
  }
 }
 // World-wide quest tracker, driven only by existing persistent progression.
 // No per-frame actor scans, no new saved fields, and no forced tutorial screens.
 const bool bSanctuaryObjective = Hero->IsNearStartingSanctuary();
 const bool bFirstlightLessons = bSanctuaryObjective && Hero->bStartingSanctuaryPlaced
   && !Hero->FirstlightTutorial.Finished();
 const bool bCanShowQuestTracker = Canvas->SizeX >= 760.f && Canvas->SizeY >= 520.f;
 if (bCanShowQuestTracker) {
  const int32 Discovered = FMath::Clamp(Hero->GetTotalDiscoveredDungeons(), 0, 30);
  const int32 Cleared = FMath::Clamp(Hero->GetTotalDefeatedDungeonBosses(), 0, 30);
  const int32 RegionsCleared = FMath::Clamp(Hero->GetClearedDungeonZoneCount(), 0, 6);
  // Progress already lives in two persistent 30-bit masks. Find one
  // discovered, unbeaten dungeon without searching world actors or paths.
  static const TCHAR* RegionNames[6] = {TEXT("Ashen Wilds"), TEXT("Moonveil Forest"), TEXT("Hollow Depths"), TEXT("Iron Foundry"), TEXT("Frostbound Reach"), TEXT("Eclipse Wastes")};
  int32 NextBossZone = INDEX_NONE;
  int32 NextBossDungeon = INDEX_NONE;
  for (int32 Zone = 0; Zone < 6 && NextBossZone == INDEX_NONE; ++Zone) {
   for (int32 Dungeon = 0; Dungeon < 5; ++Dungeon) {
    if (Hero->HasDiscoveredDungeon(Zone, Dungeon)
        && !Hero->HasDefeatedDungeonBoss(Zone, Dungeon)) {
     NextBossZone = Zone;
     NextBossDungeon = Dungeon;
     break;
    }
   }
  }
  FString Objective;
  FString Context;
  if (bSanctuaryObjective) {
   Objective = TEXT("Find the Firstlight Shrine");
   Context = TEXT("Explore the sanctuary and its northern practice area");
   if (Hero->HasDiscoveredShrine(FName(TEXT("sanctuary_firstlight")))) {
    const int32 GuideQuest = Hero->GetResidentQuestState(FName(TEXT("CrossroadsGuide")));
    if (GuideQuest == 0) {
     Objective = TEXT("Talk to the Wandering Guide (right-click)");
     Context = TEXT("Accept the Guide's salvage contract");
    } else if (GuideQuest == 1) {
     // Practice sentries award XP, not salvage. Obtain world gear and dismantle
     // it in the inventory to complete this existing quest.
     constexpr int32 GuideSalvageCost = 10;
     Objective = Hero->Salvage >= GuideSalvageCost
       ? TEXT("Return to the Guide with 10 salvage (right-click)")
       : FString::Printf(TEXT("Find gear; salvage it in B (%d / 10)"),
           FMath::Clamp(Hero->Salvage, 0, GuideSalvageCost));
     Context = Hero->Salvage >= GuideSalvageCost
       ? TEXT("Quest ready for turn-in")
       : TEXT("Practice gives XP; dungeon equipment can be salvaged");
    } else {
     Objective = TEXT("Practice north, then take the south gate to six regions");
     Context = TEXT("Discover dungeon entrances and defeat their bosses");
    }
   }
  } else if (Cleared >= 30) {
   Objective = TEXT("All 30 dungeon bosses defeated");
   Context = TEXT("Explore remaining secrets and settlements");
  } else if (Discovered < 30) {
   const int32 NextZone = Hero->GetNextUnchartedDungeonZone();
   Objective = NextZone >= 0 && NextZone < 6
     ? FString::Printf(TEXT("Explore %s: discover dungeon entrances"), RegionNames[NextZone])
     : TEXT("Discover more dungeon entrances");
   Context = NextBossZone != INDEX_NONE
     ? FString::Printf(TEXT("Next boss: %s, Dungeon %d  |  J: atlas"),
         RegionNames[NextBossZone], NextBossZone * 5 + NextBossDungeon + 1)
     : TEXT("J: atlas  |  Follow the connected dungeon road");
  } else {
   Objective = NextBossZone != INDEX_NONE
     ? FString::Printf(TEXT("Defeat boss: %s, Dungeon %d"),
         RegionNames[NextBossZone], NextBossZone * 5 + NextBossDungeon + 1)
     : TEXT("Defeat remaining dungeon bosses");
   Context = TEXT("J: atlas  |  Return to discovered, unbeaten dungeons");
  }
  if (!bSanctuaryObjective && GetWorld()) {
   // Show one nearby event in the normal tracker, without replacing the
   // primary quest objective or scanning actors every HUD frame.
   if (const AMoonfallUnrealGameMode* Mode =
       GetWorld()->GetAuthGameMode<AMoonfallUnrealGameMode>()) {
    const FString EventHint=Mode->GetNearbyAmbientEventHint(Hero->GetActorLocation());
    if (!EventHint.IsEmpty()) Context=EventHint;
   }
  }
  if (bFirstlightLessons) {
   // Optional five-step learning, earned by real player actions in any order.
   using EAction = MoonfallFirstlightTutorial::EAction;
   switch (Hero->FirstlightTutorial.Next()) {
    case EAction::DiscoverShrine:
     Objective = TEXT("Approach the Firstlight Shrine"); break;
    case EAction::SpeakToGuide:
     Objective = TEXT("Speak to the Guide (right-click / D-pad Up, then Talk)"); break;
    case EAction::SwingSword:
     Objective = TEXT("Try your sword (left-click / RT)"); break;
    case EAction::PerformDodge:
     Objective = TEXT("Dodge near the northern practice sentries"); break;
    case EAction::OpenSpellbook:
     Objective = TEXT("Open your Spell Book (P / inventory Book tab)"); break;
    default: break;
   }
   Context = TEXT("OPTIONAL: Exit via the south gate whenever ready");
  }
  const float Width = FMath::Min(560.f, Canvas->SizeX - 40.f);
  const float Left = 20.f;
  // The quest tracker must not cover the centered boss health/attack HUD.
  // Move it down only when the two actual painted bounds intersect.
  const float BossHudWidth = FMath::Min(480.f,Canvas->SizeX*0.68f);
  const float BossHudLeft = (Canvas->SizeX-BossHudWidth)*0.5f;
  const bool bBossHudVisible = Boss && BossDistanceSq < FMath::Square(3000.f);
  const bool bQuestOverlapsBoss = bBossHudVisible
     && BossHudLeft < Left+Width && BossHudLeft+BossHudWidth > Left;
  const float Top = bQuestOverlapsBoss ? 232.f : 78.f;
  DrawRect(FLinearColor(0.035f, 0.055f, 0.11f, 0.88f),
    Left, Top, Width, 124.f);
  DrawRect(FLinearColor(0.30f, 0.72f, 0.92f, 0.95f),
    Left, Top, 4.f, 124.f);
  // Static border aligns the in-world objective with the native menu frame.
  DrawRect(FLinearColor(0.48f,0.65f,0.76f,0.80f),Left,Top,Width,2.f);
  DrawRect(FLinearColor(0.48f,0.65f,0.76f,0.80f),Left,Top+122.f,Width,2.f);
  // The tracker uses the real current-region seal below, not a generic icon.
  DrawText(bSanctuaryObjective
      ? TEXT("SHATTERED SANCTUARY  |  FIRST STEPS")
      : TEXT("WORLD QUEST TRACKER  |  EXPLORATION"),
    FLinearColor(0.72f, 0.92f, 1.f), Left + 14.f, Top + 7.f, nullptr, 0.86f);
  // Region climate is a pure policy sample. Do not scan weather actors
  // or nearby residents from the frequently drawn HUD.
  const int ClimateRegion=MoonfallLivingSimulation::RegionFromWorldX(Hero->GetActorLocation().X);
  if (!bSanctuaryObjective)
   DrawRegionEmblem(Left+Width-16.f,Top+109.f,8.f,ClimateRegion,
     FLinearColor(0.53f,0.77f,0.92f));
  const auto Climate=MoonfallLivingSimulation::WeatherFor(ClimateRegion,
      MoonfallLivingSimulation::Phase(GetWorld()->GetTimeSeconds(),360.f));
  DrawText(FString::Printf(TEXT("%s  |  %s"),
      bSanctuaryObjective?TEXT("FIRSTLIGHT"):TEXT("WEATHER"),
      UTF8_TO_TCHAR(MoonfallLivingSimulation::WeatherName(Climate))),
    FLinearColor(0.7f,0.85f,0.96f),Left+Width-160.f,Top+7.f,nullptr,0.64f);
  DrawText(Objective, FLinearColor::White,
    Left + 14.f, Top + 33.f, nullptr, Canvas->SizeX < 1050.f ? 0.68f : 0.82f);
  DrawText(Context, FLinearColor(0.76f, 0.84f, 0.92f),
    Left + 14.f, Top + 57.f, nullptr, Canvas->SizeX < 1050.f ? 0.65f : 0.75f);
  // Two stable progress bars: discovering all 30 entrances, then clearing
  // all 30 bosses. Never conflate visited regions with defeated bosses.
  const float TrackWidth = FMath::Max(20.f, Width - 28.f);
  DrawRect(FLinearColor(0.12f, 0.15f, 0.23f),
    Left + 14.f, Top + 81.f, TrackWidth, 5.f);
  DrawRect(FLinearColor(0.27f, 0.74f, 0.90f),
    Left + 14.f, Top + 81.f, TrackWidth * (static_cast<float>(Discovered) / 30.f), 5.f);
  DrawRect(FLinearColor(0.12f, 0.15f, 0.23f),
    Left + 14.f, Top + 92.f, TrackWidth, 5.f);
  DrawRect(FLinearColor(0.84f, 0.56f, 0.98f),
    Left + 14.f, Top + 92.f, TrackWidth * (static_cast<float>(Cleared) / 30.f), 5.f);
  if (bFirstlightLessons)
   DrawText(FString::Printf(TEXT("FIRST STEPS %u/5  |  OPTIONAL"), Hero->FirstlightTutorial.Count()),
     FLinearColor(0.82f, 0.91f, 0.96f), Left + 14.f, Top + 102.f, nullptr,
     Canvas->SizeX < 1050.f ? 0.65f : 0.74f);
  else
   DrawText(FString::Printf(TEXT("FOUND %d/30  |  BOSSES %d/30  |  REGIONS %d/6"),
       Discovered, Cleared, RegionsCleared),
     FLinearColor(0.82f, 0.91f, 0.96f), Left + 14.f, Top + 102.f, nullptr,
     Canvas->SizeX < 1050.f ? 0.65f : 0.74f);
  // Preserve the sanctuary tutorial and primary objective. Additional regional
  // quests are read-only, derived from the same persistent dungeon ledger.
  if (!bSanctuaryObjective) {
   int Seen[MoonfallWorldQuest::ZoneCount]{}, Defeated[MoonfallWorldQuest::ZoneCount]{};
   for (int Zone=0; Zone<MoonfallWorldQuest::ZoneCount; ++Zone)
    for (int Dungeon=0; Dungeon<5; ++Dungeon) {
     Seen[Zone] += Hero->HasDiscoveredDungeon(Zone,Dungeon) ? 1 : 0;
     Defeated[Zone] += Hero->HasDefeatedDungeonBoss(Zone,Dungeon) ? 1 : 0;
    }
   const auto Goals=MoonfallWorldQuest::Select(Seen,Defeated,MoonfallWorldQuest::MaxVisible);
   const float ExtraTop=Top+132.f;
   const float OrbRadius=FMath::Clamp(FMath::Min(Canvas->SizeX,Canvas->SizeY)*0.092f,50.f,80.f);
   const float Available=Canvas->SizeY-2.f*OrbRadius-33.f-ExtraTop;
   const int Rows=MoonfallWorldQuest::VisibleRows(Available,Goals.Count);
   if (Goals.Valid && Rows>0) {
    const float Height=MoonfallWorldQuest::HeaderHeight+Rows*MoonfallWorldQuest::RowHeight;
    DrawRect(FLinearColor(0.035f,0.055f,0.11f,0.88f),Left,ExtraTop,Width,Height);
    DrawText(FString::Printf(TEXT("REGIONAL QUESTS  |  %d/%d SHOWN"),Rows,Goals.Count),
      FLinearColor(0.72f,0.92f,1.f),Left+14.f,ExtraTop+4.f,nullptr,0.76f);
    for (int Row=0; Row<Rows; ++Row) {
     const int Goal=Goals.Indices[Row];
     const int Progress=MoonfallWorldQuest::Progress(Goal,Seen,Defeated);
     const float Y=ExtraTop+MoonfallWorldQuest::HeaderHeight+Row*MoonfallWorldQuest::RowHeight;
     DrawText(FString::Printf(TEXT("%s  |  %d/5 %s"),UTF8_TO_TCHAR(MoonfallWorldQuest::GoalTitles[Goal]),
        Progress,UTF8_TO_TCHAR(MoonfallWorldQuest::Type(Goal))),
        FLinearColor(0.85f,0.92f,0.98f),Left+14.f,Y+2.f,nullptr,0.70f);
     DrawRect(FLinearColor(0.13f,0.18f,0.26f),Left+14.f,Y+20.f,TrackWidth,3.f);
     DrawRect(FLinearColor(0.32f,0.74f,0.91f),Left+14.f,Y+20.f,TrackWidth*(Progress/5.f),3.f);
    }
   }
  }
 }
 // Compact circular resource HUD at bottom left, with actual gameplay values.
 const float Radius = FMath::Clamp(FMath::Min(Canvas->SizeX, Canvas->SizeY) * 0.092f, 50.f, 80.f);
 const float CX = Radius + 25.f;
 const float CY = Canvas->SizeY - Radius - 25.f;
 const float LabelScale = FMath::Clamp(Radius / 65.f, 0.78f, 1.16f);
 const bool bCompactScreen = Canvas->SizeX < 760.f || Canvas->SizeY < 480.f;
 const float ActualHealth = FMath::Clamp(Hero->Health / FMath::Max(1.f, Hero->MaxHealth), 0.f, 1.f);
 const float ActualMana = FMath::Clamp(Hero->Mana / FMath::Max(1.f, Hero->MaxMana), 0.f, 1.f);
 const float ActualStamina = FMath::Clamp(Hero->Stamina / FMath::Max(1.f, Hero->MaxStamina), 0.f, 1.f);
 const float ActualXP = FMath::Clamp(static_cast<float>(Hero->Experience) / FMath::Max(1, Hero->ExperienceToNextLevel), 0.f, 1.f);
 const float Delta = GetWorld() ? FMath::Clamp(GetWorld()->GetDeltaSeconds(), 0.f, 0.1f) : 0.f;
 if (DisplayHealth < 0.f) { DisplayHealth = ActualHealth; DisplayMana = ActualMana; DisplayXP = ActualXP; DisplayStamina = ActualStamina; }
 if (LastShownLevel < 0) LastShownLevel = Hero->Level;
 if (Hero->Level != LastShownLevel) {
  LastShownLevel = Hero->Level;
  DisplayXP = 0.f;
  LevelGlowUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + 1.25f : 0.f;
 }
 // Reduced Motion also disables orbital resource tweening and low-HP pulsing.
 // The numeric gameplay values do not change, only their drawn animation.
 DisplayHealth = Hero->bReduceMotion ? ActualHealth : FMath::FInterpTo(DisplayHealth, ActualHealth, Delta, 9.f);
 DisplayMana = Hero->bReduceMotion ? ActualMana : FMath::FInterpTo(DisplayMana, ActualMana, Delta, 9.f);
 DisplayStamina = Hero->bReduceMotion ? ActualStamina : FMath::FInterpTo(DisplayStamina, ActualStamina, Delta, 11.f);
 DisplayXP = Hero->bReduceMotion ? ActualXP : FMath::FInterpTo(DisplayXP, ActualXP, Delta, 7.f);
 const float HealthFraction = DisplayHealth;
 const float ManaFraction = DisplayMana;
 const float XpFraction = DisplayXP;
 // Keep critical health unmistakable without flashing the entire screen.
 const bool bCriticalHealth = Hero->Health > 0.f && ActualHealth <= 0.25f;
 const float LowHealthPulse = bCriticalHealth && !Hero->bReduceMotion && GetWorld()
     ? 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 4.f) : 0.f;
 // Readable static medallion mark sits beyond the original three-resource orb.
 DrawMoonfallSigil(CX,CY-Radius*1.28f,7.f,
   FLinearColor(0.88f,0.70f,0.44f),0);
 const float InnerRadius = Radius * 0.69f;
 // Fill by horizontal rows to give the orb a true circular silhouette, not a rectangle.
 for (int32 I = -static_cast<int32>(InnerRadius); I <= static_cast<int32>(InnerRadius); I += 2) {
  const float HalfWidth = FMath::Sqrt(FMath::Max(0.f, InnerRadius * InnerRadius - I * I));
  const bool bFilled = static_cast<float>(I) >= InnerRadius * (1.f - 2.f * HealthFraction);
  DrawRect(bFilled ? FLinearColor(0.91f, 0.09f, 0.17f, 0.96f) : FLinearColor(0.16f, 0.04f, 0.09f, 0.88f),
           CX - HalfWidth, CY + I, HalfWidth * 2.f, 2.5f);
 }
 auto DrawArc = [this, CX, CY](float ArcRadius, float Fraction, const FLinearColor& Track, const FLinearColor& Fill, float Thickness) {
  const int32 Segments = FMath::Clamp(FMath::RoundToInt(ArcRadius * 0.9f), 48, 96);
  const float Progress = FMath::Clamp(Fraction, 0.f, 1.f);
  for (int32 I = 0; I < Segments; ++I) {
   const float StartFraction = static_cast<float>(I) / Segments;
   const float EndFraction = static_cast<float>(I + 1) / Segments;
   const float A0 = -PI * 0.5f + 2.f * PI * StartFraction;
   const float A1 = -PI * 0.5f + 2.f * PI * EndFraction;
   const FVector2D Start(CX + FMath::Cos(A0) * ArcRadius, CY + FMath::Sin(A0) * ArcRadius);
   const FVector2D End(CX + FMath::Cos(A1) * ArcRadius, CY + FMath::Sin(A1) * ArcRadius);
   Canvas->K2_DrawLine(Start, End, Thickness, Track);
   if (Progress <= StartFraction) continue;
   const float FilledEnd = FMath::Min(Progress, EndFraction);
   const float AFilled = -PI * 0.5f + 2.f * PI * FilledEnd;
   Canvas->K2_DrawLine(Start,
       FVector2D(CX + FMath::Cos(AFilled) * ArcRadius, CY + FMath::Sin(AFilled) * ArcRadius),
       Thickness, Fill);
  }
 };
 DrawArc(Radius * 0.78f, 1.f, FLinearColor(0.42f, 0.27f, 0.32f), FLinearColor(0.42f, 0.27f, 0.32f), 2.5f);
 if (bCriticalHealth) {
  DrawArc(Radius * 1.055f, 1.f, FLinearColor(0.4f, 0.05f, 0.09f, 0.35f),
          FLinearColor(1.f, 0.10f, 0.16f, 0.35f + LowHealthPulse * 0.45f), 2.5f);
 }
 DrawArc(Radius * 0.85f, ManaFraction, FLinearColor(0.06f, 0.11f, 0.23f), FLinearColor(0.19f, 0.59f, 1.f), 5.f);
 DrawArc(Radius * 0.97f, XpFraction, FLinearColor(0.19f, 0.10f, 0.28f), FLinearColor(0.73f, 0.32f, 1.f), 4.f);
 // Thin green stamina arc stays outside the original red HP / blue mana / purple XP orb.
 const bool bStaminaLow = Hero->Stamina < Hero->DodgeStaminaCost;
 const bool bSprintRecovery = Hero->Stamina < FMath::Clamp(Hero->SprintRestartStamina, 0.f, Hero->MaxStamina);
 DrawArc(Radius * 1.13f, DisplayStamina, FLinearColor(0.06f, 0.17f, 0.10f),
   bStaminaLow ? FLinearColor(1.f, 0.68f, 0.22f) : FLinearColor(0.22f, 0.86f, 0.43f), 3.f);
 if ((bStaminaLow || bSprintRecovery) && !bCompactScreen) {
  DrawText(bStaminaLow ? TEXT("LOW STAMINA") : TEXT("SPRINT RECOVERING"),
    FLinearColor(1.f,0.80f,0.40f),
    CX + Radius + 14.f, CY + 15.f, nullptr, 0.82f * LabelScale);
 }
 // Subtle, nonflashing right-hand readout. No extra HUD panels or asset changes.
 if (!bCompactScreen) {
  // Show the actual readied spell, not an ambiguous generic "SPELL".
  // The fourth native spell restores health; no new invented spell schools.
  const FString RightMode = Hero->bRightHandSpellReadied
      ? (Hero->RightSpell == 3 ? FString(TEXT("SPELL 4: HEAL"))
          : FString::Printf(TEXT("SPELL %d"), FMath::Clamp(Hero->RightSpell + 1, 1, 6)))
      : (Hero->GetValidatedEquippedTier(0) > 0 ? TEXT("SWORD") : TEXT("UNARMED"));
  DrawText(FString::Printf(TEXT("RIGHT HAND: %s  |  B: choose"), *RightMode),
    FLinearColor(0.79f, 0.88f, 0.98f),
    CX + Radius + 14.f, CY + 42.f, nullptr, 0.73f * LabelScale);
  if (!Hero->bRightHandSpellReadied && Hero->CurrentSwordAngle >= 0
      && Hero->CurrentSwordAngle < 4 && Hero->SwordFlowStep >= 1 && Hero->SwordFlowStep <= 4) {
   const auto& Swing=MoonfallOmniattack::Profile(Hero->CurrentSwordAngle);
   DrawText(FString::Printf(TEXT("%s | FLOW %d/4"),UTF8_TO_TCHAR(Swing.Name),Hero->SwordFlowStep),
      FLinearColor(0.91f,0.78f,0.49f),CX+Radius+14.f,CY+60.f,nullptr,0.70f*LabelScale);
  }
 }
 const float GlowLeft = GetWorld() ? FMath::Max(0.f, LevelGlowUntil - GetWorld()->GetTimeSeconds()) : 0.f;
 if (GlowLeft > 0.f) {
  const float Intensity = FMath::Clamp(GlowLeft / 1.25f, 0.f, 1.f);
  DrawArc(Radius * 1.055f, 1.f, FLinearColor(0.f,0.f,0.f,0.f),
          FLinearColor(0.85f, 0.48f, 1.f, Intensity), 2.f);
 }
 DrawText(FString::Printf(TEXT("%.0f / %.0f HP"), FMath::Max(0.f, Hero->Health), Hero->MaxHealth),
          bCriticalHealth ? FLinearColor(1.f, 0.95f, 0.95f) : FLinearColor::White,
          CX - Radius * 0.39f, CY - Radius * 0.19f, nullptr, 0.88f * LabelScale);
 DrawText(FString::Printf(TEXT("LV %d"), Hero->Level), FLinearColor(0.91f,0.80f,1.f),
          CX - Radius * 0.26f, CY + Radius * 0.28f, nullptr, 0.73f * LabelScale);
 if (GlowLeft > 0.f) {
  const float Alpha = FMath::Clamp(GlowLeft / 1.25f, 0.f, 1.f);
  DrawText(TEXT("LEVEL UP"), FLinearColor(0.91f, 0.73f, 1.f, Alpha),
           CX + Radius + 14.f, CY - 13.f, nullptr, 0.96f * LabelScale);
 }
 if (!bCompactScreen) {
  const TCHAR* RarityNames[] = { TEXT("COMMON"), TEXT("UNCOMMON"), TEXT("RARE"),
    TEXT("EPIC"), TEXT("EMERALD"), TEXT("LEGENDARY") };
  const int32 Rarity = FMath::Clamp(Hero->LastLootRarity, 0, 5);
  DrawText(FString::Printf(TEXT("Gear W:%d A:%d C:%d | Salvage %d | Bag %d"),
   Hero->GetValidatedEquippedTier(0), Hero->GetValidatedEquippedTier(1),
    Hero->GetValidatedEquippedTier(2), Hero->Salvage, Hero->InventoryItemCodes.Num()),
   FLinearColor(0.85f,0.84f,0.95f), 24.f, Canvas->SizeY - 75.f, nullptr, 0.8f);
  if (Hero->InventoryItemCodes.Num() > 0) {
   DrawText(FString::Printf(TEXT("Last drop: %s Tier %d (%+d vs equipped)"),
    RarityNames[Rarity], Hero->LastLootTier, Hero->LastLootUpgrade),
    FLinearColor(0.95f,0.84f,0.47f), 24.f, Canvas->SizeY - 55.f, nullptr, 0.8f);
  }
 }
 // One cached party lookup, no actor search or extra world-space labels.
 // Keep the companion readout away from the player's resource orb.
 if (!bCompactScreen) {
  const auto* CompanionMode=GetWorld()->GetAuthGameMode<AMoonfallUnrealGameMode>();
  const auto* Scout=CompanionMode?CompanionMode->GetLocalCompanionForHero(Hero):nullptr;
  if (IsValid(Scout) && !Scout->IsActorBeingDestroyed()
      && Scout->ResidentId==Hero->GetCompanionParty().ResidentId
      && FMath::IsFinite(Scout->CompanionHealth)) {
   const float HP=FMath::Clamp(Scout->CompanionHealth,0.f,100.f);
   const bool bDown=Scout->bCompanionDowned||HP<=0.f;
   const float W=240.f,X=Canvas->SizeX-W-18.f,Y=Canvas->SizeY-117.f;
   const TCHAR* Stance=bDown?TEXT("DOWNED"):
     CompanionMode->LocalCompanionStance==EMoonfallCompanionStance::Follow?TEXT("FOLLOW"):
     CompanionMode->LocalCompanionStance==EMoonfallCompanionStance::Hold?TEXT("HOLD"):TEXT("PASSIVE");
   DrawRect(FLinearColor(0.035f,0.045f,0.09f,0.92f),X,Y,W,65.f);
   DrawText(Scout->DisplayName.ToString().Left(24),FLinearColor::White,X+10.f,Y+5.f,nullptr,0.82f);
   DrawRect(FLinearColor(0.20f,0.12f,0.16f),X+10.f,Y+27.f,W-20.f,7.f);
   const FLinearColor HPColor=HP<=25.f?FLinearColor(1.f,0.28f,0.25f):FLinearColor(0.32f,0.82f,0.53f);
   DrawRect(HPColor,X+10.f,Y+27.f,(W-20.f)*HP/100.f,7.f);
   DrawText(FString::Printf(TEXT("%s | %.0f / 100 HP"),Stance,HP),
      bDown?FLinearColor(1.f,0.44f,0.39f):FLinearColor(0.82f,0.89f,0.98f),
      X+10.f,Y+39.f,nullptr,0.72f);
  }
 }
 if (!bCompactScreen) DrawText(TEXT("ESC  OPTIONS"), FLinearColor(0.73f, 0.70f, 0.83f, 0.85f),
          FMath::Max(16.f, Canvas->SizeX - 142.f), Canvas->SizeY - 37.f, nullptr, 0.82f);
 // Reuse one controller lookup for every projected damage label this frame.
 const APlayerController* DamageProjectionPC = GetOwningPlayerController();
 int32 VisibleDamageNumbers = 0;
 for (const AMoonfallEnemy* Enemy : NearbyDamageEnemies) {
  if (VisibleDamageNumbers >= 32) break;
  if (!IsValid(Enemy)) continue;
  // Capture position once per target; a rapid combo can draw several labels.
  const FVector EnemyDamageLocation = Enemy->GetActorLocation();
  for (int32 Slot = 0; Slot < AMoonfallEnemy::DamageHistorySize && VisibleDamageNumbers < 32; ++Slot) {
   const AMoonfallEnemy::FRecentDamageHit& Hit = Enemy->DamageHistory[Slot];
   const float Age = WorldTime - Hit.Time;
   if (Age < 0.f || Age > 0.8f || !FMath::IsFinite(Hit.Amount) || Hit.Amount <= 0.f) continue;
   FVector2D Screen;
   // Offset entries so consecutive attacks on the same target remain readable.
   const float HorizontalOffset = static_cast<float>(Slot - 1) * 24.f;
   const FVector WorldTextPosition = EnemyDamageLocation + FVector(0.f, 0.f, 175.f + Age * 65.f);
   if (DamageProjectionPC && DamageProjectionPC->ProjectWorldLocationToScreen(WorldTextPosition, Screen)
       && Screen.X + HorizontalOffset >= 0.f && Screen.X + HorizontalOffset < Canvas->SizeX
       && Screen.Y >= 0.f && Screen.Y < Canvas->SizeY) {
    ++VisibleDamageNumbers;
    const bool bShatter = Hit.bShatter;
    // Lethal hits retain ordinary damage numbers. No KO text overlays the
    // finisher choreography; the combo is communicated through animation.
    const FLinearColor DamageColor = bShatter
      ? FLinearColor(1.f, 0.56f, 0.22f, 1.f - Age / 0.8f)
      : FLinearColor(1.f, 0.84f, 0.2f, 1.f - Age / 0.8f);
    const FString DamageLabel = bShatter
      ? FString::Printf(TEXT("SHATTER -%.0f"), Hit.Amount)
      : FString::Printf(TEXT("-%.0f"), Hit.Amount);
    DrawText(DamageLabel, DamageColor, Screen.X + HorizontalOffset, Screen.Y, nullptr,
      (bShatter ? 1.55f : 1.3f) * Hero->CombatTextScale);
   }
  }
 }
}
