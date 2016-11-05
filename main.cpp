//#include <windows.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

using namespace std;

//include stuff we use
typedef vector<float> fvec;
typedef vector<fvec> fmatrix;
typedef vector<int> ivec;
typedef vector<ivec> imatrix;
typedef vector<bool> bvec;
typedef vector<bvec> bmatrix;
typedef vector<bmatrix> bmatrix3d;

//ostreams
ostream &operator<<(ostream &stream, fvec &vec)
{
    for (int i = 0; i < (int)vec.size(); i++)
        stream << vec[i] << " ";

    return stream;
}

ostream &operator<<(ostream &stream, fmatrix &matrix)
{
    for (int i = 0; i < (int)matrix.size(); i++)
        stream << matrix[i] << endl;

    return stream;
}

ostream &operator<<(ostream &stream, ivec &vec)
{
    for (int i = 0; i < (int)vec.size(); i++)
        stream << vec[i] << " ";

    return stream;
}

ostream &operator<<(ostream &stream, imatrix &matrix)
{
    for (int i = 0; i < (int)matrix.size(); i++)
        stream << matrix[i] << endl;

    return stream;
}

ostream &operator<<(ostream &stream, bvec &vec)
{
    for (int i = 0; i < (int)vec.size(); i++)
        stream << vec[i] << " ";

    return stream;
}

ostream &operator<<(ostream &stream, bmatrix &matrix)
{
    for (int i = 0; i < (int)matrix.size(); i++)
        stream << matrix[i] << endl;

    return stream;
}

ostream &operator<<(ostream &stream, bmatrix3d &matrix3d)
{
    for (int i = 0; i < (int)matrix3d.size(); i++)
        stream << matrix3d[i] << endl << endl;

    return stream;
}

void outputRoundedFvec(fvec theVec)
{
    for (int i = 0; i < (int)theVec.size(); i++)
    {
        if (theVec[i]-(int)theVec[i] >= 0.5) //for rounding up
            theVec[i] = (int) (theVec[i] + 0.5);
        cout << (int)theVec[i] << " ";
    }
}
//matrices
fmatrix lScape; //stores heights of terrain bits
bmatrix useable;
bmatrix3d lakes; //stores each of the lakes the program finds

int size = 65; //grid size (e.g. 33x33) - must be 2^n +1
float HorScale = 1.0f; //scales horizontal direction for rendering
float vertScale = 1.0f; //scales vertical direction for rendering

// angle of rotation for the camera direction
float angle = 0.0f;
// actual vector representing the camera's direction
float lx=0.0f, lz=-1.0f;
// XZ position of the camera
float x=float(size/2)*HorScale, z=float(size/2)*HorScale, y = 0.0f;
// the key states. These variables will be zero
//when no key is being presses
float deltaAngle = 0.0f;
float deltaMove = 0;
float deltaUp = 0;
//float deltaWater = 0.0f;

float waterY = -0.1f;
int cFactor = 3;

float high = 0.0f; //max height point on map
int highX = 0; //x coordinate of highest point
int highY = 0; //y coordinate of highest point
float avHeight = 0.0f; //average height of map
float low = 0.0f; //lowest height on map
int largestLake = -1; //which lake in array is the largest one
int largestLakeSize = -1; //how large the above lake is
int howManyLakes = 0; //how many lakes there are
imatrix riverPath; //list of coordinate pairs for river to go between

//mouse controls stuff
float zoom = 15.0f;
float rotx = 0;
float roty = 0.001f;
float tx = 0;
float ty = 0;
int lastx=0;
int lasty=0;
unsigned char Buttons[3] = {0};

int specialMode = 0; //what mode for displaying colours

void midPoint(int x,int y, int cDist, int noRepeat);
void diamond(int x,int y, int dist);
void seed();
void arch(float height, int x_pt1, int y_pt1, int x_pt2, int y_pt2);
void findLakePos();
void generateLake(int x, int y, float height);

void Motion(int x,int y)
{
	int diffx=x-lastx;
	int diffy=y-lasty;
	lastx=x;
	lasty=y;

	if(Buttons[1])
	{
		zoom -= (float) 0.07f * diffy;
	}
	else
    {
        if(Buttons[0])
		{
			rotx += (float) 0.5f * diffy;
			roty += (float) 0.5f * diffx;
		}
		else
			if(Buttons[2])
			{
				tx += (float) 0.05f * diffx;
				ty -= (float) 0.05f * diffy;
			}
			glutPostRedisplay();
    }
}

void Mouse(int b,int s,int x,int y)
{
	lastx=x;
	lasty=y;
	switch(b)
	{
	case GLUT_LEFT_BUTTON:
		Buttons[0] = ((GLUT_DOWN==s)?1:0);
		break;
	case GLUT_MIDDLE_BUTTON:
		Buttons[1] = ((GLUT_DOWN==s)?1:0);
		break;
	case GLUT_RIGHT_BUTTON:
		Buttons[2] = ((GLUT_DOWN==s)?1:0);
		break;
	default:
		break;
	}
	glutPostRedisplay();
}

void changeSize(int w, int h)
{
    // Prevent a divide by zero, when window is too short
    // (you cant make a window of zero width).

    if (h == 0)
        h = 1;
    float ratio =  w * 1.0 / h;

    // Use the Projection Matrix
    glMatrixMode(GL_PROJECTION);

    // Reset Matrix
    glLoadIdentity();

    // Set the viewport to be the entire window
    glViewport(0, 0, w, h);

    // Set the correct perspective.
    gluPerspective(45.0f, ratio, 0.1f, 1000.0f);

    // Get Back to the Modelview
    glMatrixMode(GL_MODELVIEW);
}


void computePos(float deltaMove)
{

    x += deltaMove * lx * 0.1f;
    z += deltaMove * lz * 0.1f;
}

void computeDir(float deltaAngle)
{

    angle += deltaAngle;
    lx = sin(angle);
    lz = -cos(angle);
}

void computeUp()
{
    y += deltaUp;
}

/*
void computeWater()
{
    waterY += deltaWater;
}
*/

