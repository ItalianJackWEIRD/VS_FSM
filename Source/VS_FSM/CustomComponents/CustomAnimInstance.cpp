// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComponents/CustomAnimInstance.h"

bool UCustomAnimInstance::ShouldStanceTransition()
{
	if (bShouldStanceTransition)
	{
		bShouldStanceTransition = false;
		return true;
	}
	else 
		return false;
}
