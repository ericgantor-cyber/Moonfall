#include "MoonfallMasteryOrb.h"
#include "MoonfallHeroPawn.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallMasteryOrb::AMoonfallMasteryOrb(){
 PrimaryActorTick.bCanEverTick=false;
 PickupRange=CreateDefaultSubobject<USphereComponent>(TEXT("PickupRange"));
 SetRootComponent(PickupRange);
 PickupRange->InitSphereRadius(85.f);
 PickupRange->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 PickupRange->SetCollisionResponseToAllChannels(ECR_Ignore);
 PickupRange->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
 PickupRange->SetGenerateOverlapEvents(false);
 PickupRange->SetCanEverAffectNavigation(false);
 OrbMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
 OrbMesh->SetupAttachment(PickupRange);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 if(Sphere.Succeeded())OrbMesh->SetStaticMesh(Sphere.Object);
 OrbMesh->SetRelativeScale3D(FVector(0.42f));
 OrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 OrbMesh->SetGenerateOverlapEvents(false);
 OrbMesh->SetCanEverAffectNavigation(false);
 OrbMesh->SetCastShadow(false);
 OrbLabel=CreateDefaultSubobject<UTextRenderComponent>(TEXT("OrbLabel"));
 OrbLabel->SetupAttachment(PickupRange);
 OrbLabel->SetRelativeLocation(FVector(0.f,0.f,45.f));
 OrbLabel->SetWorldSize(22.f);
 OrbLabel->SetHorizontalAlignment(EHTA_Center);
 OrbLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 OrbLabel->SetGenerateOverlapEvents(false);
 OrbLabel->SetCanEverAffectNavigation(false);
}
bool AMoonfallMasteryOrb::InitializeRouteIdentity(int32 InRegion,int32 InIndex,bool bInHidden){
 if(HasActorBegunPlay()||IsActorBeingDestroyed()||Region!=INDEX_NONE||Index!=INDEX_NONE||
    !MoonfallSkillMastery::ValidOrbID(InRegion,InIndex))return false;
 Region=InRegion;Index=InIndex;bHiddenOrb=bInHidden;return true;
}
void AMoonfallMasteryOrb::BeginPlay(){
 Super::BeginPlay();
#if WITH_EDITOR
 if(!FParse::Param(FCommandLine::Get(),TEXT("MoonfallMasteryPreview"))||
    !MoonfallSkillMastery::ValidOrbID(Region,Index)){
  Destroy();return;
 }
 OrbLabel->SetText(FText::FromString(bHiddenOrb?TEXT("HIDDEN ORB"):TEXT("AGILITY ORB")));
 OrbLabel->SetTextRenderColor(bHiddenOrb?FColor(190,120,255):FColor(90,255,150));
 PickupRange->OnComponentBeginOverlap.AddDynamic(this,&AMoonfallMasteryOrb::OnPickup);
 PickupRange->SetGenerateOverlapEvents(true);
 PickupRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 if(!IsActorBeingDestroyed())
  CheckHeroAtSpawn(Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this,0)));
#else
 Destroy();
#endif
}
void AMoonfallMasteryOrb::OnPickup(UPrimitiveComponent* Component,AActor* Other,UPrimitiveComponent* OtherComponent,int32 BodyIndex,bool bSweep,const FHitResult& Hit){
 if(Component!=PickupRange)return;
 auto* Hero=Cast<AMoonfallHeroPawn>(Other);
 if(!IsValid(Hero)||OtherComponent!=Hero->GetCapsuleComponent())return;
 CheckHeroAtSpawn(Hero);
}
void AMoonfallMasteryOrb::CheckHeroAtSpawn(AMoonfallHeroPawn* Hero){
 if(bClaimed||IsActorBeingDestroyed()||!GetWorld()||!IsValid(Hero)||
    Hero->IsActorBeingDestroyed()||Hero->GetWorld()!=GetWorld()||
    !Hero->IsSkillMasteryPreviewEnabled()||!Hero->IsPlayerControlled()||
    !Hero->bWorldProgressInitialized||Hero->bWorldProgressLoadInProgress||
    Hero->bPersistentSaveWritesBlocked||!FMath::IsFinite(Hero->Health)||Hero->Health<=0.f)return;
 // Retire saved or duplicate collectibles even when the hero is far away.
 // New rewards still require current capsule overlap and a validated identity.
 const auto* Ledger=Hero->GetSessionMastery();
 if(!Ledger||!MoonfallSkillMastery::ValidOrbID(Region,Index))return;
 if(!MoonfallSkillMastery::IsCollected(*Ledger,Region,Index)){
  if(!PickupRange->IsOverlappingComponent(Hero->GetCapsuleComponent()))return;
  const auto Kind=bHiddenOrb?MoonfallSkillMastery::Orb::Hidden:MoonfallSkillMastery::Orb::Agility;
  if(!Hero->TryCollectMasteryOrb(Region,Index,Kind))return;
 }
 bClaimed=true;
 PickupRange->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Destroy();
}
