#include "MoonfallNpcSocialSessionSubsystem.h"
void UMoonfallNpcSocialSessionSubsystem::Deinitialize(){
 Relationships.Reset();
 Super::Deinitialize();
}
