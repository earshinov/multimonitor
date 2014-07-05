#include "AnimationSuppressor.h"
#include "utils.h"

AnimationSuppressor::AnimationSuppressor()
{
	suppressed = false;
	
	ANIMATIONINFO settings = { sizeof(settings) };
	if (!::SystemParametersInfo(SPI_GETANIMATION, settings.cbSize, &settings, 0))
	{
		winapiError(L"SystemParametersInfo(1) failed");
		return;
	}

	original = settings.iMinAnimate;

	settings.iMinAnimate = 0;
	if (!::SystemParametersInfo(SPI_SETANIMATION, settings.cbSize, &settings, SPIF_SENDCHANGE))
	{
		winapiError(L"SystemParametersInfo(2) failed");
		return;
	}

	suppressed = true;
}

AnimationSuppressor::~AnimationSuppressor()
{
	if (!suppressed)
		return;

	ANIMATIONINFO settings = { sizeof(settings) };
	if (!::SystemParametersInfo(SPI_SETANIMATION, settings.cbSize, &settings, SPIF_SENDCHANGE))
	{
		winapiError(L"SystemParametersInfo(3) failed");
		return;
	}
}
