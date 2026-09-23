#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__


#include <cmath>
    
const float MY_PI = 3.14159265358979323846f;  
 
using namespace std;



typedef struct
{
  float x1,y1,x2,y2;
} BoundaryType; 



/// Function Prototypes ////////////////////////////////////////////////////////////////////

float degToRad(float deg);
int xDev(const BoundaryType &WorldBound,const BoundaryType &DevBound,float xworld);
int yDev(const BoundaryType &WorldBound,const BoundaryType &DevBound,float yworld);


#endif