void smoothTerrain()
{
    //loops through all the terrain
    for (int i=0; i<lScape[0].size(); i++)
    {
        for (int j=0; j<lScape[0].size(); j++)
        {
            //check for a down, up down pattern to detect spikes
            //if it finds them, it averages the spike point out

            int midHeight = lScape[i][j];

            if (i+3 < lScape[0].size()) //if not off right
                //test right
                if (lScape[i+1][j] < lScape[i][j] + 5 //arbitrary value that seems to work
                && lScape[i+2][j] > lScape[i+1][j]
                && lScape[i+3][j]< lScape[i][j] + 5)
                    lScape[i+1][j] = (lScape[i][j]+lScape[i+2][j])/2;
            if (j+3 < lScape[0].size()) //if not off bottom
                //test bottom
                if (lScape[i][j+1] < lScape[i][j] + 5
                && lScape[i][j+2] > lScape[i][j+1]
                && lScape[i][j+3]< lScape[i][j] + 5)
                    lScape[i][j+1] = (lScape[i][j]+lScape[i][j+2])/2;
        }
    }
}

void renderScene(void)
{
    //computing movement
    if (deltaMove)
        computePos(deltaMove);
    if (deltaAngle)
        computeDir(deltaAngle);
    if (deltaUp)
        computeUp();
    //if (deltaWater)
    //    computeWater();

    // Clear Color and Depth Buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset transformations
    glLoadIdentity();

    //TODO - make camera rotate around itself, not middle point
	glTranslatef(0,0,-zoom);
	glTranslatef(tx,ty,0);
	glRotatef(rotx,1,0,0);
	glRotatef(roty,0,1,0);

    // Set the camera
    gluLookAt(	x, y, z,
                x+lx, y,  z+lz,
                0.0f, 1.0f, 0.0f);

    glColor3f(0.0f, 0.0f, 1.0f);

    //water plane
    glBegin(GL_TRIANGLES);
    glVertex3f(0, waterY*vertScale, 0);
    glVertex3f(0, waterY*vertScale, float((size-1)*HorScale));
    glVertex3f(float((size-1)*HorScale), waterY*vertScale, 0);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex3f(0, waterY*vertScale, float((size-1)*HorScale));
    glVertex3f(float((size-1)*HorScale), waterY*vertScale, 0);
    glVertex3f(float((size-1)*HorScale), waterY*vertScale, float((size-1)*HorScale));
    glEnd();

    //draw terrain bits
    for (int a = 0; a < lScape.size()-1; a++)
    {
        for (int b = 0; b < lScape[0].size()-1; b++)
        {
            //draw outlines on grid
            /*glColor3f(0.0f, 0.0f, 0.0f);

            glBegin(GL_LINES); // Draw lines between points
            glVertex3f(b*HorScale, lScape[a][b]*vertScale, a*HorScale);
            glVertex3f((b+1)*HorScale, lScape[a][b+1]*vertScale, a*HorScale);
            glEnd();

            glBegin(GL_LINES); // Draw lines between points
            glVertex3f(b*HorScale, lScape[a][b]*vertScale, a*HorScale);
            glVertex3f(b*HorScale, lScape[a+1][b]*vertScale, (a+1)*HorScale);
            glEnd();

            glBegin(GL_LINES);// Draw lines between points
            glVertex3f((b+1)*HorScale, lScape[a+1][b+1]*vertScale, (a+1)*HorScale);
            glVertex3f(b*HorScale, lScape[a+1][b]*vertScale, (a+1)*HorScale);
            glEnd();

            glBegin(GL_LINES); // Draw lines between points
            glVertex3f((b+1)*HorScale, lScape[a+1][b+1]*vertScale, (a+1)*HorScale);
            glVertex3f(b*HorScale, lScape[a+1][b]*vertScale, (a+1)*HorScale);
            glEnd();*/

            if (specialMode==0)
            {
                float cTest = (lScape[a][b] + lScape[a][b+1] + lScape[a+1][b] + lScape[a+1][b+1])/4;

                if (cTest > waterY)
                    glColor3f((cTest-waterY)/(high-waterY), 0.2f + (cTest-waterY)/(high-waterY), (cTest-waterY)/(high-waterY));
                else
                    glColor3f(0.0f, 0.25f, sqrt((cTest-waterY)*(cTest-waterY)));
            }
            else if (specialMode==1) //disco mode
            {
                glColor3f(rand()%255, rand()%255, rand()%255);
            }
            else if (specialMode==2) //seizure mode
            {
                glColor3f(double((rand()%350)/100), double((rand()%350)/100), double((rand()%350)/100));
            }

            glBegin(GL_TRIANGLES);
            glVertex3f(b*HorScale, lScape[a][b]*vertScale, a*HorScale);
            glVertex3f((b+1)*HorScale, lScape[a][b+1]*vertScale, a*HorScale);
            glVertex3f(b*HorScale, lScape[a+1][b]*vertScale, (a+1)*HorScale);
            glEnd();

            glBegin(GL_TRIANGLES);
            glVertex3f((b+1)*HorScale, lScape[a][b+1]*vertScale, a*HorScale);
            glVertex3f(b*HorScale, lScape[a+1][b]*vertScale, (a+1)*HorScale);
            glVertex3f((b+1)*HorScale, lScape[a+1][b+1]*vertScale, (a+1)*HorScale);
            glEnd();
        }
    }

    //draw river, if there is one
    if (riverPath.size()>1)
    {
        glColor3f(0.0f, 0.0f, 1.0f);
        glLineWidth(30.0f); //TODO - tweak this

        for (int a=0; a<riverPath.size()-1; a++)
        {
            glBegin(GL_LINES); // Draw lines between points
            //x,y in the riverpath grid corresponds to x,z in the drawn grid, as lScape contains the heights, y
            glVertex3f((riverPath[a][0])*HorScale, (lScape[riverPath[a][1]][riverPath[a][0]])*vertScale, riverPath[a][1]*HorScale);
            glVertex3f((riverPath[a+1][0])*HorScale, (lScape[riverPath[a+1][1]][riverPath[a+1][0]])*vertScale, riverPath[a+1][1]*HorScale);
            glEnd();
        }
    }

    glutSwapBuffers();
}

void processNormalKeys(unsigned char key, int xx, int yy)
{
    if (key == 27)
        exit(0);
}

