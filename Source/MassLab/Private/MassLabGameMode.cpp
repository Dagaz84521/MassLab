#include "MassLabGameMode.h"

#include "MassLabPlayer.h"
#include "MassLabPlayerController.h"

AMassLabGameMode::AMassLabGameMode()
{
	DefaultPawnClass = AMassLabPlayer::StaticClass();
	PlayerControllerClass = AMassLabPlayerController::StaticClass();
}
