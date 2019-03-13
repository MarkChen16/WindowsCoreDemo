
#include "stdafx.h"
#include "DLLAPI.h"

int max = 0;

DLLAPI void getMax(int x, int y)
{
	if (x > y) max = x;
	else max = y;
}