void pressKey(int key, int xx, int yy)
{
    switch (key)
    {
    case GLUT_KEY_LEFT :
        deltaAngle = -0.01f;
        break;
    case GLUT_KEY_RIGHT :
        deltaAngle = 0.01f;
        break;
    case GLUT_KEY_UP :
        deltaMove = 2.0f;
        break;
    case GLUT_KEY_DOWN :
        deltaMove = -1.0f;
        break;
    case GLUT_KEY_PAGE_DOWN:
        deltaUp -= 0.5f;
        break;
    case GLUT_KEY_PAGE_UP:
        deltaUp += 0.5f;
        break;
    case GLUT_KEY_F1:
        specialMode = 1; //set display mode to disco mode
        break;
    case GLUT_KEY_F2:
        specialMode = 2; //set display mode to seizure mode
        break;
    case GLUT_KEY_F3:
        specialMode = 0; //reset display mode
        break;
    }
}

void releaseKey(int key, int x, int y)
{
    switch (key)
    {
    case GLUT_KEY_LEFT :
    case GLUT_KEY_RIGHT :
        deltaAngle = 0.0f;
        break;
    case GLUT_KEY_UP :
    case GLUT_KEY_DOWN :
        deltaMove = 0;
        break;
    case GLUT_KEY_PAGE_DOWN:
    case GLUT_KEY_PAGE_UP:
        deltaUp = 0;
        break;
    //case GLUT_KEY_F1:
    //case GLUT_KEY_F2:
    //    deltaWater = 0;
    //    break;
    }
}

/*
void calcWaterY()
{
    int total = 0;
    int count = 0;

    for (int a = 0; a < lScape.size(); a++)
    {
        for (int b = 0; b < lScape[0].size(); b++)
        {
            total += lScape[a][b];
            count++;
            if (lScape[a][b] < low)
                low = lScape[a][b];
            if (lScape[a][b] > high)
            {
                high = lScape[a][b];
                highX = a;
                highY = b;
            }
        }
    }

    total /= count;
    avHeight = total;
    y = (total + high) / 2;
    waterY = total; //(total + low)/2;
}
*/

//based on a note from in class
void changeConnectedWater(fmatrix &cells, bmatrix &result, int r, int c, int &howMany)
{
    int maxRow = (int) cells.size();
    int maxCol = (int) cells[0].size();

    if (r<0 || c<0 || r>=maxRow || c>=maxCol)
        return;

    //if the neighbouring cell in question is below water level, and not found yet
    if ((cells[r][c] < waterY) && (result[r][c] == false))
    {
        result[r][c] = true; //it's part of a lake
        howMany++; //add one to lake size counter
        //check all the cells around the cell in question
        changeConnectedWater(cells, result, r-1, c, howMany);
        changeConnectedWater(cells, result, r+1, c, howMany);
        changeConnectedWater(cells, result, r, c-1, howMany);
        changeConnectedWater(cells, result, r, c+1, howMany);
    }
}

