#include "MoonfallQuestGuide.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

AMoonfallQuestGuide::AMoonfallQuestGuide()
{
 // Half-second scheduler; no Behavior Tree, controller or expensive path search.
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickInterval = 0.5f;
 Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuideBody"));
 SetRootComponent(Body);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cylinder.Succeeded()) Body->SetStaticMesh(Cylinder.Object);
 Body->SetRelativeScale3D(FVector(0.45f, 0.45f, 1.55f));
 // NPCs do not block players, but navigation sweeps must stop at world walls.
 Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 Body->SetCollisionResponseToAllChannels(ECR_Ignore);
 Body->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

 Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuideHead"));
 Head->SetupAttachment(Body);
 if (Sphere.Succeeded()) Head->SetStaticMesh(Sphere.Object);
 // Root scales affect attached pieces, so offsets are intentionally local.
 Head->SetRelativeLocation(FVector(0.f, 0.f, 57.f));
 Head->SetRelativeScale3D(FVector(1.3f, 1.3f, 0.42f));
 Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);

 LeftArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuideLeftArm"));
 LeftArm->SetupAttachment(Body);
 if (Cube.Succeeded()) LeftArm->SetStaticMesh(Cube.Object);
 LeftArm->SetRelativeLocation(FVector(0.f, -88.f, 7.f));
 LeftArm->SetRelativeScale3D(FVector(0.40f, 0.32f, 0.64f));
 LeftArm->SetRelativeRotation(FRotator(-15.f, 0.f, -14.f));
 LeftArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);

 RightArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuideRightArm"));
 RightArm->SetupAttachment(Body);
 if (Cube.Succeeded()) RightArm->SetStaticMesh(Cube.Object);
 RightArm->SetRelativeLocation(FVector(0.f, 88.f, 7.f));
 RightArm->SetRelativeScale3D(FVector(0.40f, 0.32f, 0.64f));
 RightArm->SetRelativeRotation(FRotator(-15.f, 0.f, 14.f));
 RightArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FText AMoonfallQuestGuide::GetGuideTitle() const
{
 static const TCHAR* Names[6][3] = {
  {TEXT("Ash Warden"), TEXT("Cinder Smith"), TEXT("Ember Watcher")},
  {TEXT("Mire Keeper"), TEXT("Bog Herbalist"), TEXT("Fen Scout")},
  {TEXT("Frost Herald"), TEXT("Rime Tailor"), TEXT("Snow Pathfinder")},
  {TEXT("Storm Oracle"), TEXT("Thunder Artisan"), TEXT("Cloud Ranger")},
  {TEXT("Foundry Chronicler"), TEXT("Forge Mechanic"), TEXT("Iron Sentinel")},
  {TEXT("Eclipse Seer"), TEXT("Moon Weaver"), TEXT("Shadow Lookout")}
 };
 if (PopulationNpcId >= 18 && PopulationNpcId < 1111) {
  static const TCHAR* Regions[] = {TEXT("Ash"), TEXT("Mire"), TEXT("Frost"), TEXT("Storm"), TEXT("Foundry"), TEXT("Eclipse")};
  static const TCHAR* Roles[] = {TEXT("Warden"), TEXT("Artisan"), TEXT("Scout")};
  return FText::FromString(FString::Printf(TEXT("%s %s %04d"),
    Regions[FMath::Clamp(RegionIndex, 0, 5)], Roles[FMath::Clamp(RoleIndex, 0, 2)], PopulationNpcId + 1));
 }
 return FText::FromString(Names[FMath::Clamp(RegionIndex, 0, 5)][FMath::Clamp(RoleIndex, 0, 2)]);
}
FText AMoonfallQuestGuide::GetPersonalityLabel() const
{
 static const TCHAR* Traits[6][3] = {
  {TEXT("protective"), TEXT("pragmatic"), TEXT("cautious")},
  {TEXT("patient"), TEXT("curious"), TEXT("suspicious")},
  {TEXT("disciplined"), TEXT("reserved"), TEXT("loyal")},
  {TEXT("visionary"), TEXT("impulsive"), TEXT("bold")},
  {TEXT("methodical"), TEXT("inventive"), TEXT("dutiful")},
  {TEXT("mysterious"), TEXT("compassionate"), TEXT("watchful")}
 };
 if (PopulationNpcId >= 18 && PopulationNpcId < 1111) {
  static const TCHAR* Variants[] = {TEXT("curious"), TEXT("protective"), TEXT("reserved"),
    TEXT("spirited"), TEXT("pragmatic"), TEXT("kind"), TEXT("suspicious"), TEXT("loyal")};
  return FText::FromString(Variants[PopulationNpcId % 8]);
 }
 return FText::FromString(Traits[FMath::Clamp(RegionIndex, 0, 5)][FMath::Clamp(RoleIndex, 0, 2)]);
}

