#pragma once

// http://stackoverflow.com/questions/6078799/minimize-restore-windows-programmatically-skipping-the-animation-effect

#include <windows.h>

class AnimationSuppressor
{
public:
	AnimationSuppressor();
	~AnimationSuppressor();

private:
	int original;
	bool suppressed;
};