void makeRiverPath()
{
    //stores current river x and y coordinates, starts at highest point
    int currX = highY;
    int currY = highX;
    cout << "highX=" << highX << " highY=" << highY << endl;
    //if river is done or not
    bool keepGoing = true;

    //include first point in riverPath
    ivec firstCoord;
    firstCoord.push_back(currY); //y then x, since pushback goes backwards
    firstCoord.push_back(currX);
    riverPath.push_back(firstCoord);

    cout << "largestLake=" << largestLake << " currX=" << currX << " currY=" << currY << endl;

    cout << "at beginning" << endl;

    //keep generating river, while river can be generated
    while (keepGoing)
    {
        cout << "here" << endl;
        //declare and pick a lake location
        int lakeX = rand() % size;
        int lakeY = rand() % size;
        cout << "initialLakeCoordGen, lakeX=" << lakeX << ", lakeY=" << lakeY << endl;
        //pick random point in biggest lake
        while (!lakes[largestLake][lakeY][lakeX]) //while the selected cell is not in the lake
        {
            cout << "before assignment" << endl;
            lakeX = rand() % size;
            //cout << "after lakeX" << endl;
            lakeY = rand() % size;
            //cout << "randomLakeLoop, lakeX=" << lakeX << ", lakeY=" << lakeY << endl;
        }
        cout << "LakeCoordGen, lakeX=" << lakeX << ", lakeY=" << lakeY << endl;

        cout << "currX=" << currX << " currY=" << currY << endl;

        ivec riverCoords;
        cout << "after random" << endl;

        //if above
        if (currY < lakeY)
        {
            cout << "if above" << endl;
            if (currX < lakeX) //if to the left
            {
                cout << "if left" << endl;
                float firstMin = min(lScape[currY+1][currX], lScape[currY][currX+1]); //min of two side options
                cout << "firstMin=" << firstMin << " other=" << lScape[currY+1][currX+1] << endl;

                if (lScape[currY+1][currX+1] < firstMin) //if other is min
                {
                    if (lScape[currY][currX] > lScape[currY+1][currX+1]) //if valid (below current point)
                    {
                        currX = currX+1; //move river along
                        currY = currY+1;
                    }
                    else
                        keepGoing = false; //river is done
                }
                else if (lScape[currY+1][currX] == firstMin) //if first is min
                {
                    if (lScape[currY][currX] > lScape[currY+1][currX]) //if valid (below current point)
                    {
                        currY = currY+1;
                    }
                    else
                        keepGoing = false;
                }
                else if (lScape[currY][currX+1] == firstMin) //if second is min
                {
                    if (lScape[currY][currX] > lScape[currY][currX+1]) //if valid (below current point)
                    {
                        currX = currX+1;
                    }
                    else
                        keepGoing = false;
                }
            }
            else if (currX > lakeX) //if to the right
            {
                cout << "if right" << endl;
                float firstMin = min(lScape[currY][currX-1], lScape[currY+1][currX]); //min of two side options

                cout << "firstMin=" << firstMin << " other=" << lScape[currY+1][currX-1] << endl;

                if (lScape[currY+1][currX-1] < firstMin) //if other is min
                {
                    if (lScape[currY][currX] > lScape[currY+1][currX-1]) //if valid (below current point)
                    {
                        currX = currX-1;
                        currY = currY+1;
                    }
                    else
                        keepGoing = false;
                }
                else if (lScape[currY][currX-1] == firstMin) //if first is min
                {
                    if (lScape[currY][currX] > lScape[currY][currX-1]) //if valid (below current point)
                    {
                        currX = currX-1;
                    }
                    else
                        keepGoing = false;
                }
                else if (lScape[currY+1][currX] == firstMin) //if second is min
                {
                    if (lScape[currY][currX] > lScape[currY+1][currX]) //if valid (below current point)
                    {
                        currY = currY+1;
                    }
                    else
                        keepGoing = false;
                }
            }
            else if (currX == lakeX) //if directly above
            {
                cout << "if ==" << endl;
                if (currX==0) //special case so doesn't check locations not on the grid
                {
                    if (lScape[currY+1][currX] < lScape[currY+1][currX+1])
                    {
                        if (lScape[currY][currX] > lScape[currY+1][currX]) //if valid (below current point)
                            currY = currY+1;
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY+1][currX+1] < lScape[currY+1][currX])
                    {
                        if (lScape[currY][currX] > lScape[currX+1][currX+1]) //if valid (below current point)
                        {
                            currX = currX+1;
                            currY = currY+1;
                        }
                        else
                            keepGoing = false;
                    }
                }
                else if (currX == size-1)  //special case so doesn't check locations not on the grid
                {
                    if (lScape[currY+1][currX] < lScape[currY+1][currX-1])
                    {
                        if (lScape[currY][currX] > lScape[currY+1][currX]) //if valid (below current point)
                            currY = currY+1;
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY+1][currX-1] < lScape[currY+1][currX])
                    {
                        if (lScape[currY][currX] > lScape[currY+1][currX-1]) //if valid (below current point)
                        {
                            currX = currX-1;
                            currY = currY+1;
                        }
                        else
                            keepGoing = false;
                    }
                }
                else
                {
                    float firstMin = min(lScape[currY+1][currX-1], lScape[currY+1][currX+1]); //min of two side options

                    cout << "firstMin=" << firstMin << " other=" << lScape[currY+1][currX] << endl;

                    if (lScape[currY+1][currX] < firstMin) //if other is min
                    {
                        if (lScape[currY][currX] > lScape[currY+1][currX]) //if valid (below current point)
                        {
                            currY = currY+1;
                        }
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY+1][currX-1] == firstMin) //if first is min
                    {
                        if (lScape[currY][currX] > lScape[currY+1][currX-1]) //if valid (below current point)
                        {
                            currX = currX-1;
                            currY = currY+1;
                        }
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY+1][currX+1] == firstMin) //if second is min
                    {
                        if (lScape[currY][currX] > lScape[currY+1][currX+1]) //if valid (below current point)
                        {
                            currX = currX+1;
                            currY = currY+1;
                        }
                        else
                            keepGoing = false;
                    }
                }
            }
        }
        //if below
        else if (currY > lakeY)
        {
            cout << "if below" << endl;
            if (currX < lakeX) //if to the left
            {
                cout << "if left" << endl;
                float firstMin = min(lScape[currY-1][currX], lScape[currY][currX+1]); //min of two side options

                cout << "firstMin=" << firstMin << " other=" << lScape[currY-1][currX+1] << endl;

                if (lScape[currY-1][currX+1] < firstMin) //if other is min
                {
                    if (lScape[currY][currX] > lScape[currY-1][currX+1]) //if valid (below current point)
                    {
                        currX = currX+1;
                        currY = currY-1;
                    }
                    else
                        keepGoing = false;
                }
                else if (lScape[currY-1][currX] == firstMin) //if first is min
                {
                    if (lScape[currY][currX] > lScape[currY-1][currX]) //if valid (below current point)
                    {
                        currY = currY-1;
                    }
                    else
                        keepGoing = false;
                }
                else if (lScape[currY][currX+1] == firstMin) //if second is min
                {
                    if (lScape[currY][currX] > lScape[currY][currX+1]) //if valid (below current point)
                    {
                        currX = currX+1;
                    }
                    else
                        keepGoing = false;
                }
            }
            else if (currX > lakeX) //if to the right
            {
                cout << "if right" << endl;
                float firstMin = min(lScape[currY][currX-1], lScape[currY-1][currX]); //min of two side options

                cout << "firstMin=" << firstMin << " other=" << lScape[currY-1][currX-1] << endl;

                if (lScape[currY-1][currX-1] < firstMin) //if other is min
                {
                    if (lScape[currY][currX] > lScape[currY-1][currX-1]) //if valid (below current point)
                    {
                        currX = currX-1;
                        currY = currY-1;
                    }
                    else
                        keepGoing = false;
                }
                else if (lScape[currY][currX-1] == firstMin) //if first is min
                {
                    if (lScape[currY][currX] > lScape[currY][currX-1]) //if valid (below current point)
                    {
                        currX = currX-1;
                    }
                    else
                        keepGoing = false;
                }
                else if (lScape[currY-1][currX] == firstMin) //if second is min
                {
                    if (lScape[currY][currX] > lScape[currY-1][currX]) //if valid (below current point)
                    {
                        currY = currY-1;
                    }
                    else
                        keepGoing = false;
                }
            }
            else if (currX == lakeX) //if directly below
            {
                cout << "if ==" << endl;
                if (currX==0) //special case so doesn't check locations not on the grid
                {
                    if (lScape[currY-1][currX] < lScape[currY-1][currX+1])
                    {
                        if (lScape[currY][currX] > lScape[currY-1][currX]) //if valid (below current point)
                            currY = currY-1;
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY-1][currX+1] < lScape[currY-1][currX])
                    {
                        if (lScape[currY][currX] > lScape[currY-1][currX+1]) //if valid (below current point)
                        {
                            currX = currX+1;
                            currY = currY-1;
                        }
                        else
                            keepGoing = false;
                    }
                }
                else if (currX == size-1) //special case so doesn't check locations not on the grid
                {
                    if (lScape[currY-1][currX] < lScape[currY-1][currX-1])
                    {
                        if (lScape[currY][currX] > lScape[currY-1][currX]) //if valid (below current point)
                            currY = currY-1;
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY-1][currX-1] < lScape[currY-1][currX])
                    {
                        if (lScape[currY][currX] > lScape[currY-1][currX-1]) //if valid (below current point)
                        {
                            currX = currX-1;
                            currY = currY-1;
                        }
                        else
                            keepGoing = false;
                    }
                }
                else
                {
                    float firstMin = min(lScape[currY-1][currX-1], lScape[currY-1][currX+1]); //min of two side options

                    cout << "firstMin=" << firstMin << " other=" << lScape[currY-1][currX] << endl;

                    if (lScape[currY-1][currX] < firstMin) //if other is min
                    {
                        if (lScape[currY][currX] > lScape[currY-1][currX]) //if valid (below current point)
                        {
                            currY = currY-1;
                        }
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY-1][currX-1] == firstMin) //if first is min
                    {
                        if (lScape[currY][currX] > lScape[currY-1][currX-1]) //if valid (below current point)
                        {
                            currX = currX-1;
                            currY = currY-1;
                        }
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY-1][currX+1] == firstMin) //if second is min
                    {
                        if (lScape[currY][currX] > lScape[currY-1][currX+1]) //if valid (below current point)
                        {
                            currX = currX+1;
                            currY = currY-1;
                        }
                        else
                            keepGoing = false;
                    }
                }
            }
        }
        //if on same Y level
        else if (currY == lakeY)
        {
            cout << "if same Y" << endl;
            if (currX < lakeX) //if to the left
            {
                cout << "if left" << endl;
                if (currY==0) //special case so doesn't check locations not on the grid
                {
                    if (lScape[currY][currX+1] < lScape[currY+1][currX+1])
                    {
                        if (lScape[currY][currX] > lScape[currY][currX+1]) //if valid (below current point)
                            currX = currX+1;
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY+1][currX+1] < lScape[currY][currX+1])
                    {
                        if (lScape[currY][currX] > lScape[currY+1][currX+1]) //if valid (below current point)
                        {
                            currX = currX+1;
                            currY = currY+1;
                        }
                        else
                            keepGoing = false;
                    }
                }
                else if (currY == size-1) //special case so doesn't check locations not on the grid
                {
                    if (lScape[currY][currX+1] < lScape[currY-1][currX+1])
                    {
                        if (lScape[currY][currX] > lScape[currY][currX+1]) //if valid (below current point)
                            currX = currX+1;
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY-1][currX+1] < lScape[currY][currX+1])
                    {
                        if (lScape[currY][currX] > lScape[currY-1][currX+1]) //if valid (below current point)
                        {
                            currX = currX+1;
                            currY = currY-1;
                        }
                        else
                            keepGoing = false;
                    }
                }
                else
                {
                    float firstMin = min(lScape[currY+1][currX+1], lScape[currY-1][currX+1]); //min of two side options
                    cout << "firstMin=" << firstMin << " other=" << lScape[currY][currX+1] << endl;

                    if (lScape[currY][currX+1] < firstMin) //if other is min
                    {
                        if (lScape[currY][currX] > lScape[currY][currX+1]) //if valid (below current point)
                        {
                            currX = currX+1;
                        }
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY+1][currX+1] == firstMin) //if first is min
                    {
                        if (lScape[currY][currX] > lScape[currY+1][currX+1]) //if valid (below current point)
                        {
                            currX = currX+1;
                            currY = currY+1;
                        }
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY-1][currX+1] == firstMin) //if second is min
                    {
                        if (lScape[currY][currX] > lScape[currY-1][currX+1]) //if valid (below current point)
                        {
                            currX = currX+1;
                            currY = currY-1;
                        }
                        else
                            keepGoing = false;
                    }
                }
            }
            else if (currX > lakeX) //if to the right
            {
                cout << "if right" << endl;
                if (currY==0) //special case so doesn't check locations not on the grid
                {
                    if (lScape[currY][currX-1] < lScape[currY+1][currX-1])
                    {
                        if (lScape[currY][currX] > lScape[currY][currX-1]) //if valid (below current point)
                            currX = currX-1;
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY+1][currX-1] < lScape[currY][currX-1])
                    {
                        if (lScape[currY][currX] > lScape[currY-1][currX-1]) //if valid (below current point)
                        {
                            currX = currX-1;
                            currY = currY-1;
                        }
                        else
                            keepGoing = false;
                    }
                }
                else if (currX == size-1) //special case so doesn't check locations not on the grid
                {
                    if (lScape[currY][currX-1] < lScape[currY-1][currX-1])
                    {
                        if (lScape[currY][currX] > lScape[currY][currX-1]) //if valid (below current point)
                            currX = currX-1;
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY-1][currX-1] < lScape[currY][currX-1])
                    {
                        if (lScape[currY][currX] > lScape[currY-1][currX-1]) //if valid (below current point)
                        {
                            currX = currX-1;
                            currY = currY-1;
                        }
                        else
                            keepGoing = false;
                    }
                }
                else
                {
                    float firstMin = min(lScape[currY+1][currX-1], lScape[currY-1][currX-1]); //min of two side options

                    cout << "firstMin=" << firstMin << " other=" << lScape[currY][currX-1] << endl;

                    if (lScape[currY][currX-1] < firstMin) //if other is min
                    {
                        if (lScape[currY][currX] > lScape[currY][currX-1]) //if valid (below current point)
                        {
                            currX = currX-1;
                        }
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY+1][currX-1] == firstMin) //if first is min
                    {
                        if (lScape[currY][currX] > lScape[currY+1][currX-1]) //if valid (below current point)
                        {
                            currX = currX-1;
                            currY = currY+1;
                        }
                        else
                            keepGoing = false;
                    }
                    else if (lScape[currY-1][currX+1] == firstMin) //if second is min
                    {
                        if (lScape[currY][currX] > lScape[currY-1][currX+1]) //if valid (below current point)
                        {
                            currX = currX+1;
                            currY = currY-1;
                        }
                        else
                            keepGoing = false;
                    }
                }
            }
            else if (currX == lakeX) //if directly on the spot
            {
                cout << "if ==" << endl;
                //TODO - this is below water level and should never happen
            }
        }

        //add the next point to the riverPath vector
        riverCoords.push_back(currY); //y then x, since push_back goes backwards
        riverCoords.push_back(currX);
        riverPath.push_back(riverCoords);

        if (lScape[currY][currX]<waterY)
            keepGoing = false; //TODO - fix so it doesn't duplicate last entry
    }

    cout << "riverPath(" << riverPath.size()-1 << " segments):" << endl << riverPath << endl;
    cout << "What is being drawn:" << endl;

    for (int a=0; a<riverPath.size()-1; a++)
    {
        cout << "(" << riverPath[a][0] << "," << lScape[riverPath[a][1]][riverPath[a][0]] << "," << riverPath[a][1] << ")-->";
        cout << "(" << riverPath[a+1][0] << "," << lScape[riverPath[a+1][1]][riverPath[a+1][0]] << "," << riverPath[a+1][1] << ")" << endl;
    }

    return;
}