void AMoonfallQuestGuide::BeginPlay()
{
 Super::BeginPlay();
 HomeLocation = GetActorLocation();
}
void AMoonfallQuestGuide::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 if (!GetWorld()) return;
 const APawn* Hero = UGameplayStatics::GetPlayerPawn(this, 0);
 if (!IsValid(Hero) || FVector::DistSquared2D(Hero->GetActorLocation(), HomeLocation)
     > FMath::Square(2600.f)) return;
 const float Now = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Now)) return;
 // Eight-minute local day clock. Four shifts with different work priorities
 // by occupation: guides keep watch, artisans work, scouts patrol.
 DayPhase = FMath::FloorToInt(FMath::Fmod(Now, 480.f) / 120.f);
 const int32 Role = FMath::Clamp(RoleIndex, 0, 2);
 static const int32 RoutineByRoleAndPhase[3][4] = {
  {0, 1, 0, 3}, // guide: watch, visit, watch, sleep
  {1, 2, 1, 3}, // artisan: work, trade, work, sleep
  {0, 2, 0, 1}  // scout: patrol, scout, patrol, night watch
 };
 CurrentRoutine = RoutineByRoleAndPhase[Role][FMath::Clamp(DayPhase, 0, 3)];
 if (Now < ConversationPausedUntil) return;
 const FVector Waypoints[] = {
  HomeLocation,
  HomeLocation + FVector(180.f, 65.f, 0.f),
  HomeLocation + FVector(-130.f, -100.f, 0.f),
  HomeLocation + FVector(55.f, -160.f, 0.f)
 };
 const FVector Goal = Waypoints[CurrentRoutine];
 const FVector Here = GetActorLocation();
 const FVector ToGoal(Goal.X - Here.X, Goal.Y - Here.Y, 0.f);
 const float Distance = ToGoal.Size();
 if (Distance < 10.f) return;
 // Small swept steps, no automatic teleporting or costly path search.
 const float Step = FMath::Min(Distance, 90.f * FMath::Clamp(DeltaSeconds, 0.f, 0.5f));
 FHitResult Hit;
 AddActorWorldOffset(ToGoal.GetSafeNormal() * Step, true, &Hit);
}
void AMoonfallQuestGuide::ReactToPlayer(int32 Trust, int32 Fear)
{
 if (!GetWorld()) return;
 const float Now = GetWorld()->GetTimeSeconds();
 ConversationPausedUntil = Now + 5.f;
 CurrentMood = Fear >= 55 ? 2 : Trust >= 50 ? 1 : 0;
 // Pose changes only when the disposition actually changes; no per-frame IK.
 if (CurrentMood != FMath::RoundToInt(LastVisualMoodUpdate)) {
  LastVisualMoodUpdate = static_cast<float>(CurrentMood);
  if (LeftArm && RightArm) {
   const float Wave = CurrentMood == 1 ? 37.f : CurrentMood == 2 ? -35.f : -15.f;
   LeftArm->SetRelativeRotation(FRotator(Wave, 0.f, -14.f));
   RightArm->SetRelativeRotation(FRotator(Wave, 0.f, 14.f));
  }
 }
}

FText AMoonfallQuestGuide::GetRoutineLabel() const
{
 const int32 Role = FMath::Clamp(RoleIndex, 0, 2);
 static const TCHAR* Labels[3][4] = {
  {TEXT("Keeping watch"), TEXT("Checking the square"), TEXT("Reviewing patrols"), TEXT("Resting")},
  {TEXT("Preparing supplies"), TEXT("Trading wares"), TEXT("Gathering materials"), TEXT("Resting")},
  {TEXT("Patrolling roads"), TEXT("Night patrol"), TEXT("Surveying trails"), TEXT("Resting")}
 };
 return FText::FromString(Labels[Role][FMath::Clamp(CurrentRoutine, 0, 3)]);
}
