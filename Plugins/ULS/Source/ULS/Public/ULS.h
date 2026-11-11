#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

ULS_API DECLARE_LOG_CATEGORY_EXTERN(LogULS, Log, All);

class FULSModule : public IModuleInterface
{
};

#define ECC_PipeEntry ECC_GameTraceChannel3