int main(int argc, char **argv)
{
    srand ((unsigned) time (0)); // seed random numbers

    fvec line;

    for (int i = 0; i < size; i++) // generating a generic line
        line.push_back(-100);

    for (int i = 0; i < size; i++) // generating generic board
        lScape.push_back(line);

    seed(); // generates mountains and valleys

    //creates four corners if they don't already exist
    if (lScape[0][0] == -100)
        lScape[0][0] = 0;
    if (lScape[0][lScape[0].size()-1] == -100)
        lScape[0][lScape[0].size()-1] = 0;
    if (lScape[lScape.size()-1][0] == -100)
        lScape[lScape.size()-1][0] = 0;
    if (lScape[lScape.size()-1][lScape[0].size()-1] == -100)
        lScape[lScape.size()-1][lScape[0].size()-1] = 0;

    //custon seeds
    //lScape[4][4] = 0;

    //generate terrain
    midPoint((lScape[0].size()/2), (lScape.size()/2), lScape.size()/2, lScape.size()/2);

    smoothTerrain(); //get rid of spikes

    findLakePos(); //generates a lake

    //calculates lakes, adds to a bmatrix3d
    for (int a = 0; a < lScape.size()-1; a++)
    {
        for (int b = 0; b < lScape[0].size()-1; b++)
        {
            //check if this cell is in a lake already before
            //generating connected cells

            bool isNew = true; //if the cell is not in a lake

            for (int i=0; i<lakes.size(); i++) //bmatrix3d
                if (lakes[i][a][b]) //if the current lake matrix includes the cell
                {
                    isNew = false;
                    break;
                }

            if (isNew)
            {
                bmatrix result;
                bvec line;
                int howMany = 0;

                for (int i = 0; i < lScape.size(); i++) // generating a generic line
                    line.push_back(false);

                for (int i = 0; i < lScape[0].size(); i++) // generating generic board
                    result.push_back(line);

                //find other parts connected tyo current lake
                changeConnectedWater(lScape, result, a, b, howMany);

                if (howMany > 10) //TODO - arbitrary value
                {
                    howManyLakes++;
                    lakes.push_back(result);
                    if (howMany>largestLakeSize) //if biggest lake
                    {
                        //reassign biggest lake-related variables
                        largestLakeSize = howMany;
                        largestLake = howManyLakes-1;
                    }
                }
            }
        }
    }

    cout << "waterY = " << waterY << endl;

    cout << "===========================TERRAIN================================" << endl;

    for (int i=0; i<lScape.size(); i++)
    {
        outputRoundedFvec(lScape[i]);
        cout << endl;
    }

    cout << "==============================LAKES===============================" << endl;

    cout << lakes << endl;

    if (lakes.size())
        makeRiverPath();

    // init GLUT and create window
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(0,0);
    glutInitWindowSize(500,500);
    glutCreateWindow("Content Generation");

    // register callbacks
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);
    glutIdleFunc(renderScene);
    glutKeyboardFunc(processNormalKeys);
    glutSpecialFunc(pressKey);
    glutIgnoreKeyRepeat(1);
    glutSpecialUpFunc(releaseKey);

    glutMouseFunc(Mouse);
	glutMotionFunc(Motion);

    // OpenGL init
    glEnable(GL_DEPTH_TEST);

    // enter GLUT event processing cycle
    glutMainLoop();

    return 0;
}

