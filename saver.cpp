/* celtic_knots - generate and animate random Celtic knots
 * Copyright (C) 2008 Lewis Van Winkle
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <scrnsave.h>

#include <gl/gl.h>
#include <gl/glu.h>

#include <cmath>
#include "cknot.hpp"
#include <algorithm>

#include <stdlib.h>
#include <map>

int Width, Height;
double width, height;
#define TIMER 1

const double ResetTime = 30.0;
const double DrawTime = 20.0;

const bool DrawWire = false;
const bool DrawGraph = false;

static void InitGL(HWND hWnd, HDC & hDC, HGLRC & hRC)
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory (&pfd, sizeof pfd);
    pfd.nSize = sizeof pfd;
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;

    hDC = GetDC (hWnd);

    int i = ChoosePixelFormat(hDC, &pfd);  
    SetPixelFormat(hDC, i, &pfd);

    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);
}


static void CloseGL(HWND hWnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent( NULL, NULL );
    wglDeleteContext( hRC );
    ReleaseDC( hWnd, hDC );
}


BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}


BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
    return TRUE;
}


void InitAnim()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    height = 1.0;
    width = double(Width) / double(Height);

    glOrtho(0.0, width, height, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClearDepth(1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    srand(GetTickCount());

    if (DrawWire)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}


void DestroyAnim()
{
}


CKnot::StrokeType RandomType()
{
    const int r = rand() % 15;

    if (r == 0)
        return CKnot::Bounce;
    else if (r == 1)
        return CKnot::Glance;
    else
        return CKnot::Cross;
}


double Round(double in)
{
    return in;
}


CKnot::StrokeList CreateSquareStrokes()
{
    CKnot::StrokeList sl;

    //Create a square grid.
    const double junctionsPer = 6 + (rand() % 9);

    const size_t junctionsX = size_t(junctionsPer * width);
    const size_t junctionsY = size_t(junctionsPer * height);

    //cx and nx store the current and next x.
    //They must not be recalculated each iteration, or the results sometimes don't
    //match (even if they are calculated in EXACTLY the same way).
    double cx = double(1) / junctionsX * width;

    for (size_t x = 1; x < junctionsX; ++x)
    {
        double nx = double(x + 1) / junctionsX * width;

        double cy = double(1) / junctionsY * height;

        for (size_t y = 1; y < junctionsY; ++y)
        {
            double ny = double(y + 1) / junctionsY * height;

            if (x + 1 != junctionsX)
                sl.push_back(CKnot::Stroke(CKnot::vec2(cx, cy), CKnot::vec2(nx, cy), RandomType()));

            if (y + 1 != junctionsY)
                sl.push_back(CKnot::Stroke(CKnot::vec2(cx, cy), CKnot::vec2(cx, ny), RandomType()));

            cy = ny;
        }

        cx = nx;
    }

    return sl;
}


CKnot::StrokeList RemoveStrokes(CKnot::StrokeList& in)
{
    CKnot::StrokeList sl = in;

    //Delete some strokes at random.
    const int delThres = RAND_MAX / (3 + (rand() % 20));
    for (CKnot::StrokeList::iterator it = sl.begin(); it != sl.end();)
    {
        if (rand() < delThres)
            sl.erase(it++);
        else
            ++it;
    }

    //Now purge all strokes that aren't connected at both ends.
    //This gets rid of loops that can make the graphics overlap.
    std::map<CKnot::vec2, size_t> sCount;
    for (CKnot::StrokeList::iterator it = sl.begin(); it != sl.end(); ++it)
    {
        //Count strokes at each junction.
        if (sCount.count(it->a))
            ++(sCount[it->a]);
        else
            sCount[it->a] = 1;

        if (sCount.count(it->b))
            ++(sCount[it->b]);
        else
            sCount[it->b] = 1;
    }
    for (CKnot::StrokeList::iterator it = sl.begin(); it != sl.end();)
    {
        //Remove strokes that have open junctions.
        //Removing these can make more open junctions,
        //but these new loops will have some room to not hit other curves.
        if (sCount[it->a] == 1 || sCount[it->b] == 1)
            sl.erase(it++);
        else
            ++it;
    }

    return sl;
}


typedef std::vector<float> FloatArray;
typedef std::vector<FloatArray*> Arrays;


void DoAnim()
{
    static double time = 0.0; //Total time running.
    static double artTime = 0.0; //Total time with the current art.

    static DWORD lastTime = 0; //Time of last call.
    const double elapsed = double(GetTickCount() - lastTime) / 1000.0;

    if (lastTime)
    {
        lastTime = GetTickCount();
    }
    else
    {
        lastTime = GetTickCount();
        return;
    }

    time += elapsed;
    artTime += elapsed;

    //If we need new art, get it.
    static CKnot::AutoArt threads;
    static Arrays arrays;
    static FloatArray grid;


    if (!threads.get() || artTime > ResetTime)
    {
        CKnot::StrokeList sl = CreateSquareStrokes();
        sl = RemoveStrokes(sl);

        threads = CKnot::CreateThread(sl);
        artTime = 0.0;

        grid.clear();
        grid.reserve(sl.size() * 10);
        for (CKnot::StrokeList::const_iterator it = sl.begin(); it != sl.end(); ++it)
        {
            grid.push_back(it->a.x);
            grid.push_back(it->a.y);
            grid.push_back(it->type == CKnot::Cross ? 1.0 : 0.0);
            grid.push_back(it->type == CKnot::Glance ? 1.0 : 0.0);
            grid.push_back(it->type == CKnot::Bounce ? 1.0 : 0.0);

            grid.push_back(it->b.x);
            grid.push_back(it->b.y);
            grid.push_back(it->type == CKnot::Cross ? 1.0 : 0.0);
            grid.push_back(it->type == CKnot::Glance ? 1.0 : 0.0);
            grid.push_back(it->type == CKnot::Bounce ? 1.0 : 0.0);
        }

        for (size_t i = 0; i < arrays.size(); ++i)
            delete arrays[i];
        arrays.clear();

        const size_t threadCount = threads->GetThreadCount();

        for (size_t i = 0; i < threadCount; ++i)
        {
            const CKnot::Art::Thread* thread = threads->GetThread(i);
            const CKnot::Art::Z* z = threads->GetZ(i);

            const size_t segsPerKnot = 25;
            const size_t kc = thread->GetKnotCount();

            FloatArray* quads = new FloatArray;
            arrays.push_back(quads);


            const size_t target = kc * segsPerKnot;
            const size_t memSize = 12 * (target + 1);
            quads->reserve(memSize);

            const float scr = double(rand()) / RAND_MAX / 2;
            const float ecr = double(rand()) / RAND_MAX / 2 + .5;
            const float scg = double(rand()) / RAND_MAX / 2;
            const float ecg = double(rand()) / RAND_MAX / 2 + .5;
            const float scb = double(rand()) / RAND_MAX / 2;
            const float ecb = double(rand()) / RAND_MAX / 2 + .5;

            for (size_t i = 0; i <= target; ++i)
            {
                const double s = double(i) / double(target);
                const double t = s;

                const CKnot::vec2 cur = thread->Y(t);
                const CKnot::vec2 dcur = thread->Y(t + .00001);
                const CKnot::vec2 diff = dcur - cur;

                CKnot::vec2 normal(diff.y, -diff.x);
                normal = normal * (1.0 / normal.GetLength());
                normal = normal * .01;
                const CKnot::vec2 flip(-normal.x, -normal.y);

                const CKnot::vec2 start = cur + normal;
                const CKnot::vec2 end = cur + flip;

                const bool over = z->Y(t) > 0.0;

                //Coords
                quads->push_back(start.x);
                quads->push_back(start.y);
                quads->push_back(over ? 0.01 : .1);

                //Colors
                quads->push_back(scr);
                quads->push_back(scg);
                quads->push_back(scb);

                quads->push_back(end.x);
                quads->push_back(end.y);
                quads->push_back(over ? 0.01 : .1);

                quads->push_back(ecr);
                quads->push_back(ecg);
                quads->push_back(ecb);
            }

            assert(quads->size() == memSize);
        }
    }


    //Clear the background some nice color.
    glClearColor(   0.125f + std::sin(time / 2.0) / 8.0,
                    0.125f + std::sin(time / 3.0) / 8.0,
                    0.125f + std::sin(time / 5.0) / 8.0,
                    0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    for (size_t i = 0; i < arrays.size(); ++i)
    {
        FloatArray& quads = *arrays[i];

        glVertexPointer(3, GL_FLOAT, 24, &quads.front());
        glColorPointer(3, GL_FLOAT, 24, &quads.front() + 3);

        const size_t count = quads.size() / 6;
        const size_t progress = size_t(std::min(artTime / DrawTime, 1.0) * count / 2); //From 0 to .5 of vertices.
        assert(progress >= 0);
        assert(progress <= count / 2);

        size_t start = (count / 2) - progress;
        start += start % 2;

        glDrawArrays(GL_QUAD_STRIP, start, progress * 2);
    }

    //Draw graph
    if (DrawGraph)
    {
        glVertexPointer(2, GL_FLOAT, 20, &grid.front());
        glColorPointer(3, GL_FLOAT, 20, &grid.front() + 2);
        glDrawArrays(GL_LINES, 0, grid.size() / 5);
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glLoadIdentity();
}


LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HDC hDC;
    static HGLRC hRC;
    static RECT rect;

    switch (message)
    {
        case WM_CREATE:
            GetClientRect(hWnd, &rect);
            Width = rect.right;
            Height = rect.bottom;

            InitGL(hWnd, hDC, hRC);
            InitAnim();

            SetTimer(hWnd, TIMER, 10, NULL);
            return 0;

        case WM_DESTROY:
            KillTimer(hWnd, TIMER);
            DestroyAnim();
            CloseGL(hWnd, hDC, hRC);
            return 0;

        case WM_TIMER:
            DoAnim();
            SwapBuffers(hDC);
            return 0;

    }

    return DefScreenSaverProc(hWnd, message, wParam, lParam);
}