void midPoint(int x,int y, int cDist, int noRepeat)
{
    if (noRepeat < 0) // not infinitely recursive
        return;

    if (lScape[y][x] <= -100) // allows for seeding
    {
        int total = 0;

        total += lScape[y-cDist][x-cDist]; // multiple lines for debug
        total += lScape[y-cDist][x+cDist]; // and line shortening
        total += lScape[y+cDist][x-cDist];
        total += lScape[y+cDist][x+cDist];
        lScape[y][x] = total;

        if (rand() % 2) // random aspect
            lScape[y][x] += (float) 1/(1 +(rand() % cFactor));
        else
            lScape[y][x] -=(float) 1/(1 +(rand() % cFactor));
    }

    diamond(x-cDist, y, cDist); // generates size for corners
    diamond(x+cDist, y, cDist);
    diamond(x, y+cDist, cDist);
    diamond(x, y-cDist, cDist);

    int half = (cDist/2);

    midPoint(x+half, y+half, half, half-1);
    midPoint(x-half, y-half, half, half-1); //recursive gets other points
    midPoint(x+half, y-half, half, half-1);
    midPoint(x-half, y+half, half, half-1);
}

void diamond(int x,int y, int dist)
{
    if (lScape[y][x] > -100) // allows for seeding
        return;

    int total = 0;
    int count = 0;

    if (x-dist >= 0) // checks left
    {
        if (lScape[y][x-dist] >-100)
        {
            total = total + lScape[y][x-dist];
            count++;
        }
    }

    if (x+dist < lScape[0].size()) // checks right
    {
        if (lScape[y][x+dist] >-100)
        {
            total = total + lScape[y][x+dist];
            count++;
        }
    }

    if (y-dist >= 0) // checks down
    {
        if (lScape[y-dist][x] >-100)
        {
            total = total + lScape[y-dist][x];
            count++;
        }
    }

    if (y+dist < lScape.size()) // checks up
    {
        if (lScape[y+dist][x] >-100)
        {
            total = total + lScape[y+dist][x];
            count++;
        }
    }

    lScape[y][x] = total/count; // averages

    if (rand() % 2) // random
        lScape[y][x] +=  (float) 1/(1 +(rand() % cFactor));
    else
        lScape[y][x] -=  (float) 1/(1 +(rand() % cFactor));
}

void seed()
{
    ivec inX;
    ivec inY;

    fvec dist;
    fvec newDist;
    ivec dist_p1;
    ivec dist_p2;

    for (int i = 0; i < 2; i++) // generate initial position
    {
        int X = rand() % (lScape.size()-1);
        int Y = rand() % (lScape[0].size()-1);

        while (lScape[Y][X] > -100)
        {
            X = rand() % (lScape.size()-1);
            Y = rand() % (lScape[0].size()-1);
        }

        lScape[Y][X] = 0;
        inX.push_back(X);
        inY.push_back(Y);
    }

    for (int  a = 0; a < inX.size(); a++) // measures distance
    {
        for (int b = a +1; b < inX.size(); b++)
        {
            int X = inX[b] - inX[a];
            int Y = inY[b] - inY[a];
            dist_p1.push_back(a);
            dist_p2.push_back(b);
            dist.push_back(sqrt((X*X) + (Y*Y)));
        }
    }

    for (int i = 0; i < inX.size(); i++) // gives a direction
    {
        int direct = rand() % 8;
        int speed = rand() % 4;

        switch (direct)
        {
        case 0:
            if(inY[i] + speed < lScape.size()) // up
                inY[i] += speed;
            break;
        case 1:
            if(inY[i] + speed < lScape.size() && inX[i] + speed < lScape[0].size()) // up and right
            {
                inY[i] += speed;
                inX[i] += speed;
            }
            break;
        case 2:
            if(inX[i] + speed < lScape[0].size()) // right
                inX[i] += speed;
            break;
        case 3:
            if(inY[i] - speed >= 0 && inX[i] + speed < lScape[0].size()) // right and down
            {
                inY[i] -= speed;
                inX[i] += speed;
            }
            break;
        case 4:
            if(inY[i] - speed >= 0) // down
                inY[i] -= speed;
            break;
        case 5:
            if(inY[i] - speed >= 0 && inX[i] - speed >= 0) // left and down
            {
                inY[i] -= speed;
                inX[i] -= speed;
            }
            break;
        case 6:
            if(inX[i] - speed >= 0) // left
                inX[i] -= speed;
            break;
        case 7:
            if(inY[i] + speed < lScape.size() && inX[i] - speed >= 0) // left and up
            {
                inY[i] += speed;
                inX[i] -= speed;
            }
            break;
        default:
            break;
        }
    }

    for (int  a = 0; a < inX.size(); a++) // calculating the new distance
    {
        for (int b = a +1; b < inX.size(); b++)
        {
            int X = inX[b] - inX[a];
            int Y = inY[b] - inY[a];
            newDist.push_back(sqrt((X*X) + (Y*Y)));
        }
    }

    for (int i = 0; i < newDist.size(); i++) // sends all positions to be made into mountains
    {
        arch(1/(dist[i] - newDist[i]), inX[dist_p1[i]], inY[dist_p1[i]], inX[dist_p2[i]], inY[dist_p2[i]]);
    }
}

void arch(float height, int x_pt1, int y_pt1, int x_pt2, int y_pt2)
{
    if (isinf(height))
        height = 0;
    while (height > 10) // prevents massive spikes
        height /= 10;

    height *= 2;

    int midX = (x_pt1 + x_pt2)/2; // gets midpoints
    int midY = (y_pt1 + y_pt2)/2;

    lScape[midY][midX] = height;

    int direction = 0;
    int longest;
    int totalX;
    int totalY;

    // attempts to find the direction the of the mountain range or valley
    // checking top right
    int testX = midX + 1;
    int testY = midY + 1;
    totalX = sqrt((testX-x_pt1)*(testX-x_pt1)) + sqrt((testX-x_pt2)*(testX-x_pt2));
    totalY = sqrt((testY-y_pt1)*(testY-y_pt1)) + sqrt((testY-y_pt2)*(testY-y_pt2));
    longest = totalX + totalY;

    // midle right
    testX = midX + 1;
    testY = midY;
    totalX = sqrt((testX-x_pt1)*(testX-x_pt1)) + sqrt((testX-x_pt2)*(testX-x_pt2));
    totalY = sqrt((testY-y_pt1)*(testY-y_pt1)) + sqrt((testY-y_pt2)*(testY-y_pt2));
    if ( totalX + totalY > longest)
    {
        longest = totalX + totalY;
        direction = 1;
    }

    //bottom right
    testX = midX + 1;
    testY = midY - 1;
    totalX = sqrt((testX-x_pt1)*(testX-x_pt1)) + sqrt((testX-x_pt2)*(testX-x_pt2));
    totalY = sqrt((testY-y_pt1)*(testY-y_pt1)) + sqrt((testY-y_pt2)*(testY-y_pt2));
    if ( totalX + totalY > longest)
    {
        longest = totalX + totalY;
        direction = 2;
    }

    // top middle
    testX = midX;
    testY = midY + 1;
    totalX = sqrt((testX-x_pt1)*(testX-x_pt1)) + sqrt((testX-x_pt2)*(testX-x_pt2));
    totalY = sqrt((testY-y_pt1)*(testY-y_pt1)) + sqrt((testY-y_pt2)*(testY-y_pt2));
    if ( totalX + totalY > longest)
    {
        longest = totalX + totalY;
        direction = 3;
    }

    int dist = sqrt(((midX-x_pt1)*(midX-x_pt1))+((midY-y_pt1)*(midY-y_pt1))); // determines the length of the range/valley
    int toChangeX = midX;
    int toChangeY = midY;

    if (direction == 0) // so direction is only determined once
    {
        for (int i = 1; i <= dist; i++) // extends in the direction chosen previously
        {
            toChangeY++;
            toChangeX++;
            lScape[toChangeY][toChangeX] = height/i;
            if (toChangeY+1 >= lScape.size())
                break;
            if (toChangeX+1 >= lScape.size())
                break;
        }

        toChangeX = midX;
        toChangeY = midY;

        for (int i = 1; i <= dist; i++) // extends in the opposite direction chosen previously
        {
            toChangeY--;
            toChangeX--;
            lScape[toChangeY][toChangeX] = height/i;
            if (toChangeY-1 >= lScape.size())
                break;
            if (toChangeX-1 >= lScape.size())
                break;
        }
    }
    else if (direction == 1)
    {
        for (int i = 1; i <= dist; i++) // extends in the direction chosen previously
        {
            toChangeX++;
            lScape[toChangeY][toChangeX] = height/i;
            if (toChangeX+1 >= lScape.size())
                break;
        }

        toChangeX = midX;

        for (int i = 1; i <= dist; i++) // extends in the opposite direction chosen previously
        {
            toChangeX--;
            lScape[toChangeY][toChangeX] = height/i;
            if (toChangeX-1 >= lScape.size())
                break;
        }
    }
    else if (direction == 2)
    {
        for (int i = 1; i <= dist; i++) // extends in the direction chosen previously
        {
            toChangeY--;
            toChangeX++;
            lScape[toChangeY][toChangeX] = height/i;
            if (toChangeY-1 >= lScape.size())
                break;
            if (toChangeX+1 >= lScape.size())
                break;
        }

        toChangeX = midX;
        toChangeY = midY;

        for (int i = 1; i <= dist; i++) // extends in the opposite direction chosen previously
        {
            toChangeY++;
            toChangeX--;
            lScape[toChangeY][toChangeX] = height/i;
            if (toChangeY+1 >= lScape.size())
                break;
            if (toChangeX-1 >= lScape.size())
                break;
        }
    }
    else if (direction == 3)
    {
        for (int i = 1; i <= dist; i++) // extends in the direction chosen previously
        {
            toChangeY++;
            lScape[toChangeY][toChangeX] = height/i;
            if (toChangeY+1 >= lScape.size())
                break;
        }

        toChangeY = midY;

        for (int i = 1; i <= dist; i++) // extends in the opposite direction chosen previously
        {
            toChangeY--;
            lScape[toChangeY][toChangeX] = height/i;
            if (toChangeY-1 >= lScape.size())
                break;
        }
    }
}

void findLakePos()
{
    int findLakeFactor = 2;
    bool isLake = false;
    float total = 0;
    int count = 0;

    for (int a = 0; a < lScape.size(); a++)
    {
        for (int b = 0; b < lScape[0].size(); b++)
        {
            total += lScape[a][b]; // adds to total height
            count++; // records number of spaces
            if (lScape[a][b] < low)
                low = lScape[a][b]; // records lowest point
            if (lScape[a][b] > high)
            {
                high = lScape[a][b];// records highest point
                highX = a;
                highY = b;
            }

        }
    }
    total /= count;
    y = (total + high) / 2;

    for (int a = 1; a < lScape.size()-1; a++)
    {
        bvec test;
        for (int b = 1; b < lScape[0].size()-1; b++)
        {
            count = 0;
            if (lScape[a][b]-total < findLakeFactor && lScape[a][b]-total > -findLakeFactor)
            {
                if (lScape[a+1][b]-lScape[a][b] < findLakeFactor && lScape[a+1][b]-lScape[a][b] > -findLakeFactor)
                    count++;

                if (lScape[a+1][b+1]-lScape[a][b] < findLakeFactor && lScape[a+1][b]-lScape[a][b] > -findLakeFactor)
                    count++;

                if (lScape[a][b+1]-lScape[a][b] < findLakeFactor && lScape[a+1][b]-lScape[a][b] > -findLakeFactor)
                    count++;

                if (lScape[a-1][b+1]-lScape[a][b] < findLakeFactor && lScape[a+1][b]-lScape[a][b] > -findLakeFactor)
                    count++;

                if (lScape[a-1][b]-lScape[a][b] < findLakeFactor && lScape[a+1][b]-lScape[a][b] > -findLakeFactor)
                    count++;

                if (lScape[a-1][b-1]-lScape[a][b] < findLakeFactor && lScape[a+1][b]-lScape[a][b] > -findLakeFactor)
                    count++;

                if (lScape[a][b-1]-lScape[a][b] < findLakeFactor && lScape[a+1][b]-lScape[a][b] > -findLakeFactor)
                    count++;

                if (lScape[a+1][b-1]-lScape[a][b] < findLakeFactor && lScape[a+1][b]-lScape[a][b] > -findLakeFactor)
                    count++;
            }
            if (count >= 5)
            {
                test.push_back(1);
                isLake = true;
            }
            else
                test.push_back(0);
        }
        useable.push_back(test);
    }

    if (isLake)
    {
        int lakeY = rand()%useable.size();
        int lakeX = rand()%useable[0].size();

        while (useable[lakeY][lakeX] != 1)
        {
            lakeY = rand()%useable.size();
            lakeX = rand()%useable[0].size();
        }

        waterY = lScape[lakeY+1][lakeX+1] - 0.7f;

        useable[lakeY][lakeX] = 0;

        float moveDown = 10.0f/(1+rand()%2);

        lScape[lakeY+1][lakeX+1] -= moveDown;

        generateLake(lakeX+1, lakeY+1, moveDown*0.75);
        generateLake(lakeX-1, lakeY-1, moveDown*0.75);
        generateLake(lakeX+1, lakeY-1, moveDown*0.75);
        generateLake(lakeX-1, lakeY+1, moveDown*0.75);
    }
}

void generateLake(int x, int y, float height)
{
    if (height < 0.5)
        return;

    if (x < -1 || x > useable.size() || y < -1 || y > useable.size())
        return;

    if (x == -1 || y == -1 || x == useable[0].size() || y == useable.size())
    {
        lScape[y+1][x+1] -= height;
        return;
    }

    if (!useable[y][x])
        return;

    lScape[y+1][x+1] -= height;

    useable[y][x] = 0;

    /*generateLake(x+1, y+1, height*0.5);
    generateLake(x-1, y-1, height*0.5);
    generateLake(x+1, y-1, height*0.5);
    generateLake(x-1, y+1, height*0.5);*/

    generateLake(x, y+1, height*0.75);
    generateLake(x, y-1, height*0.75);
    generateLake(x+1, y, height*0.75);
    generateLake(x-1, y, height*0.75);
}
